// Off-target unit tests for the per-pixel compositing rule.
//
// The rule that matters most is the reversed-Z comparison, and it is exactly the
// kind of thing that looks right and is backwards: drawing Steve through MHW's
// walls is a plausible-looking image, which is why it survived a visual check
// in an earlier milestone's design notes and had to be pinned here instead.
#include "frame_composite.hpp"

#include <cstdio>

using namespace crafterhunter::composite;

namespace {

int failures = 0;

void check(bool condition, const char* what) {
    if (!condition) {
        std::fprintf(stderr, "FAILED: %s\n", what);
        ++failures;
    }
}

PixelView pixel(float host, bool hasHost, float mine, bool hasMine, bool hasMineDepth = true) {
    return PixelView{host, hasHost, mine, hasMine, hasMineDepth};
}

}  // namespace

int main() {
    // Sky: the host pass wrote no depth there. Minecraft must be drawn, or a
    // Minecraft building against the sky would punch a hole in the world.
    check(decide(pixel(0.0f, false, 0.5f, true)) == Decision::Draw,
          "Minecraft draws where MHW has no geometry");

    // Reversed Z: nearer is the LARGER value. Minecraft nearer than MHW draws;
    // Minecraft behind MHW does not. Both directions are checked, because only
    // one of them fails if the comparison is written backwards.
    check(decide(pixel(0.2f, true, 0.8f, true)) == Decision::Draw,
          "Minecraft nearer than MHW draws (reversed Z: larger is nearer)");
    check(decide(pixel(0.8f, true, 0.2f, true)) == Decision::Skip,
          "Minecraft behind MHW is skipped, not drawn through it");

    // Coplanar surfaces must not flicker between the two.
    check(decide(pixel(0.5f, true, 0.5f, true)) == Decision::Draw,
          "coplanar surfaces draw rather than flickering on the epsilon");

    // The extremes, because 0 is the clear value and 1 is the near plane.
    check(decide(pixel(0.0f, true, 1.0f, true)) == Decision::Draw,
          "Minecraft against the far plane draws");
    check(decide(pixel(1.0f, true, 0.0f, true)) == Decision::Skip,
          "Minecraft behind the near plane is skipped");

    // No Minecraft pixel at all: nothing to draw, and nothing to blame.
    check(decide(pixel(0.0f, false, 0.0f, false)) == Decision::Unusable,
          "an empty Minecraft pixel is unusable, not a host-only frame");
    check(decide(pixel(0.5f, true, 0.5f, false)) == Decision::Unusable,
          "Minecraft's own sky is never drawn over MHW");

    // Minecraft colour without a depth to compare. Drawing it unconditionally
    // would paste Minecraft over MHW's characters; refusing it keeps the host
    // frame intact, and the loss is visible as missing Minecraft rather than as
    // a Steve in front of a monster.
    check(decide(pixel(0.0f, false, 0.0f, true, false)) == Decision::Skip,
          "Minecraft colour with no depth is refused rather than pasted");

    // Coverage: a frame that is almost entirely Minecraft's own sky carries
    // nothing worth compositing.
    check(!worthCompositing(0.0f), "an empty frame is not worth compositing");
    check(!worthCompositing(0.004f), "a frame with no visible Minecraft is not worth it");
    check(worthCompositing(0.005f), "half a percent of Minecraft is the threshold");
    check(worthCompositing(0.5f), "half a frame is worth compositing");
    check(worthCompositing(1.0f), "a full frame is worth compositing");

    if (failures == 0) {
        std::printf(
            "Frame composite checks passed: sky, reversed-Z both directions, coplanar, extremes, "
            "empty Minecraft sky, depthless colour, coverage threshold.\n");
        return 0;
    }
    std::fprintf(stderr, "%d frame composite check(s) failed\n", failures);
    return 1;
}
