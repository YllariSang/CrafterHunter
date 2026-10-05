// Off-target unit tests for the shared-memory frame contract.
//
// The rules live in frame_transport.hpp without D3D types precisely so this can
// run on the host; the Java writer is checked against the same numbers in
// tests/frame-transport-agreement.sh, so a layout change on one side fails a
// test instead of producing a composited frame that is subtly wrong.
#include "frame_transport.hpp"

#include <cstddef>
#include <cstdio>
#include <cstring>
#include <vector>

using namespace crafterhunter::frame;

namespace {

int failures = 0;

void check(bool condition, const char* what) {
    if (!condition) {
        std::fprintf(stderr, "FAILED: %s\n", what);
        ++failures;
    }
}

SlotHeader makeSlot(std::uint64_t sequence, std::uint32_t width, std::uint32_t height,
                    std::uint64_t published = 1) {
    SlotHeader slot{};
    slot.sequence = sequence;
    slot.capturedNanos = 1'000'000;
    slot.width = width;
    slot.height = height;
    slot.formatVersion = FormatVersion;
    slot.published = published;
    return slot;
}

// The field order the guest writes by hand. Offsets are asserted, not assumed:
// a struct laid out differently here and differently there produces a frame that
// is *almost* right, which survives a visual check and fails a test.
void checkFieldOffsets() {
    SlotHeader slot{};
    check(offsetof(SlotHeader, formatVersion) == 0, "formatVersion is the first field");
    check(offsetof(SlotHeader, sequence) == 16, "sequence sits after the four reserved words");
    check(offsetof(SlotHeader, capturedNanos) == 24, "capturedNanos follows sequence");
    check(offsetof(SlotHeader, published) == 32, "published follows the clock");
    check(offsetof(SlotHeader, width) == 40, "width follows published");
    check(offsetof(SlotHeader, height) == 44, "height follows width");
    static_assert(sizeof(SlotHeader) == 64, "the slot header is one cache line");
    static_assert(alignof(SlotHeader) == 64, "the slot header is cache-line aligned");
    (void)slot;
}

void checkBufferHeaderOffsets() {
    BufferHeader header{};
    check(offsetof(BufferHeader, magic) == 0, "magic is the first field");
    check(offsetof(BufferHeader, formatVersion) == 4, "formatVersion follows magic");
    static_assert(sizeof(BufferHeader) == 128, "the preamble is two cache lines");
    (void)header;
}

}  // namespace

int main() {
    constexpr std::uint32_t width = 1908;
    constexpr std::uint32_t height = 1028;

    checkFieldOffsets();
    checkBufferHeaderOffsets();

    // Sizes are checked against the size the live capture actually produced:
    // 1908x1028x4 is 7,845,696 bytes, which is what a real frame measured.
    check(frameBytes(width, height) == 7'845'696, "1908x1028 RGBA8 is 7845696 bytes");
    check(frameBytes(1920, 1080) == 8'294'400, "1920x1080 RGBA8 is 8294400 bytes");
    check(frameBytes(0, 10) == 0, "a zero width is not a frame");
    check(pixelsOffset(0, width, height) >= headerBytes() + SlotCount * slotHeaderBytes(),
          "pixels start after every slot header");
    check(pixelsOffset(1, width, height) - pixelsOffset(0, width, height) == frameBytes(width, height),
          "the two slots do not overlap");
    check(bufferBytes(width, height) >= pixelsOffset(1, width, height) + frameBytes(width, height),
          "the buffer holds both slots");

    // A slot header must not share a cache line with anything, because the
    // writer stores a sequence there while pixels are being copied. The static
    // assertions in checkFieldOffsets pin the size and alignment at compile time;
    // these repeat the size at runtime so a failure names the rule.
    check(sizeof(SlotHeader) == 64, "a slot header is exactly one cache line");
    check(alignof(SlotHeader) == 64, "a slot header is cache-line aligned");

    // Freshness: a frame from now is fresh, one past the limit is not, and a
    // clock that went backwards is refused rather than treated as fresh.
    check(fresh(1'000, 1'000), "a frame captured now is fresh");
    check(fresh(1'000, 1'000 + MaxAgeNanos), "a frame exactly at the limit is fresh");
    check(!fresh(1'000, 1'000 + MaxAgeNanos + 1), "a frame one nanosecond past the limit is not");
    check(!fresh(0, 1'000), "a frame with no clock is not fresh");
    check(!fresh(2'000, 1'000), "a clock that went backwards is refused");

    // Every reason to refuse, one at a time.
    check(usable(Magic, FormatVersion, makeSlot(1, width, height), width, height),
          "a complete frame is usable");
    check(!usable(Magic ^ 1u, FormatVersion, makeSlot(1, width, height), width, height),
          "a foreign buffer is refused");
    check(!usable(Magic, FormatVersion + 1, makeSlot(1, width, height), width, height),
          "another buffer layout is refused");
    SlotHeader badVersion = makeSlot(1, width, height);
    badVersion.formatVersion = FormatVersion + 1;
    check(!usable(Magic, FormatVersion, badVersion, width, height),
          "a slot written by another layout is refused");
    check(!usable(Magic, FormatVersion, makeSlot(0, width, height), width, height),
          "an empty slot is refused");
    check(!usable(Magic, FormatVersion, makeSlot(1, width, height, 0), width, height),
          "a slot caught mid-copy is refused");
    check(!usable(Magic, FormatVersion, makeSlot(1, width, height + 1), width, height),
          "a slot of a different height is refused");
    check(!usable(Magic, FormatVersion, makeSlot(1, width - 1, height), width, height),
          "a slot of a different width is refused");

    // Choosing the newest usable frame, including the case where the writer is
    // mid-publish in the other slot.
    SlotHeader slots[2] = {makeSlot(7, width, height), makeSlot(9, width, height)};
    check(newest(Magic, FormatVersion, slots, width, height) == 1, "the higher sequence wins");
    check(newest(Magic, FormatVersion, slots, width, height) != NotNewest,
          "two complete frames yield one of them");

    slots[1].published = 0;
    check(newest(Magic, FormatVersion, slots, width, height) == 0,
          "an unfinished newer frame does not hide the last complete one");

    slots[1] = makeSlot(0, width, height);
    check(newest(Magic, FormatVersion, slots, width, height) == 0, "an empty other slot is skipped");

    slots[0] = makeSlot(0, width, height);
    slots[1] = makeSlot(0, width, height);
    check(newest(Magic, FormatVersion, slots, width, height) == NotNewest,
          "two empty slots yield nothing rather than a stale guess");

    slots[0] = makeSlot(4, width, height);
    slots[1] = makeSlot(2, width, height);
    check(newest(Magic, FormatVersion, slots, width, height) == 0,
          "the older sequence still wins on its own merits, not on slot order");

    slots[0] = makeSlot(4, width, height);
    slots[1] = makeSlot(4, width, height);
    check(newest(Magic, FormatVersion, slots, width, height) == 1,
          "a sequence tie resolves deterministically");

    check(newest(Magic ^ 1u, FormatVersion, slots, width, height) == NotNewest,
          "a foreign buffer yields nothing even with complete slots");

    // A buffer big enough for the live capture, laid out the way the reader
    // will walk it: header, both slot headers, then both frames' pixels.
    const std::size_t total = bufferBytes(width, height);
    std::vector<std::uint8_t> buffer(total, 0);
    BufferHeader* header = reinterpret_cast<BufferHeader*>(buffer.data());
    header->magic = Magic;
    header->formatVersion = FormatVersion;
    SlotHeader* live = reinterpret_cast<SlotHeader*>(buffer.data() + slotOffset(1));
    *live = makeSlot(11, width, height);
    std::memcpy(buffer.data() + pixelsOffset(1, width, height), "\x7f", 1);

    SlotHeader* read = reinterpret_cast<SlotHeader*>(buffer.data() + slotOffset(0));
    const std::uint32_t pick = newest(header->magic, header->formatVersion, read, width, height);
    check(pick == 1, "the walked buffer reports slot one as newest");
    check(*reinterpret_cast<const std::uint8_t*>(buffer.data() + pixelsOffset(1, width, height)) == 0x7f,
          "slot one's pixels are where the offset says");

    if (failures == 0) {
        std::printf(
            "Frame transport checks passed: sizes, cache lines, freshness, seven refusals, "
            "newest-slot choice, mid-publish, tie-break, walked buffer.\n");
        return 0;
    }
    std::fprintf(stderr, "%d frame transport check(s) failed\n", failures);
    return 1;
}
