#include "world_frame.hpp"
#include "world_freshness.hpp"
#include <cassert>
#include <cstdio>

int main(int argc, char** argv) {
    assert(argc==3);
    using namespace crafterhunter::world;
    Freshness clock;
    using R=Freshness::Result;
    assert(clock.observe(99,1,100)==R::Warmup && !clock.live(100));
    assert(clock.observe(99,1,200)==R::Duplicate && !clock.live(200));
    assert(clock.observe(99,2,300)==R::Advanced && clock.live(300));
    assert(clock.observe(99,2,1200)==R::Duplicate && clock.live(1200));
    assert(!clock.live(1300)); // duplicate must not refresh the watchdog
    assert(clock.observe(99,1,1400)==R::Refused && !clock.live(1400));
    assert(clock.observe(99,3,1500)==R::Advanced && clock.live(1500));
    assert(!clock.live(1499)); // local clock rollback fails closed
    assert(clock.observe(100,1,1600)==R::Warmup && !clock.live(1600));
    assert(clock.observe(99,4,1700)==R::Refused); // retired producer cannot return
    assert(clock.observe(100,2,1800)==R::Advanced && clock.live(1800));
    for(unsigned g=101; g<=103; ++g) assert(clock.observe(g,1,1900)==R::Warmup);
    assert(clock.observe(104,1,2000)==R::Refused && !clock.live(2000));
    std::puts("PASS: startup/restart warmup, advance, duplicate expiry, rollback, retired generations and bounded exhaustion");
    Snapshot frame;
    assert(read(argv[1],frame));
    assert(frame.width==8 && frame.height==4 && frame.stride==32);
    assert(frame.generation==99 && frame.identity==1 && frame.issueNanos==1234);
    assert(frame.nearPlane==0.05f && frame.farPlane==1024);
    assert(frame.projection[11]==-1 && frame.projection[14]==0.05000244f);
    assert(frame.pose[0]==917.5f && frame.pose[3]==9.75f);
    for(auto pixel:frame.colour) assert(pixel==1);
    for(auto pixel:frame.depth) assert(pixel==1);
    assert(read(argv[2],frame));
    assert(frame.width==4 && frame.generation==100 && frame.identity==1);
    std::ifstream input(argv[1],std::ios::binary);
    std::vector<char> bytes((std::istreambuf_iterator<char>(input)),{});
    const auto rejected=std::string(argv[1])+".rejected";
    auto refuse=[&](const std::vector<char>& data) {
        { std::ofstream out(rejected,std::ios::binary); out.write(data.data(),data.size()); }
        assert(!read(rejected.c_str(),frame));
        assert(frame.generation==100); // failed read cannot replace good output
    };
    auto bad=bytes; bad[4]=2; refuse(bad); // unknown version
    bad=bytes; bad[12]=0; refuse(bad); // unknown row order
    bad=bytes; bad[24]=1; refuse(bad); // wrong stride
    bad=bytes; bad[56]=1; refuse(bad); // attachment size disagreement
    bad=bytes; bad[40]=0; refuse(bad); // missing identity
    bad=bytes; bad[164]=1; refuse(bad); // reserved layout changed
    bad=bytes; bad.resize(300); refuse(bad); // incomplete depth
    bad=bytes; bad.push_back(0); refuse(bad); // trailing data
    std::puts("PASS: Java/native round trip, metadata, resize, eight malformed-frame refusals");
}
