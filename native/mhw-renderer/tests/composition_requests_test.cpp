#include "composition_requests.hpp"
#include <cassert>
#include <cstdio>
#include <string_view>

int main() {
    using namespace crafterhunter::composition;
    FrameRequests cache;
    Requests files;
    unsigned queries=0;
    auto exists=[&](std::string_view name) {
        ++queries;
        if(name=="paired-full.enabled") return files.full;
        if(name=="paired-compose.enabled") return files.preview;
        assert(name=="player-compose.enabled");
        return files.player;
    };
    assert(!cache.value().full && !cache.value().preview && !cache.value().player);
    // Reproduce the expensive case: missing flags and a draw that keeps
    // refusing. Every eligible callback must observe the cached OFF state.
    for(unsigned callback=0;callback<1000;++callback) {
        const auto& requests=cache.sample(1,exists);
        assert(!requests.full && !requests.preview && !requests.player);
    }
    assert(queries==3); // original per-callback polling would issue 3,000 queries
    // A complete atomic flag replacement is picked up on the next frame, never
    // a different choice halfway through one frame.
    files={true,true,true};
    const auto& old=cache.sample(1,exists);
    assert(!old.full && !old.preview && !old.player && queries==3);
    const auto& next=cache.sample(2,exists);
    assert(next.full && next.preview && next.player && queries==6);
    files={};
    for(unsigned callback=0;callback<1000;++callback) {
        const auto& requests=cache.sample(2,exists);
        assert(requests.full && requests.preview && requests.player);
    }
    assert(queries==6);
    // All combinations remain independent; full/preview precedence belongs to
    // the existing dispatch, and player enablement cannot gate either one.
    for(unsigned mask=0;mask<8;++mask) {
        files={bool(mask&1),bool(mask&2),bool(mask&4)};
        const auto& requests=cache.sample(mask+3,exists);
        assert(requests.full==files.full && requests.preview==files.preview && requests.player==files.player);
    }
    assert(queries==30);
    cache.reset();
    assert(!cache.value().full && !cache.value().preview && !cache.value().player);
    files={};
    cache.sample(10,exists); // stop/restart may reuse the same frame number
    assert(queries==33 && !cache.value().player);
    files.player=true;
    cache.sample(0,exists); // frame wrap/rollback must still resample
    assert(queries==36 && cache.value().player);
    std::puts("PASS: 1,000 refusing callbacks query flags only once per frame; next-frame toggles, independent modes and reset");
}
