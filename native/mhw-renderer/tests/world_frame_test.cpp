#include "world_frame.hpp"
#include "world_freshness.hpp"
#include "player_model.hpp"
#include <cassert>
#include <cstdio>
#include <filesystem>

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
    assert(frame.clipMapping==1 && frame.clipOrigin==1 && frame.cameraMode==1);
    assert(frame.cameraPosition[0]==917.546812345 && frame.viewRotation[15]==1);
    assert(frame.rangeMin==0 && frame.rangeMax==1 && frame.clearDepth==0);
    assert(!frame.playerKnown); // legacy v2 can be inspected, never player-calibrated
    assert(!frame.bodyExcluded);
    for(auto pixel:frame.colour) assert(pixel==1);
    for(auto pixel:frame.depth) assert(pixel==1);
    for(unsigned mode=0;mode<3;++mode) {
        const auto path=std::filesystem::path(argv[1]).parent_path()/ ("camera-"+std::to_string(mode)+".frame");
        assert(read(path.string().c_str(),frame) && frame.cameraMode==mode);
        const auto playerPath=std::filesystem::path(argv[1]).parent_path()/ ("player-"+std::to_string(mode)+".frame");
        assert(read(playerPath.string().c_str(),frame) && frame.playerKnown && frame.cameraMode==mode);
        assert(frame.playerPosition[0]==917.546812345 && frame.playerPosition[1]==-60
            && frame.playerPosition[2]==343.1757);
        assert(!frame.bodyExcluded);
        const auto separatePath=std::filesystem::path(argv[1]).parent_path()/ ("separate-"+std::to_string(mode)+".frame");
        assert(read(separatePath.string().c_str(),frame) && frame.bodyExcluded && frame.playerKnown);
        const auto capturedPath=std::filesystem::path(argv[1]).parent_path()/ ("capture-"+std::to_string(mode)+".frame");
        assert(read(capturedPath.string().c_str(),frame) && frame.cameraMode==mode && frame.bodyPose.size()==132);
        crafterhunter::player::Pose issued;
        assert(crafterhunter::player::capturedPose(frame,issued));
        assert(issued.sequence==17 && issued.asset==42 && issued.feet==frame.playerPosition);
        crafterhunter::player::Pose newest,selected;
        const auto livePath=std::filesystem::path(argv[1]).parent_path()/"latest-player.pose";
        assert(crafterhunter::player::readPose(livePath.string().c_str(),newest) && newest.sequence==18);
        unsigned reads=0;
        auto latest=[&](crafterhunter::player::Pose& p) { ++reads; p=newest; return true; };
        assert(crafterhunter::player::selectPose(frame,true,latest,selected));
        assert(reads==0 && selected.sequence==17 && selected.feet==frame.playerPosition);
        // Attachment pixels are at these older feet/bones; the live pose is five blocks ahead.
        assert(selected.feet[0]+selected.bones[0].matrix[12]==frame.playerPosition[0]+3);
        assert(crafterhunter::player::selectPose(frame,false,latest,selected));
        assert(reads==1 && selected.sequence==18);
        auto unavailable=frame; unavailable.bodyPose.clear();
        assert(!crafterhunter::player::selectPose(unavailable,true,latest,selected) && reads==1);
        auto wrongGeneration=frame; ++wrongGeneration.generation;
        assert(!crafterhunter::player::selectPose(wrongGeneration,true,latest,selected) && reads==1);
        crafterhunter::player::Asset asset;asset.generation=99;asset.identity=43;asset.parts=1;
        assert(!crafterhunter::player::matches(asset,issued,99));
        asset.identity=42;assert(crafterhunter::player::matches(asset,issued,99));
        const auto unfilteredPath=std::filesystem::path(argv[1]).parent_path()/ ("unfiltered-"+std::to_string(mode)+".frame");
        assert(read(unfilteredPath.string().c_str(),frame) && !frame.bodyExcluded && frame.bodyPose.empty());
    }
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
    auto bad=bytes; bad[4]=1; refuse(bad); // legacy version lacks render transform
    bad=bytes; bad[12]=0; refuse(bad); // unknown row order
    bad=bytes; bad[24]=1; refuse(bad); // wrong stride
    bad=bytes; bad[56]=1; refuse(bad); // attachment size disagreement
    bad=bytes; bad[40]=0; refuse(bad); // missing identity
    bad=bytes; bad[280]=1; refuse(bad); // reserved layout changed
    bad=bytes; bad[164]=0; refuse(bad); // unknown clip convention
    bad=bytes; bad[168]=0; refuse(bad); // unknown origin
    bad=bytes; bad[264]=3; refuse(bad); // invalid perspective
    bad=bytes; bad[276]=0; refuse(bad); // unknown provenance
    bad=bytes; bad.resize(300); refuse(bad); // incomplete depth
    bad=bytes; bad.push_back(0); refuse(bad); // trailing data
    const auto playerPath=std::filesystem::path(argv[1]).parent_path()/"player-0.frame";
    std::ifstream playerInput(playerPath,std::ios::binary);
    std::vector<char> playerBytes((std::istreambuf_iterator<char>(playerInput)),{});
    bad=playerBytes; bad[304]=0; refuse(bad); // player provenance missing
    bad=playerBytes; bad[308]=1; refuse(bad); // v3 reserved fields stay zero
    bad=playerBytes; bad[4]=2; refuse(bad); // player fields cannot masquerade as v2
    bad=playerBytes; bad[286]=char(0xf8); bad[287]=char(0x7f); refuse(bad); // NaN feet
    bad=playerBytes; bad[12]=7; refuse(bad); // unknown v4 flags
    bad=playerBytes; bad[12]=3; bad[4]=3; refuse(bad); // v3 cannot claim body exclusion
    bad=bytes; bad[12]=3; refuse(bad); // v2 cannot claim body exclusion
    const auto capturedPath=std::filesystem::path(argv[1]).parent_path()/"capture-0.frame";
    std::ifstream capturedInput(capturedPath,std::ios::binary);
    std::vector<char> capturedBytes((std::istreambuf_iterator<char>(capturedInput)),{});
    bad=capturedBytes; bad[308]=0; refuse(bad); // size and exclusion must agree
    bad=capturedBytes; bad[312]=1; refuse(bad); // reserved bytes
    bad=capturedBytes; bad[12]=1; refuse(bad); // hidden body pose cannot accompany unfiltered capture
    bad=capturedBytes; bad[584]=100; refuse(bad); // another generation in embedded pose
    bad=capturedBytes; bad[592]=0; refuse(bad); // missing asset
    bad=capturedBytes; bad[600]=0; refuse(bad); // missing pose sequence
    bad=capturedBytes; bad[632]=65; refuse(bad); // too many bones
    bad=capturedBytes; bad[640]=2; refuse(bad); // unsupported visibility
    bad=capturedBytes; bad[656]=1; refuse(bad); // nonaffine body pose
    bad=capturedBytes; bad.pop_back(); refuse(bad); // incomplete pose
    bad=capturedBytes; bad.push_back(0); refuse(bad); // trailing bytes
    // Retain actual v3 archive compatibility, without claiming exclusion.
    bad=playerBytes; bad[4]=3;
    { std::ofstream out(rejected,std::ios::binary); out.write(bad.data(),bad.size()); }
    assert(read(rejected.c_str(),frame) && frame.playerKnown && !frame.bodyExcluded);
    std::puts("PASS: v2/v3/v4/v5 Java/native round trip, issue-time pose, all F5 modes, resize and malformed-frame refusals");
}
