#include "guest_camera.hpp"
#include <cassert>
#include <iostream>
#include <limits>

using namespace crafterhunter;
namespace gc=guest_camera;
bool near(double a,double b) {return std::abs(a-b)<1e-3;}
reprojection::Matrix multiply(const reprojection::Matrix& a,const reprojection::Matrix& b) {
    reprojection::Matrix result{};
    for(unsigned col=0;col<4;++col)for(unsigned row=0;row<4;++row)
        for(unsigned k=0;k<4;++k)result[col*4+row]+=a[k*4+row]*b[col*4+k];
    return result;
}
bool decodeSample(const gc::Sample& sample,gc::Sample& decoded) {
    std::array<unsigned char,192> bytes{};
    auto store=[&](unsigned at,auto value){std::memcpy(bytes.data()+at,&value,sizeof value);};
    store(0,std::uint32_t{0x43474843});store(4,std::uint32_t{1});
    store(8,sample.generation);store(16,sample.request);store(24,sample.sequence);
    store(32,sample.mode);store(36,sample.flags);
    for(unsigned i=0;i<3;++i)store(40+i*8,sample.position[i]);
    for(unsigned i=0;i<16;++i) {
        store(64+i*4,static_cast<float>(sample.view[i]));
        store(128+i*4,static_cast<float>(sample.projection[i]));
    }
    return gc::decode(bytes.data(),bytes.size(),decoded);
}
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
    // First rejected live walking packet, sequence 8437 on 2026-10-11.
    // Its final projection contains bob rotation/translation; old p[11] == -1
    // validation rejected it and immediately released an otherwise healthy lease.
    auto walking=java;
    walking.projection={
        0.765992283821106,0.0019011441618204117,0,-0.0,
        -0.0010196756338700652,1.4281318187713623,2.2370204533217475e-7,-0.004581194370985031,
        4.671381702792132e-6,-0.00654261838644743,4.88299920107238e-5,-0.9999894499778748,
        0.009737290441989899,-0.0838579386472702,0.05000244081020355,-0.0};
    assert(std::abs(walking.projection[11]+1)>1e-5);
    gc::Sample decoded;assert(decodeSample(walking,decoded));
    reprojection::Matrix perspective{},effect{};
    assert(gc::projectionParts(walking.projection,perspective,effect));
    const auto rebuilt=multiply(perspective,effect);
    for(unsigned i=0;i<16;++i)assert(near(rebuilt[i],walking.projection[i]));
    auto checkEffects=[&](const gc::Sample& sample) {
        assert(decodeSample(sample,decoded));assert(gc::makePlan(sample,a,9,p));
        auto hostProjection=p.projection;
        hostProjection[10]=-1.00001f;hostProjection[11]=-1;hostProjection[14]=-16;
        const auto original=hostProjection;
        const float guestAspect=p.projection[5]/p.projection[0];
        assert(gc::projection(p,guestAspect,hostProjection.data()));
        for(unsigned col=0;col<4;++col)for(unsigned row=2;row<4;++row)
            assert(hostProjection[col*4+row]==original[col*4+row]);
        reprojection::Matrix hostView{},hostP{};
        for(unsigned i=0;i<16;++i){hostView[i]=p.view[i];hostP[i]=hostProjection[i];}
        const auto hostClip=reprojection::transform(multiply(hostP,hostView),{10100,20200,29900,1});
        const auto guestEye=reprojection::transform(sample.view,
            {11-sample.position[0],22-sample.position[1],29-sample.position[2],1});
        const auto guestClip=reprojection::transform(sample.projection,guestEye);
        for(unsigned i=0;i<2;++i)assert(near(hostClip[i]/hostClip[3],guestClip[i]/guestClip[3]));
    };
    checkEffects(walking);
    // Synthetic hurt/roll plus Z translation and off-centre perspective also
    // changes p[15]; neither walking nor this valid effect can expire control.
    auto hurt=java;
    reprojection::Matrix hurtEffect{0.8,0.6,0,0,-0.6,0.8,0,0,0,0,1,0,0.03,-0.05,0.1,1};
    auto offCentre=java.projection;offCentre[8]=0.15;offCentre[9]=-0.1;
    hurt.projection=multiply(offCentre,hurtEffect);assert(hurt.projection[15]!=0);
    checkEffects(hurt);
    gc::Lease walkingLease;walkingLease.start(walking.request,100);
    assert(!walkingLease.observe(decoded,9,true,101));
    ++decoded.sequence;assert(walkingLease.observe(decoded,9,true,110));
    assert(walkingLease.live(110));
    auto invalid=walking;invalid.projection[6]+=0.02; // not perspective * rigid effect
    assert(!decodeSample(invalid,decoded));
    invalid=walking;invalid.projection[3]=0.5;assert(!decodeSample(invalid,decoded));
    invalid=walking;invalid.projection[14]=0;assert(!decodeSample(invalid,decoded));
    invalid=walking;invalid.projection[0]=std::numeric_limits<double>::infinity();
    assert(!decodeSample(invalid,decoded));
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
