// Depth-candidate selection policy.
//
// Deliberately free of D3D types so it can be unit-tested off-target with
// tools/test-depth-selection.sh; renderer.cpp only fills CandidateView from
// what it already knows about each resource.
//
// Two conditions, not one. Measured 2026-10-05 (captures 14757842 and
// 2089775): two candidates were fresh in the same frame while one of them read
// back 0.00 % covered, so "pick whichever depth was cleared this frame" would
// have bound an all-zero buffer. Under reversed Z that makes
// `depth + epsilon < 0` never true, so nothing is ever discarded and the stone
// draws through everything in front of it - the exact failure this path exists
// to prevent. Content therefore has to be measured, not assumed, and an
// unmeasured candidate is treated as unusable rather than as a default choice.
#pragma once

#include <cstddef>

namespace crafterhunter::depth {

// A candidate described only by facts the renderer already has, plus the last
// measured coverage. No handles, so the same struct works in a test.
struct CandidateView {
    float clear;                  // value the game clears this target with (scene depth is 0)
    unsigned long long lastFrame; // frame the resource was last cleared on
    unsigned long long frame;     // frame currently being composed
    bool targetMatches;           // R32_TYPELESS and the same size as the backbuffer
    bool contentKnown;            // a read-back has completed for this candidate
    float covered;                // fraction of sampled pixels holding geometry
};

// At least 2 % of sampled pixels must hold geometry. Erring strict is the safe
// direction: too strict and the stone disappears (fail closed), too lenient and
// an empty buffer is bound (draw-through).
inline constexpr float ContentThreshold = 0.02f;
// Cleared on this frame or the one before it. The scene depth is cleared every
// frame, so anything older was not part of the frame being composed.
inline constexpr unsigned long long FreshFrames = 1;
// How often a candidate already known to hold content is re-measured. Content
// varies by area - depth2 held 4.47 % in one area and 0.00 % in another - so
// a decision taken once at discovery cannot be trusted forever.
inline constexpr unsigned long long ContentRecheckFrames = 300;
inline constexpr std::size_t NotFound = static_cast<std::size_t>(-1);

inline bool eligible(const CandidateView& candidate) {
    if (!candidate.targetMatches) return false;
    if (candidate.clear != 0.0f) return false;                     // reversed-Z scene depth clears to 0
    if (candidate.frame - candidate.lastFrame > FreshFrames) return false;
    if (!candidate.contentKnown) return false;                     // unmeasured: fail closed
    return candidate.covered >= ContentThreshold;
}

// First eligible candidate in discovery order; NotFound when none qualifies,
// which the renderer reads as "skip composition for this frame".
inline std::size_t select(const CandidateView* candidates, std::size_t count) {
    for (std::size_t i = 0; i < count; ++i)
        if (eligible(candidates[i])) return i;
    return NotFound;
}

// Fraction of sampled pixels holding geometry. Reversed Z writes (0, 1] for
// geometry and leaves the clear value 0 for anything the pass never touched,
// so "greater than zero" is the content test. The grid spans the whole target
// so a frame that is mostly sky still counts as content-bearing.
template <typename Read>
inline float coverage(Read read, unsigned width, unsigned height) {
    const unsigned step = width && height
        ? (width < height ? width : height) / 54u
        : 0u;
    const unsigned stride = step ? step : 1u;
    unsigned hits = 0, total = 0;
    for (unsigned y = 0; y < height; y += stride)
        for (unsigned x = 0; x < width; x += stride) {
            ++total;
            if (read(x, y) > 0.0f) ++hits;
        }
    return total ? static_cast<float>(hits) / static_cast<float>(total) : 0.0f;
}

}  // namespace crafterhunter::depth
