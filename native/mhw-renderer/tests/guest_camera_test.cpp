#include "guest_camera.hpp"
#include <cassert>
#include <iostream>

using namespace crafterhunter;
namespace gc=guest_camera;
bool near(double a,double b) {return std::abs(a-b)<1e-3;}
int main(int argc,char** argv) {
    assert(argc==2);
    gc::Sample java;assert(gc::read(argv[1],java));
    assert(java.generation==123 && java.request==456 && java.sequence==7 && java.mode==2);
    assert(java.position[2]==34 && java.projection[5]==2);
    alignment::Anchor a{{10,20,30},{100,200,300},1,123,9,true};
    gc::Plan p;
    // First person, rear third person and opposite-facing front share ONE anchor.
    for(unsigned mode=0;mode<3;++mode) {
        auto s=java;s.mode=mode;s.position={10,21.62,30.0+(mode==1 ? 4 : mode==2 ? -4 : 0)};
        if(mode==2) {s.view[0]=-1;s.view[10]=-1;}
        assert(gc::makePlan(s,a,9,p));
        reprojection::Matrix inverse{},v{};for(unsigned i=0;i<16;++i)v[i]=p.view[i];
        assert(reprojection::inverse(v,inverse));
        assert(near(inverse[12],10000) && near(inverse[13],20162));
        assert(near(inverse[14],30000+(mode==1 ? 400 : mode==2 ? -400 : 0)));
        auto camera=reprojection::transform(v,{inverse[12],inverse[13],inverse[14],1});
        assert(near(camera[0],0) && near(camera[1],0) && near(camera[2],0));
        // Shared world point maps back to exactly the guest's view, in centimetres.
        auto mapped=reprojection::transform(v,{10100,20200,30300,1});
        auto expected=reprojection::transform(s.view,{11-s.position[0],22-s.position[1],33-s.position[2],1});
        for(unsigned i=0;i<3;++i) assert(near(mapped[i],expected[i]*100));
    }
    java.position={10,21.62,31.25}; // collision-shortened rear camera, not guessed 4 blocks
    assert(gc::makePlan(java,a,9,p));assert(near(p.view[14],-30125));
    auto native=p.projection;native[10]=0.123f;native[11]=-1;native[14]=12.5f;
    assert(gc::projection(p,1.5f,native.data()));
    assert(near(native[0],2.0/1.5) && near(native[5],2));
    assert(native[10]==0.123f && native[11]==-1 && native[14]==12.5f); // native Z survives
    assert(!gc::makePlan(java,a,10,p));
    java.view[0]=0;assert(!gc::makePlan(java,a,9,p)); // singular refusal
    // Real view effects can include roll and camera-relative translation.
    java.view={0,0,-1,0,-1,0,0,0,0,1,0,0,0.125,-0.25,0.5,1};
    assert(gc::makePlan(java,a,9,p));
    reprojection::Matrix rotated{};for(unsigned i=0;i<16;++i)rotated[i]=p.view[i];
    auto actual=reprojection::transform(rotated,{10100,20200,30300,1});
    auto expected=reprojection::transform(java.view,{11-java.position[0],22-java.position[1],33-java.position[2],1});
    for(unsigned i=0;i<3;++i) assert(near(actual[i],expected[i]*100));
    gc::Lease l;l.start(456,100);java.view[0]=1;
    assert(!l.observe(java,9,true,101));assert(!l.live(101)); // frozen startup is not control
    ++java.sequence;assert(l.observe(java,9,true,110));assert(l.live(609));
    assert(l.observe(java,9,true,600)); // duplicate does not refresh expiry
    assert(!l.observe(java,9,true,610));assert(l.state==gc::Lease::State::Released);
    l.start(456,700);++java.sequence;assert(!l.observe(java,9,true,701)); // latch, no auto resume
    l.start(457,710);java.request=457;
    assert(!l.observe(java,9,true,711));++java.sequence;assert(l.observe(java,9,true,712));
    assert(!l.observe(java,10,true,713)); // host epoch change releases
    l.start(458,800);java.request=458;assert(!l.observe(java,9,true,801));
    ++java.sequence;assert(l.observe(java,9,true,802));
    ++java.generation;assert(!l.observe(java,9,true,803)); // process replacement releases
    l.start(459,900);java.request=459;assert(!l.observe(java,9,true,901));
    ++java.sequence;assert(l.observe(java,9,true,902));
    assert(!l.observe(java,9,false,903)); // loss of filter/camera/anchor releases
    l.start(460,1000);assert(!l.observe(java,9,false,3500)); // bounded pending request
    l.start(461,4000);java.request=461;assert(!l.observe(java,9,true,4001));
    ++java.sequence;assert(l.observe(java,9,true,4100));
    assert(!l.observe(java,9,true,4050)); // clock rollback releases
    std::cout<<"guest camera F5/fixed-anchor/centimetres/native-Z/lease regression passed\n";
}
