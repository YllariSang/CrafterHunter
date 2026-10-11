#include "guest_camera.hpp"
#include "world_freshness.hpp"
#include <cstdio>
#include <stdexcept>

using namespace crafterhunter;
namespace gc=guest_camera;
using M=reprojection::Matrix;
void check(bool ok,const char* message) { if(!ok) throw std::runtime_error(message); }
M identity() { M m{};m[0]=m[5]=m[10]=m[15]=1;return m; }
M multiply(const M& a,const M& b) {
    M out{};
    for(unsigned c=0;c<4;++c) for(unsigned r=0;r<4;++r)
        for(unsigned k=0;k<4;++k) out[c*4+r]+=a[k*4+r]*b[c*4+k];
    return out;
}
gc::Sample capturedSample(const world::Snapshot& frame,const gc::Sample& live) {
    auto sample=live;sample.mode=frame.cameraMode;sample.position=frame.cameraPosition;
    for(unsigned i=0;i<16;++i) {sample.view[i]=frame.viewRotation[i];sample.projection[i]=frame.projection[i];}
    return sample;
}
// Independent projected-position oracle: the frozen frame's final projection
// sees its own frozen body. Host effects/centimetres must preserve its clip XY/W.
void projectBody(const world::Snapshot& frame,const alignment::Anchor& anchor,const gc::Plan& plan) {
    auto projection=plan.projection;
    const float aspect=frame.height ? float(frame.width)/frame.height : 1.75f;
    const auto native=projection;
    check(gc::projection(plan,aspect,projection.data()),"native projection refused");
    for(unsigned col=0;col<4;++col) for(unsigned row=2;row<4;++row)
        check(projection[col*4+row]==native[col*4+row],"native Z/W changed");
    M view{},p{},fv{},fp{};
    for(unsigned i=0;i<16;++i) {view[i]=plan.view[i];p[i]=projection[i];fv[i]=frame.viewRotation[i];fp[i]=frame.projection[i];}
    const auto point=frame.playerPosition;
    reprojection::Vector host{},guest{};host[3]=guest[3]=1;
    for(unsigned i=0;i<3;++i) {
        const double centre=point[i]+(i==1 ? .9:0);
        host[i]=(anchor.hostOrigin[i]+anchor.metresPerBlock*(centre-anchor.guestOrigin[i]))*100;
        guest[i]=centre-frame.cameraPosition[i];
    }
    const auto actual=reprojection::transform(multiply(p,view),host);
    auto wanted=reprojection::transform(multiply(fp,fv),guest);
    M perspective{},effect{};check(gc::projectionParts(fp,perspective,effect),"frozen projection invalid");
    wanted[0]*=(perspective[5]/perspective[0])/aspect;
    check(std::abs(actual[3])>1e-6 && std::abs(wanted[3])>1e-6,"probe at eye plane");
    for(unsigned i=0;i<2;++i)
        check(std::abs(actual[i]/actual[3]-wanted[i]/wanted[3])<1e-4,
            "flying body/camera projection times disagree");
}
int main(int argc,char** argv) {
    try {
        alignment::Anchor anchor{{10,20,30},{100,200,300},1,123,9,true};
        gc::Sample live;live.generation=123;live.request=456;live.sequence=700;live.flags=1;
        live.view=identity();live.projection={1,0,0,0,0,1.8,0,0,0,0,-1.001,-1,0,0,-.1,0};live.mode=1;
        world::Snapshot frame;frame.generation=123;frame.identity=10;frame.playerKnown=true;
        frame.width=949;frame.height=509;frame.playerPosition={10,20,30};frame.cameraPosition={10,21.62,34};frame.cameraMode=1;
        for(unsigned i=0;i<16;++i) {frame.projection[i]=live.projection[i];frame.viewRotation[i]=live.view[i];}
        // Flying/camera motion advances ahead of the frozen body and attachments.
        // A latest-camera substitution must fail the numerical oracle above.
        for(unsigned step=0;step<8;++step) {
            live.position={10+.15*step,21.62+.3*step,34-.12*step};++live.sequence;
            gc::Plan plan;check(gc::makePairedPlan(frame,live,anchor,9,plan),"frozen camera refused");
            projectBody(frame,anchor,plan);
            check(plan.captureIdentity==frame.identity && plan.sequence==live.sequence,"render/control identities conflated");
        }
        // Capture advances with its body: constant relative framing through flight.
        for(unsigned step=0;step<8;++step) {
            frame.playerPosition={10+.4*step,20+.7*step,30-.3*step};
            frame.cameraPosition={frame.playerPosition[0],frame.playerPosition[1]+1.62,frame.playerPosition[2]+4};++frame.identity;
            gc::Plan plan;check(gc::makePairedPlan(frame,live,anchor,9,plan),"advanced capture refused");projectBody(frame,anchor,plan);
        }
        for(unsigned mode=0;mode<3;++mode) {
            frame.cameraMode=mode;live.mode=(mode+1)%3; // F5 live feed can lead the captured view.
            frame.cameraPosition={10,21.62,mode==1 ? 31.25:mode==2 ? 26:30};
            auto view=identity();if(mode==2) {view[0]=-1;view[10]=-1;}
            for(unsigned i=0;i<16;++i) frame.viewRotation[i]=view[i];
            const auto unchangedAnchor=anchor;
            gc::Plan plan;check(gc::makePairedPlan(frame,live,anchor,9,plan),"F5 capture refused");
            check(plan.mode==mode,"F5 visibility uses a different render time");
            check(anchor.guestOrigin==unchangedAnchor.guestOrigin && anchor.hostEpoch==9,"F5 re-anchored");
            M mappedView{},eye{};for(unsigned i=0;i<16;++i)mappedView[i]=plan.view[i];
            check(reprojection::inverse(mappedView,eye),"F5 view singular");
            for(unsigned i=0;i<3;++i)
                check(std::abs(eye[12+i]-(anchor.hostOrigin[i]+frame.cameraPosition[i]-anchor.guestOrigin[i])*100)<1e-3,
                    "F5/collision camera did not use the fixed anchor");
        }
        frame.cameraMode=1;frame.cameraPosition={10,21.62,34};frame.playerPosition={10,20,30};
        // Final bob/hurt/roll effects and off-centre perspective remain exact.
        auto perspective=live.projection;perspective[8]=.15;perspective[9]=-.1;
        M effect{.8,.6,0,0,-.6,.8,0,0,0,0,1,0,.03,-.05,.1,1};
        auto final=multiply(perspective,effect);
        for(unsigned i=0;i<16;++i) {frame.projection[i]=final[i];frame.viewRotation[i]=identity()[i];}
        gc::Plan plan;check(gc::makePairedPlan(frame,live,anchor,9,plan),"effect capture refused");projectBody(frame,anchor,plan);
        const auto good=frame;
        auto refuse=[&](){gc::Plan retained;retained.captureIdentity=999;
            check(!gc::makePairedPlan(frame,live,anchor,9,retained),"invalid frame accepted");
            check(retained.captureIdentity==999,"refusal replaced output");};
        frame.identity=0;refuse();frame=good;
        frame.generation++;refuse();frame=good;
        frame.playerKnown=false;refuse();frame=good;
        frame.cameraMode=3;refuse();frame=good;
        frame.viewRotation={};refuse();frame=good;
        frame.projection[0]=INFINITY;refuse();frame=good;
        live.flags=0;refuse();live.flags=1;
        check(!gc::makePairedPlan(frame,live,anchor,10,plan),"wrong anchor epoch accepted");
        // Captured camera identities never renew an independent control lease.
        gc::Lease lease;lease.start(live.request,100);check(!lease.observe(live,9,true,101),"old feed warmed lease");
        ++live.sequence;check(lease.observe(live,9,true,110),"fast lease did not advance");
        frame.identity++;check(gc::makePairedPlan(frame,live,anchor,9,plan),"duplicate-feed plan refused");
        check(lease.observe(live,9,true,500) && !lease.observe(live,9,true,610),"capture renewed expired lease");
        world::Freshness freshness;freshness.observe(123,1,100);freshness.observe(123,2,110);
        check(freshness.live(110) && !freshness.live(1110),"paired freshness relaxed");
        for(int i=1;i<argc;++i) {
            world::Snapshot archived;check(world::read(argv[i],archived),"archived capture refused");
            anchor.guestOrigin=archived.playerPosition;anchor.guestGeneration=archived.generation;
            live.generation=archived.generation;live.mode=archived.cameraMode;
            gc::Plan replay;check(gc::makePairedPlan(archived,live,anchor,9,replay),"actual captured camera refused");
            if(archived.cameraMode!=0) projectBody(archived,anchor,replay);
        }
        std::puts("PASS frozen camera/body flight, captured F5/collision/effects, native Z/W, fixed anchor and unchanged independent lease");
    } catch(const std::exception& e) {std::fprintf(stderr,"FAIL: %s\n",e.what());return 1;}
}
