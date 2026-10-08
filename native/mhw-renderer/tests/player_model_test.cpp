#include "player_model.hpp"
#include "world_alignment.hpp"
#include "world_freshness.hpp"
#include <cassert>
#include <fstream>
#include <iostream>
using namespace crafterhunter;
static void write(const char* path,const std::vector<std::uint8_t>& b) {
    std::ofstream f(path,std::ios::binary); f.write(reinterpret_cast<const char*>(b.data()),b.size());
}
static void put32(std::vector<std::uint8_t>& b,unsigned offset,std::uint32_t n) {
    for(unsigned i=0;i<4;++i) b[offset+i]=n>>(i*8);
}
static void put64(std::vector<std::uint8_t>& b,unsigned offset,std::uint64_t n) {
    for(unsigned i=0;i<8;++i) b[offset+i]=n>>(i*8);
}
static void f32(std::vector<std::uint8_t>& b,unsigned offset,float n) { std::uint32_t bits; std::memcpy(&bits,&n,4); put32(b,offset,bits); }
static void f64(std::vector<std::uint8_t>& b,unsigned offset,double n) { std::uint64_t bits; std::memcpy(&bits,&n,8); put64(b,offset,bits); }
int main(int argc,char** argv) {
    if(argc==4) {
        player::Asset asset; player::Pose pose;
        assert(player::readAsset(argv[1],asset) && player::readPose(argv[2],pose));
        assert(player::matches(asset,pose,7));
        assert(asset.vertices.size()==432 && asset.parts==12 && asset.skin.size()==64*64*4);
        // Every baked cube exports 6 faces / 12 triangles, independent of any camera.
        for(unsigned bone=0;bone<12;++bone) {
            unsigned count=0;
            for(const auto& v:asset.vertices) if(v.bone==bone) ++count;
            assert(count==36);
        }
        std::cout<<"PASS actual Minecraft baked geometry + animated matrices cross Java/native channel (432 vertices, 72 faces)\n";
        return 0;
    }
    assert(argc==3);
    std::vector<std::uint8_t> mesh(40+3*24+4);
    put32(mesh,0,0x4d504843); put32(mesh,4,1); put64(mesh,8,7); put64(mesh,16,2);
    put32(mesh,24,1); put32(mesh,28,3); put32(mesh,32,1); put32(mesh,36,1);
    mesh.back()=255; write(argv[1],mesh);
    player::Asset asset; assert(player::readAsset(argv[1],asset)); assert(asset.vertices.size()==3 && asset.skin.size()==4);
    std::vector<std::uint8_t> pose(132);
    put32(pose,0,0x50504843); put32(pose,4,1); put64(pose,8,7); put64(pose,16,2); put64(pose,24,1);
    f64(pose,32,-917.5); f64(pose,40,-60); f64(pose,48,348); put32(pose,56,1); put32(pose,64,1);
    for(unsigned i:{0u,5u,10u,15u}) f32(pose,68+i*4,1);
    write(argv[2],pose); player::Pose state; assert(player::readPose(argv[2],state)); assert(player::matches(asset,state,7));
    assert(!player::matches(asset,state,8)); state.asset=3; assert(!player::matches(asset,state,7)); state.asset=2;
    world::Freshness freshness;
    assert(freshness.observe(state.generation,state.sequence,100)==world::Freshness::Result::Warmup);
    assert(!freshness.live(100));
    assert(freshness.observe(state.generation,state.sequence+1,350)==world::Freshness::Result::Advanced);
    assert(freshness.live(350));
    assert(freshness.observe(state.generation,state.sequence+1,1000)==world::Freshness::Result::Duplicate);
    assert(!freshness.live(1350)); // Duplicate files cannot renew pose liveness.
    assert(freshness.observe(state.generation,state.sequence,1351)==world::Freshness::Result::Refused);
    alignment::Session session; session.observe({1,2,3},100);
    assert(session.calibrate(state.feet,{10,20,30},7,0)); reprojection::Matrix anchor;
    assert(alignment::worldMatrix(session.anchor,7,session.epoch,anchor));
    auto mapped=reprojection::transform(anchor,{state.feet[0],state.feet[1],state.feet[2],1});
    assert(mapped[0]==10 && mapped[1]==20 && mapped[2]==30);
    assert(!alignment::worldMatrix(session.anchor,8,session.epoch,anchor));
    // Malformed geometry must never replace an accepted asset.
    put32(mesh,40+20,1); write(argv[1],mesh); assert(!player::readAsset(argv[1],asset)); assert(asset.identity==2);
    put32(mesh,40+20,0); f32(mesh,40,std::numeric_limits<float>::quiet_NaN()); write(argv[1],mesh); assert(!player::readAsset(argv[1],asset));
    put32(pose,64,2); write(argv[2],pose); assert(!player::readPose(argv[2],state));
    put32(pose,64,1); f32(pose,68+15*4,0); write(argv[2],pose); assert(!player::readPose(argv[2],state));
    f32(pose,68+15*4,1); f64(pose,32,std::numeric_limits<double>::infinity()); write(argv[2],pose); assert(!player::readPose(argv[2],state));
    pose.pop_back(); write(argv[2],pose); assert(!player::readPose(argv[2],state));
    std::cout<<"PASS player asset/pose matching, negative feet anchor, restart refusal, bone/finite/affine/truncation bounds\n";
}
