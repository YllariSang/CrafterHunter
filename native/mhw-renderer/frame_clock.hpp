// Deciding whether a published frame is recent enough to composite.
//
// The two processes stamp frames with unrelated clocks, and this was measured
// rather than assumed. The guest writes System.nanoTime(); on this host it runs
// 3,422,487 ms - fifty-seven minutes - behind CLOCK_MONOTONIC, which is what a JVM
// that counts suspend time and a kernel that stops the monotonic clock during
// suspend will disagree by after a laptop has been closed once. The difference is
// also only bounded by how long the machine has been up, so it is not a constant
// that could be calibrated once.
//
// Two approaches were tried against that reality and both fail, which is why the
// shape of this header is what it is:
//
//   1. Compare the guest's stamp to ours. Refuses every frame: the clocks are
//      hours apart and no tolerance survives that.
//   2. Measure the difference on the first frame and correct for it afterwards.
//      This looks like it should work and cannot. The offset would be fitted from
//      a single sample, so `local - guest` equals that offset by construction and
//      the age it reports is identically zero - for every frame, forever. It also
//      has to learn its offset from a frame that has already passed a freshness
//      check, so with genuinely unrelated clocks no frame can ever be accepted
//      and no offset can ever be learned. The two requirements contradict.
//
// What is left is to stop comparing clocks at all and measure liveness on the
// reader's own clock: a frame is recent if the guest's sequence number advanced
// recently. That needs no agreement between the processes, and it answers the
// question that actually matters - whether the world in the frame has moved on.
//
// The bound is loose on purpose. The transport's MaxAgeNanos is 50 ms, but the
// measured publication interval is 140 ms when Minecraft is busy and 1000 ms when
// it is not, because a frame cannot be published until its GPU read-back returns.
// A 50 ms bound would refuse every frame that was ever published. Three seconds
// clears the slowest observed rate with room to spare and still notices a guest
// that has stopped within a few frames.
//
// Free of D3D types so tools/test-frame-clock.sh can check all of it off-target.
#pragma once

#include <cstdint>

namespace crafterhunter::clock {

// GetTickCount64 counts milliseconds. Nanoseconds are milliseconds times a
// million, and the multiplication is the whole conversion: there is no division
// in it, and adding one is what broke it. An earlier version divided as well and
// produced milliseconds times a hundred - a number ten thousand times too small to
// compare against anything, which made every frame look like it came from the
// future.
inline constexpr std::uint64_t millisToNanos(std::uint64_t millis) {
    return millis * 1'000'000ull;
}

// How long the guest may go without advancing its sequence before the last frame
// is treated as a world that has stopped. Justified by the measured publication
// interval above; see the note at the top of this header.
inline constexpr std::uint64_t StallBoundNanos = 3'000'000'000ull; // 3 s

// Where a frame's age comes from.
enum class AgeSource {
    // The clocks agree closely enough to compare stamps. Not the case on this
    // host, but kept because a guest on another machine may well agree, and
    // because the transport's own contract is written in terms of its stamp.
    GuestStamp,
    // The clocks cannot be compared, so age is measured from publication on our
    // own clock. This is what actually runs.
    LocalLiveness,
};

// Signed distance from the guest's stamp to ours, so "behind" and "ahead" are
// distinguishable. Unsigned subtraction cannot express that difference at all,
// which is how a stamp from the future turns into a very old frame instead of a
// refusal.
//
// Saturates rather than wrapping: a difference larger than either bound is already
// far outside anything the caller could compare, and returning a defined value
// beats returning a wrapped one.
inline constexpr std::int64_t divergenceNanos(std::uint64_t localNanos,
                                              std::uint64_t guestNanos) {
    constexpr std::uint64_t bound = 0x7fff'ffff'ffff'ffffull;
    if (guestNanos > localNanos) {
        const std::uint64_t gap = guestNanos - localNanos;
        return -static_cast<std::int64_t>(gap > bound ? bound : gap);
    }
    const std::uint64_t gap = localNanos - guestNanos;
    return static_cast<std::int64_t>(gap > bound ? bound : gap);
}

// Pick the age source from one measurement.
//
// A negative divergence means the guest stamped ahead of us, which is not a
// clock difference to be corrected for but a stamp we cannot place, so it is
// never treated as comparable however small it is.
inline constexpr AgeSource chooseAgeSource(std::int64_t divergence,
                                            std::uint64_t toleranceNanos) {
    const std::uint64_t magnitude = divergence < 0
        ? static_cast<std::uint64_t>(-(divergence + 1)) + 1u
        : static_cast<std::uint64_t>(divergence);
    return divergence >= 0 && magnitude <= toleranceNanos
        ? AgeSource::GuestStamp
        : AgeSource::LocalLiveness;
}

// The single decision the renderer makes about a frame's age.
//
// `guestCapturedNanos` is the transport's stamp and `lastAdvanceLocalNanos` is
// when this reader last saw the sequence change, both on whatever clock their
// producer used. `stallBoundNanos` bounds liveness; `maxAgeNanos` bounds the
// guest's own stamp and is the transport's, not ours.
//
// The two branches are exclusive and neither falls back to the other: a frame is
// aged one way or the other, chosen by a measured fact, and a frame that fails
// both is refused rather than drawn optimistically.
inline constexpr bool frameIsFresh(AgeSource source, std::uint64_t nowLocalNanos,
                                  std::uint64_t guestCapturedNanos,
                                  std::uint64_t lastAdvanceLocalNanos,
                                  std::uint64_t stallBoundNanos,
                                  std::uint64_t maxAgeNanos) {
    if (source == AgeSource::GuestStamp) {
        // The transport's own rule, applied only when the clocks agree.
        if (guestCapturedNanos == 0 || nowLocalNanos < guestCapturedNanos) return false;
        return nowLocalNanos - guestCapturedNanos <= maxAgeNanos;
    }
    // Liveness. `lastAdvance == 0` means nothing has ever advanced, so there is no
    // evidence the guest is publishing and the frame is refused; the alternative
    // is drawing a frame from a guest that never started.
    if (lastAdvanceLocalNanos == 0) return false;
    if (nowLocalNanos < lastAdvanceLocalNanos) return false;  // our clock went backwards
    return nowLocalNanos - lastAdvanceLocalNanos <= stallBoundNanos;
}

}  // namespace crafterhunter::clock
