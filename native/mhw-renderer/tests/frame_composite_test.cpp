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

// Regression: the letterbox, checked against the live sizes.
//
// Minecraft publishes 1908x1028 and MHW renders 1920x1080. The frame is fitted inside
// the host rather than stretched, so these properties are what make the placement
// correct rather than merely plausible: one scale on both axes, inset and centred,
// never larger than the host, and host pixels outside the rectangle mapping outside
// [0,1] so the shader's discard catches them.
//
// The mapping direction is the part that was wrong. Scaling the screen coordinate -
// `guest = host * scale + offset` - inverts the relationship and makes the drawn region
// *larger* than the screen, so Minecraft was cropped and stretched to fill all
// 1920x1080. Brought into the rectangle instead, `guest = (host - offset) / scale`, the
// region's edges are the rectangle's own and the bars appear.
//
// The test lives here rather than beside the shader because the shader cannot be run
// headlessly, and an unchecked letterbox produces an image that looks like a UV bug
// whether or not it is one.
void checkLetterbox() {
    constexpr float hostW = 1920.0f;
    constexpr float hostH = 1080.0f;
    constexpr float guestW = 1908.0f;
    constexpr float guestH = 1028.0f;

    const Letterbox box = letterbox(hostW, hostH, guestW, guestH);

    // 1908/1920 = 0.99375 against 1028/1080 = 0.95185, so height is the tight axis and
    // the rectangle spans the host's full width.
    check(guestH / hostH < guestW / hostW, "height is the tight axis for these sizes");
    check(box.scaleX > box.scaleY, "so the rectangle is wider than it is tall");
    check(box.offsetX > -0.0001f && box.offsetX < 0.0001f,
          "with no horizontal bar, because the guest fills the width");
    check(box.offsetY > 0.0f, "and a real vertical bar above");

    // The bars are equal, which is what centred means.
    const float barTop = box.offsetY;
    const float barBottom = (1.0f - box.scaleY) - box.offsetY;
    check(barTop > 0.02f && barTop < 0.03f,
          "the vertical bar is about 23 px of 1080, which is what 1908x1028 gives");
    check(barTop - barBottom > -0.0001f && barTop - barBottom < 0.0001f,
          "and the same below, so the rectangle is centred");
    check(barTop * hostH > 20.0f && barTop * hostH < 26.0f, "roughly 23 rows of bar");

    // Never larger than the host on either axis.
    check(box.scaleX <= 1.0f, "the rectangle never exceeds the host's width");
    check(box.scaleY <= 1.0f, "and never exceeds its height");

    // The rectangle's own corners map exactly onto the frame's corners.
    float guestU = 0.0f;
    float guestV = 0.0f;
    mapToGuest(box, box.offsetX, box.offsetY, guestU, guestV);
    check(guestU > -0.0001f && guestU < 0.0001f, "the rectangle's origin is the frame's (0,0)");
    check(guestV > -0.0001f && guestV < 0.0001f, "on both axes");
    mapToGuest(box, box.offsetX + box.scaleX, box.offsetY + box.scaleY, guestU, guestV);
    check(guestU > 0.9999f && guestU < 1.0001f, "and its far corner is the frame's (1,1)");
    check(guestV > 0.9999f && guestV < 1.0001f, "on both axes");

    // The centre of the rectangle samples the centre of the frame, which is what a
    // scaled-and-offset mapping gets wrong.
    mapToGuest(box, box.offsetX + box.scaleX * 0.5f, box.offsetY + box.scaleY * 0.5f,
               guestU, guestV);
    check(guestU > 0.4999f && guestU < 0.5001f,
          "the centre of the rectangle is the centre of the frame");
    check(guestV > 0.4999f && guestV < 0.5001f, "on both axes");

    // Everything outside the rectangle maps outside [0,1], which is what the shader's
    // discard acts on. The host's own corners are outside - that is the whole point of
    // a letterbox, and the assertion the inverted mapping failed.
    check(!insideGuest(box, 0.0f, 0.0f), "the host's top-left corner is outside the rectangle");
    check(!insideGuest(box, 1.0f, 1.0f), "and so is its bottom-right corner");
    check(!insideGuest(box, 0.5f, box.offsetY * 0.5f), "the middle of the top bar is outside");
    check(!insideGuest(box, 0.5f, 1.0f - box.offsetY * 0.5f), "and the middle of the bottom bar");

    // The host's left and right edges are *inside*, because for these sizes there is no
    // side bar: the guest is wider relative to the host than it is tall, so the frame
    // spans the full width. Asserted because it is easy to get backwards - an earlier
    // version of this test asserted they were outside, which holds only when a bar
    // exists on that axis.
    check(insideGuest(box, 0.0f, 0.5f), "the left edge is inside, since there is no side bar");
    check(insideGuest(box, 1.0f, 0.5f), "and so is the right edge");
    float leftU = 0.0f;
    float leftV = 0.0f;
    mapToGuest(box, 0.0f, 0.5f, leftU, leftV);
    check(leftU > -0.0001f && leftU < 0.0001f,
          "and the left edge samples the frame's own left column");

    // The screen centre is inside, because the rectangle is centred on it.
    check(insideGuest(box, 0.5f, 0.5f), "the middle of the screen is inside the rectangle");

    // Just inside the rectangle's edges, and just outside them. These are the vertical
    // edges, so the varying coordinate is the second one.
    const float epsilon = 0.0005f;
    check(insideGuest(box, 0.5f, box.offsetY + box.scaleY * 0.5f),
          "the middle of the rectangle's height is inside");
    check(!insideGuest(box, 0.5f, box.offsetY - epsilon), "a step above its top edge is outside");
    check(!insideGuest(box, 0.5f, box.offsetY + box.scaleY + epsilon),
          "and a step below its bottom edge");

    // Aspect ratio preserved: one scale on both axes. This matters more than it looks,
    // because a stretched Steve would also invalidate the depth comparison the
    // composite depends on.
    const float drawnRatio = (box.scaleX * hostW) / (box.scaleY * hostH);
    const float guestRatio = guestW / guestH;
    check(drawnRatio > guestRatio - 0.001f && drawnRatio < guestRatio + 0.001f,
          "the drawn rectangle keeps the guest's aspect ratio");

    // Degenerate inputs must not produce a NaN that reaches the shader.
    const Letterbox noGuest = letterbox(hostW, hostH, 0.0f, guestH);
    check(noGuest.scaleX == 0.0f && noGuest.offsetX == 0.0f,
          "a zero guest width gives a degenerate rectangle, not a NaN");
    check(!insideGuest(noGuest, 0.5f, 0.5f),
          "which discards everything rather than drawing one stray texel");
    mapToGuest(noGuest, 0.5f, 0.5f, guestU, guestV);
    check(guestU == 0.0f && guestV == 0.0f, "and mapping it does not divide by zero");
    const Letterbox noHost = letterbox(0.0f, hostH, guestW, guestH);
    check(noHost.scaleX == 0.0f && noHost.scaleY == 0.0f, "a zero host dimension is handled too");

    // Identical sizes fill the host exactly, with no bars and no offset.
    const Letterbox identical = letterbox(guestW, guestH, guestW, guestH);
    check(identical.scaleX > 0.9999f && identical.scaleX < 1.0001f,
          "identical sizes fill the host exactly");
    check(identical.scaleY > 0.9999f && identical.scaleY < 1.0001f, "on both axes");
    check(identical.offsetX == 0.0f && identical.offsetY == 0.0f, "with no bars at all");
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

    // The interim rule used against the current transport, which carries colour
    // and no depth. It must draw against MHW's sky and refuse everywhere MHW has
    // geometry of its own, and the two halves are checked separately: a rule that
    // simply returned Draw would satisfy the first and cover the hunter with the
    // second.
    check(decideSkyOnly(pixel(0.0f, false, 0.0f, true, false)) == Decision::Draw,
          "sky-only draws Minecraft where MHW has no geometry");
    check(decideSkyOnly(pixel(0.9f, true, 0.9f, true, false)) == Decision::Skip,
          "sky-only refuses where MHW has geometry, whatever Minecraft's depth");
    check(decideSkyOnly(pixel(0.0f, true, 0.0f, true, false)) == Decision::Skip,
          "sky-only refuses even at the far plane rather than covering the host");
    check(decideSkyOnly(pixel(0.0f, false, 0.0f, false, false)) == Decision::Unusable,
          "sky-only still refuses a frame with no Minecraft pixel at all");
    // The interim rule must be strictly more conservative than the full one
    // wherever the full rule would skip, or it would paper over the occlusion
    // the full rule exists to provide. Minecraft behind MHW: both refuse.
    check(decide(pixel(0.9f, true, 0.2f, true, true)) == Decision::Skip &&
              decideSkyOnly(pixel(0.9f, true, 0.2f, true, true)) == Decision::Skip,
          "sky-only skips everything the full rule skips");
    // The gap, stated as a test: a nearer Minecraft pixel over MHW's geometry is
    // exactly the case that needs Minecraft's own depth, and the interim rule
    // cannot reach it. If this ever starts passing, depth has arrived.
    check(decide(pixel(0.2f, true, 0.8f, true, true)) == Decision::Draw &&
              decideSkyOnly(pixel(0.2f, true, 0.8f, true, true)) == Decision::Skip,
          "the full rule draws a nearer Minecraft pixel that sky-only cannot");

    // Coverage: a frame that is almost entirely Minecraft's own sky carries
    // nothing worth compositing.
    check(!worthCompositing(0.0f), "an empty frame is not worth compositing");
    check(!worthCompositing(0.004f), "a frame with no visible Minecraft is not worth it");
    check(worthCompositing(0.005f), "half a percent of Minecraft is the threshold");
    check(worthCompositing(0.5f), "half a frame is worth compositing");
    check(worthCompositing(1.0f), "a full frame is worth compositing");

    checkLetterbox();

    if (failures == 0) {
        std::printf(
            "Frame composite checks passed: sky, reversed-Z both directions, coplanar, extremes, "
            "empty Minecraft sky, depthless colour, coverage threshold, letterbox placement, "
            "aspect, discard bounds, degenerate sizes.\n");
        return 0;
    }
    std::fprintf(stderr, "%d frame composite check(s) failed\n", failures);
    return 1;
}
