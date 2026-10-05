// Off-target unit tests for the depth-candidate selection policy.
// Built and run by tools/test-depth-selection.sh; no D3D headers involved.
#include "selection.hpp"

#include <cstdio>
#include <vector>

namespace depth = crafterhunter::depth;

namespace {
int failures = 0;

void check(bool ok, const char* what) {
    if (!ok) { ++failures; std::printf("  FAIL: %s\n", what); }
}

// A candidate that should bind: cleared this frame, correct target, measured
// and holding geometry.
depth::CandidateView good() {
    return depth::CandidateView{0.0f, 100, 100, true, true, 0.99f};
}

void testFreshContentIsSelected() {
    const auto c = good();
    check(depth::eligible(c), "fresh measured candidate with content is eligible");
    check(depth::select(&c, 1) == 0, "fresh measured candidate with content is selected");
}

void testFreshButEmptyIsRejected() {
    // Measured 2026-10-05: candidate 1 was age=1 and 0.00 % covered in the
    // same frame candidate 0 held the scene. Binding it draws through.
    auto c = good();
    c.covered = 0.0f;
    check(!depth::eligible(c), "fresh but empty candidate is rejected");
    check(depth::select(&c, 1) == depth::NotFound, "fresh but empty candidate is not selected");
}

void testStaleIsRejected() {
    auto c = good();
    c.lastFrame = c.frame - depth::FreshFrames - 1;
    check(!depth::eligible(c), "candidate older than the freshness window is rejected");
    c.lastFrame = c.frame;               check(depth::eligible(c), "cleared this frame is fresh");
    c.lastFrame = c.frame - 1;           check(depth::eligible(c), "cleared last frame is still fresh");
    c.lastFrame = c.frame - 2;           check(!depth::eligible(c), "cleared two frames ago is stale");
}

void testWrongClearIsRejected() {
    auto c = good();
    c.clear = 1.0f;
    check(!depth::eligible(c), "a target cleared to 1 is not the reversed-Z scene depth");
    c.clear = 0.5f;
    check(!depth::eligible(c), "a partially cleared target is rejected");
}

void testTargetMismatchIsRejected() {
    auto c = good();
    c.targetMatches = false;
    check(!depth::eligible(c), "wrong format or size is rejected");
}

void testUnknownContentFailsClosed() {
    auto c = good();
    c.contentKnown = false;
    c.covered = 1.0f;  // even an optimistic placeholder must not bind unmeasured
    check(!depth::eligible(c), "an unmeasured candidate is rejected regardless of its placeholder");
}

void testDiscoveryOrderFallsThrough() {
    // depth[0] stale, depth[1] holds the scene: selection must reach depth[1].
    std::vector<depth::CandidateView> candidates(3);
    candidates[0] = good(); candidates[0].lastFrame = candidates[0].frame - 5;
    candidates[1] = good();
    candidates[2] = good(); candidates[2].covered = 0.0f;
    check(depth::select(candidates.data(), candidates.size()) == 1,
          "selection skips a stale first candidate for a fresh second one");
}

void testNothingEligibleIsNotFound() {
    std::vector<depth::CandidateView> candidates(3, good());
    candidates[0].covered = 0.0f;
    candidates[1].contentKnown = false;
    candidates[2].lastFrame = candidates[2].frame - 40;
    check(depth::select(candidates.data(), candidates.size()) == depth::NotFound,
          "no eligible candidate reports NotFound so composition is skipped");
}

void testEmptyListIsNotFound() {
    check(depth::select(nullptr, 0) == depth::NotFound, "an empty candidate list is NotFound");
}

std::vector<float> filled(std::size_t pixels, float value) {
    return std::vector<float>(pixels, value);
}

void testCoverageFullAndEmpty() {
    const auto all = filled(216u * 108u, 1.0f);
    const auto none = filled(216u * 108u, 0.0f);
    const auto read = [](const std::vector<float>& b, unsigned w) {
        return [w, &b](unsigned x, unsigned y) { return b[static_cast<std::size_t>(y) * w + x]; };
    };
    check(depth::coverage(read(all, 216), 216, 108) > 0.999f, "a full buffer reads as fully covered");
    check(depth::coverage(read(none, 216), 216, 108) == 0.0f, "an all-zero buffer reads as empty");
}

void testCoverageThresholdBoundary() {
    // 540 / 54 = 10 px stride, so 54 x 54 = 2916 samples are taken.
    constexpr unsigned size = 540, stride = 10;
    const auto sample = [](unsigned lit) {
        std::vector<float> data(static_cast<std::size_t>(size) * size, 0.0f);
        for (unsigned i = 0; i < lit; ++i) {
            const unsigned y = (i / 54) * stride, x = (i % 54) * stride;
            data[static_cast<std::size_t>(y) * size + x] = 1.0f;
        }
        return data;
    };
    const auto above = sample(62);  // 2.13 % of samples lit
    const auto below = sample(55);  // 1.89 %
    const auto coverAbove = depth::coverage(
        [&](unsigned x, unsigned y) { return above[static_cast<std::size_t>(y) * size + x]; }, size, size);
    const auto coverBelow = depth::coverage(
        [&](unsigned x, unsigned y) { return below[static_cast<std::size_t>(y) * size + x]; }, size, size);
    check(coverAbove >= depth::ContentThreshold, "2.13 % coverage passes the content threshold");
    check(coverBelow < depth::ContentThreshold, "1.89 % coverage fails the content threshold");
    auto c = good(); c.covered = coverBelow;
    check(!depth::eligible(c), "a candidate below the threshold is not selected");
    c.covered = coverAbove;
    check(depth::eligible(c), "a candidate above the threshold is selected");
}

void testCoverageSpansWholeTarget() {
    // Content in the far corner only: a centre-only probe would call this empty.
    const unsigned w = 400, h = 300;
    std::vector<float> data(static_cast<std::size_t>(w) * h, 0.0f);
    for (unsigned y = h - 60; y < h; ++y)
        for (unsigned x = w - 60; x < w; ++x)
            data[static_cast<std::size_t>(y) * w + x] = 1.0f;
    const float covered = depth::coverage(
        [&](unsigned x, unsigned y) { return data[static_cast<std::size_t>(y) * w + x]; }, w, h);
    check(covered >= depth::ContentThreshold,
          "geometry in a corner of the frame still counts as content");
}

void testRecheckIntervalIsMeaningful() {
    check(depth::ContentRecheckFrames > depth::FreshFrames,
          "content is re-measured much less often than freshness is checked");
    check(depth::ContentThreshold > 0.0f && depth::ContentThreshold < 1.0f,
          "the content threshold is a fraction");
}
}  // namespace

int main() {
    testFreshContentIsSelected();
    testFreshButEmptyIsRejected();
    testStaleIsRejected();
    testWrongClearIsRejected();
    testTargetMismatchIsRejected();
    testUnknownContentFailsClosed();
    testDiscoveryOrderFallsThrough();
    testNothingEligibleIsNotFound();
    testEmptyListIsNotFound();
    testCoverageFullAndEmpty();
    testCoverageThresholdBoundary();
    testCoverageSpansWholeTarget();
    testRecheckIntervalIsMeaningful();
    if (failures) { std::printf("%d depth-selection check(s) failed\n", failures); return 1; }
    std::printf("depth-selection checks passed\n");
    return 0;
}
