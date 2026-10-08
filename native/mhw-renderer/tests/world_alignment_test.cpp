#include "world_alignment.hpp"
#include <cassert>
#include <cstdio>
#include <limits>
using namespace crafterhunter;
int main() {
    const reprojection::Matrix identity{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
    alignment::Anchor a{{917,-60,348},{-249,40,407},1,7,9,true};
    reprojection::Matrix world{},eye{};
    assert(alignment::worldMatrix(a,7,9,world));
    auto p=reprojection::transform(world,{918,-58,345,1});
    assert(p[0]==-248 && p[1]==42 && p[2]==404); // not absolute MC coordinates
    reprojection::Matrix inv{}; assert(reprojection::inverse(world,inv));
    auto back=reprojection::transform(inv,p);
    assert(back[0]==918 && back[1]==-58 && back[2]==345);
    // All three cameras reconstruct the SAME world block. A per-F5 anchor or
    // player-position-only transform makes this fail for the displaced cameras.
    const reprojection::Vector block{918,-59,349,1};
    for(unsigned mode=0;mode<3;++mode) {
        auto view=identity;
        alignment::Position camera{917,-58.38,343};
        if(mode==1) camera[2]-=4;
        if(mode==2) { camera[2]+=4; view[0]=-1; view[10]=-1; }
        reprojection::Vector relative=block;
        for(unsigned i=0;i<3;++i) relative[i]-=camera[i];
        const auto guestEye=reprojection::transform(view,relative);
        assert(alignment::eyeMatrix(a,7,9,view,camera,eye));
        const auto host=reprojection::transform(eye,guestEye);
        const auto expected=reprojection::transform(world,block);
        for(unsigned i=0;i<4;++i) assert(std::abs(host[i]-expected[i])<1e-10);
    }
    a.metresPerBlock=2; assert(alignment::worldMatrix(a,7,9,world));
    p=reprojection::transform(world,{918,-59,349,1});
    assert(p[0]==-247 && p[1]==42 && p[2]==409);
    const auto unchanged=world;
    assert(!alignment::worldMatrix(a,8,9,world) && world==unchanged);
    assert(!alignment::worldMatrix(a,7,10,world) && world==unchanged);
    a.metresPerBlock=-1; assert(!alignment::worldMatrix(a,7,9,world));
    a.metresPerBlock=std::numeric_limits<double>::quiet_NaN();
    assert(!alignment::worldMatrix(a,7,9,world));
    a.reset(); assert(!alignment::worldMatrix(a,7,9,world));
    alignment::Session session;
    assert(!session.calibrate({1,2,3},{4,5,6},7,0));
    session.observe({4,5,6},100);
    assert(session.calibrate({1,2,3},{4,5,6},7,0));
    assert(!session.calibrate({1,2,3},{4,5,6},7,1));
    session.observe({5,5,6},200); assert(session.anchor.armed);
    using R=alignment::Session::Refusal;
    assert(session.observe({50,5,6},300)==R::Jump); assert(!session.anchor.armed && session.epoch==2);
    assert(session.calibrate({1,2,3},{50,5,6},7,0));
    assert(session.observe({50,5,6},1501)==R::Gap); assert(!session.anchor.armed);
    assert(session.observe({50,5,6},1000)==R::ClockRollback); assert(session.epoch==4);
    assert(session.observe({50,5,6},2000)==R::None); // exact 1 s remains accepted
    assert(session.observe({75,5,6},2001)==R::None); // exact 25 m remains accepted
    assert(session.observe({std::numeric_limits<double>::quiet_NaN(),5,6},2002)==R::InvalidPose);
    std::puts("PASS: anchor translation/scale, inverse, F5 fixed-block invariance and generation/scene/reset refusal");
}
