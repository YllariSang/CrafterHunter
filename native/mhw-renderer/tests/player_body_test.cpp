#include "player_body.hpp"
#include <cassert>
#include <cstdio>

int main() {
    using namespace crafterhunter::player;
    assert(bodyDepth(true,false,true)==BodyDepth::Hidden); // old composite already contains Steve
    assert(bodyDepth(true,false,false)==BodyDepth::Hidden);
    assert(bodyDepth(true,true,false)==BodyDepth::Hidden); // do not draw over absent/stale guest depth
    assert(bodyDepth(true,true,true)==BodyDepth::SharedGuest);
    assert(bodyDepth(false,false,false)==BodyDepth::Private); // standalone model path
    assert(bodyDepth(false,true,true)==BodyDepth::Private);
    GuestDepthStamp stamp{10,20,30,40,50,60};
    assert(stamp.matches(stamp));
    auto other=stamp; other.frame++; assert(!stamp.matches(other));
    other=stamp; other.generation++; assert(!stamp.matches(other));
    other=stamp; other.identity++; assert(!stamp.matches(other));
    other=stamp; other.epoch++; assert(!stamp.matches(other));
    other=stamp; other.target++; assert(!stamp.matches(other));
    other=stamp; other.camera++; assert(!stamp.matches(other));
    assert(!GuestDepthStamp{}.matches(GuestDepthStamp{}));
    // Fixed same-pixel reversed-Z oracle: world/held item must remain in front.
    // Production keeps this depth rather than clearing a new player-only DSV.
    auto visible=[](float model,float guest,float host) { return model>guest && model>host+0.000001f; };
    assert(!visible(.6f,.7f,0)); // Minecraft block covers Steve
    assert(visible(.8f,.7f,0)); // Steve in front of block
    assert(!visible(.8f,.7f,.9f)); // MHW foreground covers both
    assert(!visible(.8f,.81f,0)); // held item/armor stays in front of body
    assert(visible(.6f,0,0)); // clearing guest depth recreates the original occlusion defect
    std::puts("PASS single body selection, exact-frame guest depth and block/item/MHW occlusion ordering");
}
