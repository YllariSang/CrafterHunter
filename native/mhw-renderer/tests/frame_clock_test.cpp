// Off-target unit tests for deciding whether a published frame is recent enough.
//
// The failures covered here were all silent, and two of them were arithmetic. A
// conversion short by a factor of ten thousand made every frame look like it came
// from the future; an offset fitted from one sample would have reported an age of
// identically zero. Neither crashes, so each is asserted rather than assumed.
#include "frame_clock.hpp"

#include <cstdio>

using namespace crafterhunter::clock;

namespace {

int failures = 0;

void check(bool condition, const char* what) {
    if (!condition) {
        std::fprintf(stderr, "FAILED: %s\n", what);
        ++failures;
    }
}

// The conversion, against numbers that can be checked by eye.
void checkConversion() {
    check(millisToNanos(1) == 1'000'000ull, "one millisecond is a million nanoseconds");
    check(millisToNanos(1'000) == 1'000'000'000ull, "one second is a billion nanoseconds");
    check(millisToNanos(0) == 0, "zero is zero");

    // The exact regression, stated as a ratio so it survives any future edit: a
    // million milliseconds is about seventeen minutes, not eleven days.
    check(millisToNanos(1'000'000) / 1'000'000ull == 1'000'000ull,
          "a million milliseconds is about a thousand seconds, not eleven days");
    check(millisToNanos(1'000) == 1'000ull * 1'000'000ull,
          "the conversion multiplies by a million and never divides");
}

// The measured situation on this host, which is the one that matters.
void checkMeasuredClocksAreUnrelated() {
    // The guest's stamp sat 3,422,487 ms behind CLOCK_MONOTONIC. Any tolerance
    // sized for a frame budget rejects that, which is the point.
    const std::uint64_t local = 35'294'342'479'253ull;
    const std::uint64_t guest = 31'871'855'753'258ull;
    const std::int64_t divergence = divergenceNanos(local, guest);
    check(divergence > 0, "a stamp behind ours is a positive divergence");
    check(divergence == 3'422'486'725'995ll,
          "the divergence is exactly the measured difference, to the nanosecond");
    check(chooseAgeSource(divergence, 50'000'000ull) == AgeSource::LocalLiveness,
          "fifty-seven minutes of divergence is never comparable at a 50 ms tolerance");
}

// A guest stamp ahead of ours must be distinguishable from one behind, because
// only one of them is a clock difference and the other is unplaceable.
void checkDirectionMatters() {
    const std::uint64_t now = 10'000'000'000ull;
    const std::int64_t behind = divergenceNanos(now, now - 1'000'000ull);
    const std::int64_t ahead = divergenceNanos(now, now + 1'000'000ull);
    check(behind == 1'000'000ll, "a stamp behind reads positive");
    check(ahead == -1'000'000ll, "a stamp ahead reads negative rather than wrapping");
    check(chooseAgeSource(behind, 50'000'000ull) == AgeSource::GuestStamp,
          "a millisecond of divergence is comparable");
    check(chooseAgeSource(ahead, 50'000'000ull) == AgeSource::LocalLiveness,
          "a stamp from the future is never comparable, however small");
}

// The boundary of comparability, both sides.
void checkComparabilityBoundary() {
    check(chooseAgeSource(50'000'000ll, 50'000'000ull) == AgeSource::GuestStamp,
          "exactly at the tolerance the clocks are comparable");
    check(chooseAgeSource(50'000'001ll, 50'000'000ull) == AgeSource::LocalLiveness,
          "one nanosecond past the tolerance is not");
}

// The transport's own rule, still applied when it can be trusted.
void checkGuestStampRule() {
    constexpr std::uint64_t maxAge = 50'000'000ull;
    const std::uint64_t now = millisToNanos(10'000);
    check(frameIsFresh(AgeSource::GuestStamp, now, now - 5'000'000ull, 0, 0, maxAge),
          "a stamp five milliseconds old is fresh");
    check(frameIsFresh(AgeSource::GuestStamp, now, now - maxAge, 0, 0, maxAge),
          "a stamp exactly at the limit is fresh");
    check(!frameIsFresh(AgeSource::GuestStamp, now, now - maxAge - 1, 0, 0, maxAge),
          "one nanosecond past the limit is not");
    check(!frameIsFresh(AgeSource::GuestStamp, now, 0, 0, 0, maxAge),
          "a zero stamp is refused rather than treated as brand new");
    check(!frameIsFresh(AgeSource::GuestStamp, now, now + 1, 0, 0, maxAge),
          "a stamp from the future is refused rather than wrapping to ancient");
}

// Liveness, which is what actually runs on this host.
void checkLivenessRule() {
    constexpr std::uint64_t bound = StallBoundNanos;
    const std::uint64_t now = millisToNanos(1'000'000);
    // Published a second ago, at the rate actually measured.
    check(frameIsFresh(AgeSource::LocalLiveness, now, 0, now - 1'000'000'000ull, bound, 0),
          "a frame published a second ago is fresh at the measured rate");
    check(frameIsFresh(AgeSource::LocalLiveness, now, 0, now - bound, bound, 0),
          "a frame published exactly at the stall bound is still fresh");
    check(!frameIsFresh(AgeSource::LocalLiveness, now, 0, now - bound - 1, bound, 0),
          "one nanosecond past the stall bound is not");
    check(!frameIsFresh(AgeSource::LocalLiveness, now, 0, 0, bound, 0),
          "a guest that never advanced is refused, not drawn optimistically");
    check(!frameIsFresh(AgeSource::LocalLiveness, now, 0, now + 1, bound, 0),
          "our own clock going backwards is refused rather than wrapping");
}

// The stall bound has to clear the slowest rate measured, which is the reason it
// is not the transport's 50 ms. If either of these fails, the bound is wrong.
void checkStallBoundClearsMeasuredRates() {
    check(StallBoundNanos > 1'000'000'000ull,
          "the stall bound clears the measured 1000 ms publication interval");
    check(StallBoundNanos > 50'000'000ull,
          "and the transport's 50 ms MaxAgeNanos, which is below any real rate");
    check(StallBoundNanos < 60'000'000'000ull,
          "while staying short enough to notice a stopped guest within a minute");
}

// The branch actually taken must depend only on the measured source, never on
// the other branch's inputs: passing a plausible guest stamp must not rescue a
// frame that liveness says has stopped.
void checkBranchesDoNotRescue() {
    const std::uint64_t now = millisToNanos(1'000'000);
    // Stalled long ago, but carrying a stamp that looks perfectly fresh.
    check(!frameIsFresh(AgeSource::LocalLiveness, now, now - 1'000'000ull,
                        now - 10 * StallBoundNanos, StallBoundNanos, 50'000'000ull),
          "a fresh-looking guest stamp does not rescue a frame that stopped publishing");
    // And a frame with no liveness evidence is refused even with no stamp to judge.
    check(!frameIsFresh(AgeSource::LocalLiveness, now, 0, 0, StallBoundNanos, 0),
          "no evidence of publishing means no frame");
}

}  // namespace

int main() {
    checkConversion();
    checkMeasuredClocksAreUnrelated();
    checkDirectionMatters();
    checkComparabilityBoundary();
    checkGuestStampRule();
    checkLivenessRule();
    checkStallBoundClearsMeasuredRates();
    checkBranchesDoNotRescue();

    if (failures == 0) {
        std::printf(
            "Frame clock checks passed: conversion, measured divergence, stamp direction, "
            "comparability boundary, guest-stamp rule, liveness rule, stall bound clears the "
            "measured rates, branches do not rescue.\n");
        return 0;
    }
    std::fprintf(stderr, "%d frame clock check(s) failed\n", failures);
    return 1;
}
