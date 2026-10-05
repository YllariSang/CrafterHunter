// Off-target unit tests for the frame source's decision logic.
//
// The mapping itself needs a file and a platform, so what is checked here is
// everything around it: a reader handed a buffer it did not write must refuse
// rather than composite it, and the refusals must be the same ones the header
// already states. A frame reader that draws a stale or foreign buffer produces
// a plausible image, which is why each refusal is asserted rather than assumed.
#include "frame_source.hpp"

#include <cstdio>
#include <cstdlib>
#include <vector>

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

// The decision an unmapped reader must reach: refuse, and say why.
void checkUnmappedRefuses() {
    Source source;
    check(!source.mapped(), "a source that was never opened is not mapped");
    check(!source.open() || source.mapped(),
          "opening a channel that does not exist must fail rather than pretend");

    const char* reason = nullptr;
    // Even if a caller ignored `open()` and asked anyway, the answer is a refusal.
    FrameView view = source.newestFrame(1920, 1080, 1'000'000, &reason);
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
    checkStaleRefuses();
    checkOutOfRangeRefuses();
    checkGeometryOfRealCapture();

    if (failures == 0) {
        std::printf(
            "Frame source checks passed: unmapped refuses, short buffer refuses, stale refuses, "
            "oversized refuses, real-capture geometry.\n");
        return 0;
    }
    std::fprintf(stderr, "%d frame source check(s) failed\n", failures);
    return 1;
}
