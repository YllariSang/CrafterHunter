#include "guest_camera_frame.hpp"
#include "composition_requests.hpp"
#include <cstdio>
#include <stdexcept>

namespace gc=crafterhunter::guest_camera;
namespace composition=crafterhunter::composition;
void check(bool ok,const char* text) { if(!ok) throw std::runtime_error(text); }
gc::Plan camera(unsigned mode,std::uint64_t capture,float z=0) {
    gc::Plan p;p.mode=mode;p.captureIdentity=capture;p.generation=11;p.request=22;
    p.sequence=capture;p.epoch=33;p.expires=1000;
    p.view={1,0,0,0,0,1,0,0,0,0,1,0,0,0,-z,1};
    p.projection={1,0,0,0,0,1,0,0,0,0,-1,-1,0,0,-1,0};
    if(mode==2) {p.view[0]=-1;p.view[10]=-1;}
    return p;
}
std::array<float,16> vp(const gc::Plan& p,float aspect=1) {
    auto projection=p.projection;check(gc::projection(p,aspect,projection.data()),"projection failed");
    std::array<float,16> out{};
    for(unsigned c=0;c<4;++c) for(unsigned r=0;r<4;++r)
        for(unsigned k=0;k<4;++k) out[c*4+r]+=projection[k*4+r]*p.view[c*4+k];
    return out;
}
// Replay measured X/Y/W rows without requiring original game assets or poses.
gc::Plan framing(unsigned mode,std::uint64_t id,const std::array<float,16>& matrix) {
    auto p=camera(mode,id);
    for(unsigned c=0;c<4;++c) {
        p.view[c*4]=matrix[c*4];p.view[c*4+1]=matrix[c*4+1];
        p.view[c*4+2]=-matrix[c*4+3];p.view[c*4+3]=c==3 ? 1:0;
    }
    return p;
}
int main(int argc,char** argv) {
    try {
        auto first=camera(0,1),rear=camera(1,2,400),front=camera(2,3,-400);
        auto firstVP=vp(first),rearVP=vp(rear),frontVP=vp(front);
        gc::FrameModeGate gate;
        check(gate.observe(first,1,firstVP.data(),10),"first camera not established");
        // Source body face is six centimetres ahead of the first-person camera.
        // Under the old gate, a newly visible rear body filled the old eye view.
        const auto faceWidth=[&](const auto& matrix) {
            const double w=-6*matrix[11]+matrix[15];
            return 1920*.5*50*std::abs(matrix[0]/w);
        };
        check(faceWidth(firstVP)>1920 && faceWidth(rearVP)<200,"flash oracle does not reproduce near-eye enlargement");
        const bool draw=gate.observe(rear,1,firstVP.data(),20);
        check((draw ? faceWidth(firstVP):0)<200,"first-person frame drew new rear body");
        check(!draw && gate.mode()==0,"paired attachments not held with body during F5");
        check(gate.observe(rear,1,rearVP.data(),30),"rear did not resume after matching frame camera");
        check(!gate.observe(front,1,rearVP.data(),40),"rear frame drew front capture");
        check(gate.observe(front,1,frontVP.data(),50),"front camera did not resume");
        check(!gate.observe(first,1,frontVP.data(),60),"front frame drew first-person capture");
        check(gate.observe(first,1,firstVP.data(),70),"first camera did not resume");

        // Keep a new same-mode capture's plan, then accept the next host frame
        // with that plan even if walking/flying has already advanced the capture.
        check(!gate.observe(rear,1,firstVP.data(),80),"transition did not wait");
        auto moved=rear;moved.captureIdentity=4;moved.sequence=4;moved.view[12]=20;moved.view[13]=35;
        check(gate.observe(moved,1,rearVP.data(),90),"moving F5 transition requires exact newest capture");
        auto further=moved;further.captureIdentity=5;further.view[12]=40;
        check(gate.observe(further,1,rearVP.data(),100),"steady same-mode capture advances stalled");
        check(gate.mode()==1,"steady mode changed");
        auto bad=rearVP;bad[0]=INFINITY;
        check(!gate.observe(further,1,bad.data(),110),"nonfinite scene camera accepted");
        check(!gate.observe(further,0,rearVP.data(),110),"invalid aspect accepted");
        check(!gate.observe(further,1,nullptr,110),"missing scene camera accepted");
        check(!gate.observe(further,1,rearVP.data(),1000),"expired plan accepted");
        check(gate.mode()==gc::FrameModeGate::Unknown,"expiry kept transition state");
        for(unsigned scope=0;scope<3;++scope) {
            gate.reset();check(gate.observe(first,1,firstVP.data(),10),"scope warmup failed");
            auto next=rear;
            if(scope==0) ++next.generation;
            if(scope==1) ++next.request;
            if(scope==2) ++next.epoch;
            check(!gate.observe(next,1,firstVP.data(),20),"new ownership scope inherited camera mode");
            check(gate.observe(next,1,rearVP.data(),30),"new scope did not resume");
        }
        // Native depth rows differ from the guest perspective; framing matches
        // only X/Y/W. Native reversed-Z remains untouched by this CPU guard.
        auto native=rearVP;for(unsigned c=0;c<4;++c)native[c*4+2]=c==3 ? 16.f:.00001f;
        check(gc::sameFrameCamera(rear,1,native.data()),"native Z terms treated as guest depth");
        check(!gc::sameFrameCamera(first,1,native.data()),"different eye planes match");
        auto wide=vp(rear,16.f/9);
        check(gc::sameFrameCamera(rear,16.f/9,wide.data()),"host aspect not applied");
        check(!gc::sameFrameCamera(rear,1,wide.data()),"wrong host aspect accepted");
        // Off-centre projection and rigid effects survive the comparison,
        // including cancellation in a large translated camera matrix.
        auto effects=rear;effects.projection[8]=.12f;effects.projection[9]=-.07f;
        effects.view={.8f,.6f,0,0,-.6f,.8f,0,0,0,0,1,0,80000,-60000,-100000,1};
        auto effectVP=vp(effects,16.f/9);
        check(gc::sameFrameCamera(effects,16.f/9,effectVP.data()),"float effects/cancellation refused");
        auto other=effects;other.view[14]+=400;
        check(!gc::sameFrameCamera(other,16.f/9,effectVP.data()),"large-origin first/rear displacement conflated");
        check(composition::fallback(true,false)==composition::Fallback::None,
            "refused full paired draw became overlay");
        check(composition::fallback(true,true)==composition::Fallback::Preview,"opt-in preview fallback lost");
        check(composition::fallback(false,true)==composition::Fallback::Preview,"preview route lost");
        check(composition::fallback(false,false)==composition::Fallback::Overlay,"standalone overlay route lost");
        unsigned replayed=0;
        for(int a=1;a<argc;++a) {
            FILE* f=std::fopen(argv[a],"r");check(f,"missing measured frame fixture");
            unsigned oldMode=0,newMode=0;
            while(std::fscanf(f,"%u %u",&oldMode,&newMode)==2) {
                std::array<float,16> old{},next{};
                for(float& x:old)check(std::fscanf(f,"%f",&x)==1,"short CPU fixture");
                for(float& x:next)check(std::fscanf(f,"%f",&x)==1,"short selected fixture");
                auto prior=framing(oldMode,1,old),selected=framing(newMode,2,next);
                gate.reset();check(gate.observe(prior,1,old.data(),10),"measured prior mode refused");
                check(!gate.observe(selected,1,old.data(),20),"measured mismatched frame drew guest");
                check(gate.observe(selected,1,next.data(),30),"measured matching frame refused");++replayed;
            }
            std::fclose(f);
        }
        std::printf("PASS F5 near-eye flash, all mode transitions, moving capture history, ownership/expiry/refusal, native Z and paired fallback; %u measured samples\n",replayed);
    } catch(const std::exception& e) {std::fprintf(stderr,"FAIL: %s\n",e.what());return 1;}
}
