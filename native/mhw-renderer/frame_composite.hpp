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
