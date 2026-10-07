#include "world_frame.hpp"
#include <cassert>
#include <cstdio>

int main(int argc, char** argv) {
    assert(argc==3);
    using namespace crafterhunter::world;
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
