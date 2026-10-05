// Whether a Minecraft pixel is drawn, and how, given MHW's depth at that pixel.
//
// Deliberately free of D3D types so it runs off-target, like selection.hpp. The
// rules here are the difference between "Minecraft is visible in MHW" and
// "Minecraft is a flat rectangle pasted over MHW", so they are worth testing
// rather than burying in a shader.
//
// Two facts about MHW's frame come from the resource trace and the existing
// stone shader, and both are load-bearing:
//
//   1. Scene depth is reversed Z and clears to 0. Under reversed Z a *larger*
//      value is nearer, so "is the Minecraft geometry in front of the host
//      geometry" is `mcDepth > hostDepth`, not `<`. Getting this backwards draws
//      Steve through walls, which is the failure this whole comparison exists to
//      prevent.
//   2. A host pixel that no geometry ever touched still holds the clear value,
//      which is 0 — the *far* end of the range. So host depth 0 means sky, and
//      Minecraft must be drawn there rather than discarded, or the sky behind a
//      Minecraft building would punch a hole in the world.
#pragma once

#include <cstdint>

namespace crafterhunter::composite {

// Reversed Z: nearer is larger. The epsilon absorbs the fact that a Minecraft
// pixel and a host pixel rarely describe the same surface at exactly the same
// depth, and without it coplanar geometry flickers between drawn and discarded
// as the camera moves.
inline constexpr float DepthEpsilon = 1e-6f;

// What a single pixel's two depths mean for compositing.
enum class Decision {
    Draw,        // Minecraft geometry here, in front of (or in open air above) MHW's
    Skip,        // MHW's own geometry is in front; Minecraft must not cover it
    Unusable,    // no frame, no depth, or a value that cannot be compared
};

struct PixelView {
    float hostDepth;      // MHW's scene depth, reversed Z
    bool  hostHasDepth;   // false where the host pass never wrote depth
    float minecraftDepth; // Minecraft's own linear eye depth
    bool  minecraftHasPixel; // false where Minecraft drew nothing (its sky)
    bool  minecraftHasDepth; // false when Minecraft has colour but no depth to compare
};

// The rule, once per pixel.
inline Decision decide(const PixelView& pixel) {
    if (!pixel.minecraftHasPixel) return Decision::Unusable;
    // No Minecraft depth to compare: Minecraft has colour but the host cannot
    // tell how far away it is. Drawing it unconditionally would paste Minecraft
    // over MHW's characters; refusing it loses the frame entirely. Refuse.
    if (!pixel.minecraftHasDepth) return Decision::Skip;

    // Nothing of MHW's own is here, so draw: this is the sky case, and it is the
    // common one for a full-screen frame.
    if (!pixel.hostHasDepth) return Decision::Draw;

    // Reversed Z, so nearer is the larger value. The epsilon keeps two surfaces
    // that are genuinely coplanar from flickering.
    return pixel.minecraftDepth + DepthEpsilon > pixel.hostDepth ? Decision::Draw : Decision::Skip;
}

// The interim rule, for a transport that carries Minecraft colour but no depth.
//
// `decide()` refuses colour-without-depth because pasting it unconditionally
// would cover MHW's hunter with a rectangle. That objection is about
// *unconditional* pasting. Restricted to host pixels holding no geometry at all,
// the same frame draws only where MHW drew nothing and covers nothing, so this is
// the pinned rule with the depth comparison unavailable rather than a loosened
// version of it.
//
// What this costs, stated plainly: Minecraft is visible against MHW's sky and
// is NOT occluded by MHW's terrain. A tree between Steve and the camera will not
// hide him. Full occlusion needs Minecraft's own depth in the transport, which
// means a second attachment and a linearisation, and is its own piece of work
// rather than a shader tweak.
inline Decision decideSkyOnly(const PixelView& pixel) {
    if (!pixel.minecraftHasPixel) return Decision::Unusable;
    if (pixel.hostHasDepth) return Decision::Skip;
    return Decision::Draw;
}

// Where the guest's frame sits inside the host's, in the uv the shader samples.
//
// Extracted from renderer.cpp so it can be checked without a GPU. Worth extracting
// because a subtly wrong letterbox does not look wrong: it looks like Minecraft being
// drawn in the wrong place, which is indistinguishable from the UV mapping being wrong
// until it is measured.
//
// Minecraft is fitted inside MHW rather than stretched to fill it. Stretching to fit
// would shear Steve and, worse, shear the depth being compared against MHW's own - a
// comparison against a resampled depth is not a comparison at all. Fitting keeps both
// axes at one scale and centres the result.
struct Letterbox {
    float scaleX;  // fraction of the host's width the guest occupies
    float scaleY;  // fraction of the host's height it occupies
    float offsetX; // where that fraction starts, from the left
    float offsetY; // where it starts, from the top
};

// The shader's uv for a host pixel, given hostUv in [0,1] on both axes. SV_Position
// has its origin at the top-left and the guest's frame is stored top-down, so the V
// axis needs no flip here: the row flip is the guest's job, and doing it twice is how a
// frame ends up mirrored.
// Never larger than the whole host, and never negative.
inline float clampToHost(float fraction) {
    if (fraction < 0.0f) return 0.0f;
    return fraction > 1.0f ? 1.0f : fraction;
}

inline Letterbox letterbox(float hostWidth, float hostHeight,
                           float guestWidth, float guestHeight) {
    Letterbox box{0.0f, 0.0f, 0.0f, 0.0f};
    // A zero guest dimension would divide by zero and produce a NaN that reaches the
    // shader as either a silently discarded or a fully-drawn frame. Reported as a
    // degenerate zero rectangle, which discards everything - visibly absent rather
    // than invisibly wrong.
    if (hostWidth <= 0.0f || hostHeight <= 0.0f) return box;
    if (guestWidth <= 0.0f || guestHeight <= 0.0f) return box;

    const float fitX = hostWidth / guestWidth;
    const float fitY = hostHeight / guestHeight;
    // One scale for both axes: this is what keeps the aspect ratio.
    const float fit = fitX < fitY ? fitX : fitY;
    // Clamped to the host. On the tight axis the arithmetic lands just above 1 - for
    // 1908 into 1920 it comes out at 1.0000003 - and an unclamped scale above 1 makes
    // the offset negative, which maps the host's own edge column to a uv below zero and
    // discards it. One pixel of nothing, but it means the rectangle is very slightly
    // larger than the thing it is supposed to fit inside.
    box.scaleX = clampToHost((guestWidth * fit) / hostWidth);
    box.scaleY = clampToHost((guestHeight * fit) / hostHeight);
    box.offsetX = (1.0f - box.scaleX) * 0.5f;
    box.offsetY = (1.0f - box.scaleY) * 0.5f;
    return box;
}

// The shader's uv for one host pixel.
//
// The direction of this mapping is the whole subtlety, and it was wrong once. To draw
// a rectangle occupying `scale` of the screen and starting at `offset`, the screen
// coordinate must be brought *into* that rectangle:
//
//     guest = (host - offset) / scale
//
// Scaling the screen coordinate instead - `guest = host * scale + offset` - inverts the
// relationship. The region drawn becomes host in [-offset/scale, (1-offset)/scale],
// which for 1908x1028 into 1920x1080 is [-0.022, 1.022] vertically: larger than the
// screen at both ends. Minecraft was therefore not letterboxed at all. It was cropped -
// about 2% lost off the top and the bottom - and stretched to fill all 1920x1080, which
// is the single thing this function exists to prevent.
//
// SV_Position has its origin at the top-left and the guest's frame is stored top-down,
// so the V axis needs no flip here: the row flip is the guest's job, and doing it twice
// is how a frame ends up mirrored.
inline void mapToGuest(const Letterbox& box, float hostU, float hostV,
                       float& guestU, float& guestV) {
    // A degenerate rectangle has no inverse. Reported as (0,0) rather than a division
    // by zero; insideGuest rejects it before reaching here.
    if (box.scaleX <= 0.0f || box.scaleY <= 0.0f) {
        guestU = 0.0f;
        guestV = 0.0f;
        return;
    }
    guestU = (hostU - box.offsetX) / box.scaleX;
    guestV = (hostV - box.offsetY) / box.scaleY;
}

// Is this host pixel inside the guest's rectangle? The shader discards anything
// outside, and the test is on the mapped uv rather than on host coordinates so the two
// can never disagree about where the edges are.
inline bool insideGuest(const Letterbox& box, float hostU, float hostV) {
    // A degenerate rectangle maps every host pixel to uv (0,0), which is *inside*
    // [0,1] - so without this it would draw one stray texel from the corner of the
    // frame at the centre of the screen rather than nothing. Caught by the test
    // asserting that a zero-sized guest discards everything.
    if (box.scaleX <= 0.0f || box.scaleY <= 0.0f) return false;
    float guestU = 0.0f;
    float guestV = 0.0f;
    mapToGuest(box, hostU, hostV, guestU, guestV);
    return !(guestU < 0.0f || guestU > 1.0f || guestV < 0.0f || guestV > 1.0f);
}

// Fraction of the frame worth uploading at all, given a coverage measurement.
// Minecraft's own sky is a large fraction of most frames and carries no
// information for MHW: it is Minecraft's sky, not the world's, and drawing it
// would replace MHW's sky with a flat gradient. So the transport may hand over
// a whole frame, but the composite reports how much of it is Minecraft at all.
inline bool worthCompositing(float minecraftCoverage) {
    // Below half a percent there is no Steve in the frame worth the bandwidth.
    return minecraftCoverage >= 0.005f;
}

}  // namespace crafterhunter::composite
