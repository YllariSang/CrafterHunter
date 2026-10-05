// Off-target unit tests for the frame source's decision logic.
//
// The mapping itself needs a file and a platform, so what is checked here is
// everything around it: a reader handed a buffer it did not write must refuse
// rather than composite it, and the refusals must be the same ones the header
// already states. A frame reader that draws a stale or foreign buffer produces
// a plausible image, which is why each refusal is asserted rather than assumed.
#include "frame_source.hpp"
#include "frame_clock.hpp"

#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

// The header under test, so one check can assert the shape of its Windows branch.
// A host test cannot execute that branch, but it can insist the branch exists and
// does not cache a length.
#ifndef FRAME_SOURCE_PATH
#error "FRAME_SOURCE_PATH must name frame_source.hpp"
#endif

using namespace crafterhunter::frame;
using namespace crafterhunter::frames;

namespace {

int failures = 0;

void check(bool condition, const char* what) {
    if (!condition) {
        std::fprintf(stderr, "FAILED: %s\n", what);
        ++failures;
    }
}

// A buffer laid out the way the guest lays one out.
std::vector<std::uint8_t> makeChannel(std::uint32_t width, std::uint32_t height,
                                      std::uint32_t magic = Magic,
                                      std::uint32_t version = FormatVersion) {
    std::vector<std::uint8_t> buffer(bufferBytes(width, height), 0);
    auto* header = reinterpret_cast<BufferHeader*>(buffer.data());
    header->magic = magic;
    header->formatVersion = version;
    return buffer;
}

void writeSlot(std::vector<std::uint8_t>& buffer, std::uint32_t index, std::uint64_t sequence,
               std::uint64_t capturedNanos, std::uint32_t width, std::uint32_t height,
               std::uint64_t published = 1) {
    auto* slots = reinterpret_cast<SlotHeader*>(buffer.data() + headerBytes());
    slots[index].formatVersion = FormatVersion;
    slots[index].sequence = sequence;
    slots[index].capturedNanos = capturedNanos;
    slots[index].published = published;
    slots[index].width = width;
    slots[index].height = height;
}

// The re-check that catches a slot recycled mid-upload.
//
// `newestFrame()` hands back a pointer into shared memory and the caller spends
// milliseconds uploading from it. This checks the decision made *after* that
// upload, which is the only point at which a recycled slot is visible.
void checkRecycledSlotDetected() {
    // The rule is stated against a buffer, since a mapped channel cannot be
    // arranged to be recycled at the right moment from here.
    struct SlotView { std::uint64_t sequence; std::uint64_t published; };
    auto stillHolds = [](const SlotView* slots, std::size_t count, std::uint64_t sequence) {
        for (std::size_t i = 0; i < count; ++i) {
            if (slots[i].sequence == sequence) return slots[i].published == 1;
        }
        return false;
    };

    const SlotView settled[2] = {{7, 1}, {6, 1}};
    check(stillHolds(settled, 2, 7), "a finished frame in either slot is recognised");

    // The guest marks a slot incomplete before refilling it, keeping its old
    // sequence. That is the case where the sequence still matches and only the
    // flag gives it away.
    const SlotView filling[2] = {{7, 0}, {6, 1}};
    check(!stillHolds(filling, 2, 7), "a slot being refilled is not mistaken for a finished frame");
    check(stillHolds(filling, 2, 6), "and the other slot is unaffected");

    // Recycled and finished: the sequence has moved on, so ours is gone.
    const SlotView moved[2] = {{8, 1}, {7, 1}};
    check(!stillHolds(moved, 2, 6), "a frame whose slot has been reused is not still held");
    check(stillHolds(moved, 2, 8), "while the newest one is");
    check(!stillHolds(settled, 2, 0), "sequence zero is never held");
}

// Regression: recovery from a channel that has stopped yielding frames.
//
// The reader used to return on "no complete frame" without re-opening anything, so
// once its mapping described something the guest was no longer writing - an unlinked
// file, a recreated one, or one from before a restart - it kept describing it
// forever. Nothing observable from inside that mapping would ever have said so,
// which is why this had to be a policy rather than a condition.
void checkBoundedRecovery() {
    // The first attempt is always allowed: a reader that has never tried must not
    // wait out an interval before its first try.
    check(mayRemap(1'000'000'000ull, 0), "a reader that has never re-opened may do so now");

    // And then it is bounded, which is the property that stops a closed Minecraft
    // costing one remap per frame.
    // An attempt at T sets the clock, so the next is due at T + interval and not
    // before. Written as a boundary on both sides, because getting this wrong in the
    // permissive direction is one remap per frame.
    const std::uint64_t attempt = 5'000'000'000ull;
    check(!mayRemap(attempt, attempt), "the instant of the last attempt is not due again");
    check(!mayRemap(attempt + 1, attempt), "a nanosecond later it is not");
    check(!mayRemap(attempt + RemapIntervalNanos - 1, attempt),
          "one nanosecond before the interval it is not");
    check(mayRemap(attempt + RemapIntervalNanos, attempt),
          "exactly at the interval it is allowed again");
    check(mayRemap(attempt + 10 * RemapIntervalNanos, attempt),
          "well past the interval it is allowed");

    // A clock that went backwards must not deadlock recovery: the subtraction would
    // never reach the interval again.
    check(mayRemap(1'000, 5'000'000'000ull), "a backwards clock is treated as due");

    // A frame in hand means there is nothing to recover from.
    check(!shouldRemap(true, attempt + 10 * RemapIntervalNanos, attempt),
          "a channel yielding frames is never re-opened, however long since the last attempt");
    // No frame means the mapping may be describing the past - but still rate-limited.
    check(shouldRemap(false, attempt + 10 * RemapIntervalNanos, attempt),
          "no frame and the interval elapsed: re-open");
    check(!shouldRemap(false, attempt + 1, attempt),
          "no frame but inside the interval: wait rather than re-open every frame");
    check(shouldRemap(false, attempt + 1, 0),
          "no frame and never tried: re-open immediately");
}

// Regression: a frozen frame stays refused across repeated re-open attempts.
//
// The recovery routine used to zero the liveness clock on every successful re-open.
// Re-opening a channel whose file still exists always succeeds, including when that
// file is the *same* frozen one - so once per second the reader forgot the sequence
// had stopped advancing, observed the same unmoving sequence as if it were new
// progress, and drew a frame from a guest that had gone away. Recovery was not merely
// failing to help; once a second it was defeating the stall detection meant to stop
// exactly this.
//
// The test simulates the loop rather than calling it: memory, a stalled sequence, a
// re-open, and back again.
void checkFrozenFrameSurvivesRecovery() {
    constexpr std::uint32_t width = 1908;
    constexpr std::uint32_t height = 1028;
    constexpr std::uint64_t stallSequence = 42;
    constexpr std::uint64_t lastAdvance = 5'000'000'000ull;
    constexpr std::uint64_t bound = 3'000'000'000ull;

    FrameMemory memory{stallSequence, width, height, lastAdvance, true};
    const std::uint64_t now = lastAdvance + 60'000'000'000ull;  // a minute later

    // Before recovery the frame is already stale.
    check(!crafterhunter::clock::frameIsFresh(crafterhunter::clock::AgeSource::LocalLiveness,
              now, 0, memory.advanceNanos, bound, 0),
          "a sequence that stopped advancing a minute ago is not fresh");

    // Five re-open attempts, each seeing exactly the same frozen channel.
    for (int attempt = 1; attempt <= 5; ++attempt) {
        const Generation generation = classifyGeneration(memory, true, true,
            stallSequence, width, height);
        check(generation == Generation::Same,
              "an unmoving sequence on unchanged geometry is the same writer");
        check(!shouldForget(generation),
              "so the reader must not forget what it knows about it");

        // Because the memory survives, the frame is still refused afterwards.
        check(!crafterhunter::clock::frameIsFresh(crafterhunter::clock::AgeSource::LocalLiveness,
                  now + static_cast<std::uint64_t>(attempt) * bound, 0, memory.advanceNanos,
                  bound, 0),
              "and it is still refused after re-opening");
    }

    // A genuinely new writer does clear the memory, and the three ways to be one are
    // each distinguished. Without these the fix would just be "never forget".
    check(classifyGeneration(memory, true, true, stallSequence - 1, width, height)
              == Generation::Restarted,
          "a sequence that went backwards is a restarted writer");
    check(classifyGeneration(memory, true, true, stallSequence + 1, 1904, 1024)
              == Generation::Resized,
          "changed geometry is a new buffer");
    check(classifyGeneration(memory, false, true, stallSequence, width, height)
              == Generation::Replaced,
          "a bad magic is a different file altogether");
    check(classifyGeneration(memory, true, false, stallSequence, width, height)
              == Generation::Replaced,
          "and so is another format version");
    check(shouldForget(Generation::Restarted)
              && shouldForget(Generation::Resized)
              && shouldForget(Generation::Replaced),
          "each of those does clear the reader's memory");

    // A forward sequence is progress, not a new generation: the memory stays and the
    // caller refreshes the liveness clock.
    check(classifyGeneration(memory, true, true, stallSequence + 1, width, height)
              == Generation::Same,
          "a higher sequence on the same geometry is the same writer, still working");
    check(!shouldForget(Generation::Same), "so nothing is forgotten");

    // A reader that has never seen a writer has no memory to keep.
    const FrameMemory none{};
    check(classifyGeneration(none, true, true, 7, width, height) == Generation::Restarted,
          "an unestablished reader treats what it finds as a new generation");
}

// The decision an unmapped reader must reach: refuse, and say why.
void checkUnmappedRefuses() {
    Source source;
    check(!source.mapped(), "a source that was never opened is not mapped");
    check(!source.open() || source.mapped(),
          "opening a channel that does not exist must fail rather than pretend");

    const char* reason = nullptr;
    // Even if a caller ignored `open()` and asked anyway, the answer is a refusal.
    FrameView view = source.newestFrame(1'000'000, &reason);
    check(view.pixels == nullptr, "an unmapped source yields no pixels");
    check(reason != nullptr, "an unmapped source says why");
}

void checkShortBufferRefuses() {
    // A buffer with only the preamble: enough to hold a header, not a frame.
    std::vector<std::uint8_t> buffer(headerBytes(), 0);
    auto* header = reinterpret_cast<BufferHeader*>(buffer.data());
    header->magic = Magic;
    header->formatVersion = FormatVersion;

    // Walk it the way the reader does, with no platform involved: the rule being
    // checked is "a buffer that cannot hold a frame is refused".
    const auto* slots = reinterpret_cast<const SlotHeader*>(buffer.data() + headerBytes());
    check(buffer.size() < headerBytes() + SlotCount * slotHeaderBytes(),
          "a preamble-only buffer is shorter than its headers");
    (void)slots;
}

// The rule behind that refusal, pinned directly.
//
// This is the threshold whose failure mode was invisible from the host: on
// Windows the mapping's length was never stored, so it read as 0, and a zero
// length fails this test on the machine that matters. The test below asserts the
// length is *derived* on Windows so the same failure cannot come back.
void checkLengthRule() {
    const std::uint64_t headers = headerBytes() + std::uint64_t(SlotCount) * slotHeaderBytes();
    check(!channelLongEnough(0), "a zero length is never enough");
    check(!channelLongEnough(headers - 1), "one byte short of the headers is refused");
    check(channelLongEnough(headers), "exactly the headers is enough");
    // The real capture, to check against a number rather than a round value.
    constexpr std::uint32_t width = 1908;
    constexpr std::uint32_t height = 1028;
    check(channelLongEnough(bufferBytes(width, height)),
          "a real 1908x1028 channel passes the length rule");
}

// The length must be assigned once, after the platform split.
//
// This asserts the *shape* of the code rather than its behaviour, because a host test
// cannot run the Windows branch and so cannot observe a zero length there. What it can
// insist on is that there is only one place the length is set, that the place is not
// inside a platform branch, and that length() has no per-platform variant - because a
// platform-specific length() is exactly how the two came to disagree in the first place.
void checkLengthAssignedOnceOutsidePlatformSplit() {
    const char* source = FRAME_SOURCE_PATH;
    std::FILE* file = std::fopen(source, "rb");
    if (!file) {
        check(false, "frame_source.hpp could not be read for the length-assignment check");
        return;
    }
    std::string text;
    char buffer[4096];
    std::size_t read = 0;
    while ((read = std::fread(buffer, 1, sizeof(buffer), file)) > 0) text.append(buffer, read);
    std::fclose(file);

    // "bool open(" rather than "bool open() {" - the signature gained an optional path so
    // a test can point a reader at its own file, and an assertion pinned to the old
    // spelling would fail over a change that was deliberate.
    const std::size_t openStart = text.find("bool open(");
    check(openStart != std::string::npos, "Source::open() exists");
    if (openStart == std::string::npos) return;
    const std::size_t openEnd = text.find("void close() {", openStart);
    check(openEnd != std::string::npos, "and ends before close()");
    if (openEnd == std::string::npos) return;
    const std::string body = text.substr(openStart, openEnd - openStart);

    std::size_t assignments = 0;
    for (std::size_t at = body.find("length_ ="); at != std::string::npos;
         at = body.find("length_ =", at + 1)) {
        ++assignments;
    }
    const std::string howMany = std::to_string(assignments);
    check(assignments == 1,
          ("the mapping length is assigned exactly once inside open(), so a platform"
           " branch cannot omit it; found " + howMany).c_str());

    // The single assignment must come after both platform branches close, so neither
    // can bypass it.
    const std::size_t lastPlatformEnd = body.rfind("#endif");
    const std::size_t assignmentAt = body.find("length_ =");
    check(lastPlatformEnd != std::string::npos && assignmentAt != std::string::npos
              && assignmentAt > lastPlatformEnd,
          "and it comes after the platform split, not inside either branch");

    // A platform-specific length() is what let the two disagree, and VirtualQuery's
    // region size is page-rounded, so it is not the file length either.
    const std::size_t lengthAt = text.find("std::size_t length() const");
    check(lengthAt != std::string::npos, "Source::length() exists");
    if (lengthAt != std::string::npos) {
        const std::size_t implEnd = text.find("}", lengthAt);
        const std::string impl = text.substr(lengthAt, implEnd - lengthAt);
        check(impl.find("#if") == std::string::npos,
              "length() has no platform-specific variant, so the platforms cannot disagree");
        check(impl.find("VirtualQuery") == std::string::npos,
              "and it does not use VirtualQuery, whose region size is page-rounded and so"
              " is not the file length");
    }
}

// The path spellings the Windows reader tries under Proton.
void checkChannelPathSpellings() {
    check(ChannelPathCandidates == 2, "two spellings are tried");
    // The Unix spelling is the one the guest uses, byte for byte, or the two processes
    // are writing and reading different files.
    check(std::string(ChannelPathUnix) == std::string(ChannelPath),
          "the Unix spelling is exactly the path the guest publishes to");
    // The drive spelling is the same path as a Windows process under Proton sees it: Z:
    // is the mapping of /, and Wine's path handling expects backslashes.
    check(std::string(ChannelPathDrive).rfind("Z:", 0) == 0, "the drive spelling starts at Z:");
    check(std::string(ChannelPathDrive).find('/') == std::string::npos,
          "and uses backslashes throughout, as Wine expects");
    check(std::string(ChannelPathDrive).find("shm") != std::string::npos,
          "naming the same directory as the Unix spelling");
    check(std::string(ChannelPathDrive).find("frame.channel") != std::string::npos,
          "and the same file");
    // Out-of-range indices fall back rather than reading past the list.
    check(std::string(channelPathAt(0)) == std::string(ChannelPathUnix), "index 0 is the Unix path");
    check(std::string(channelPathAt(1)) == std::string(ChannelPathDrive), "index 1 is the drive path");
    check(std::string(channelPathAt(99)) == std::string(ChannelPathUnix),
          "an out-of-range index falls back rather than reading past the list");
}
void checkStaleRefuses() {
    constexpr std::uint32_t width = 1908;
    constexpr std::uint32_t height = 1028;
    auto buffer = makeChannel(width, height);
    writeSlot(buffer, 0, 5, 1'000'000, width, height);
    writeSlot(buffer, 1, 6, 1'000'000, width, height);

    // Two seconds later the frame is well past MaxAgeNanos and must be refused,
    // because a Steve from two seconds ago is somewhere the world has left.
    const bool stale = !crafterhunter::frame::fresh(1'000'000, 3'000'000'000ull);
    check(stale, "a frame two seconds old is stale");

    // And the same frame shortly after capture is fresh: the limit is 50 ms, so
    // a millisecond is well inside it. Both sides of the boundary are checked,
    // because a limit that is off by a factor of a thousand is invisible until
    // frames are refused wholesale.
    check(crafterhunter::frame::fresh(1'000'000, 1'000'000 + 1'000'000ull),
          "a frame a millisecond old is fresh");
    check(crafterhunter::frame::fresh(1'000'000, 1'000'000 + MaxAgeNanos),
          "a frame exactly at the limit is still fresh");
    check(!crafterhunter::frame::fresh(1'000'000, 1'000'000 + MaxAgeNanos + 1),
          "one nanosecond past the limit is not fresh");
}

void checkOutOfRangeRefuses() {
    constexpr std::uint32_t width = 1908;
    constexpr std::uint32_t height = 1028;
    auto buffer = makeChannel(width, height);
    writeSlot(buffer, 0, 1, 1'000, width, height);

    // A frame claiming a size the buffer cannot hold: the offset runs past the
    // end, and the reader must notice rather than walk off it. This is the shape
    // a half-finished resize produces.
    writeSlot(buffer, 0, 2, 1'000, 3840, 2160);
    // The offset itself is inside the buffer - it is the frame that runs past
    // the end, which is why the check has to be on the sum rather than on
    // either half.
    const std::size_t offset = pixelsOffset(0, 3840, 2160);
    check(offset <= buffer.size(), "an oversized frame's offset starts inside the buffer");
    check(offset + frameBytes(3840, 2160) > buffer.size(),
          "an oversized frame claims space the buffer does not have");
}

void checkGeometryOfRealCapture() {
    // The sizes the live capture produced, so the arithmetic is checked against a
    // real frame rather than a round number.
    constexpr std::uint32_t width = 1908;
    constexpr std::uint32_t height = 1028;
    auto buffer = makeChannel(width, height);
    check(buffer.size() == 15'691'648, "the 1908x1028 channel is 15691648 bytes");
    writeSlot(buffer, 1, 12, 42'000, width, height);

    const std::size_t offset = pixelsOffset(1, width, height);
    check(offset + frameBytes(width, height) == buffer.size(),
          "the last slot's frame ends exactly at the end of the buffer");
    check(pixelsOffset(0, width, height) == headerBytes() + 2 * slotHeaderBytes(),
          "pixels start after every header");
}

}  // namespace

int main() {
    checkUnmappedRefuses();
    checkShortBufferRefuses();
    checkLengthRule();
    checkLengthAssignedOnceOutsidePlatformSplit();
    checkChannelPathSpellings();
    checkRecycledSlotDetected();
    checkBoundedRecovery();
    checkFrozenFrameSurvivesRecovery();
    checkStaleRefuses();
    checkOutOfRangeRefuses();
    checkGeometryOfRealCapture();

    if (failures == 0) {
        std::printf(
            "Frame source checks passed: unmapped refuses, short buffer refuses, length rule, "
            "length assigned once, path spellings, recycled slot detected, bounded recovery, "
            "stale refuses, "
            "oversized refuses, frozen frame survives recovery, real-capture geometry.\n");
        return 0;
    }
    std::fprintf(stderr, "%d frame source check(s) failed\n", failures);
    return 1;
}
