// The shared-memory frame contract, on the native side.
//
// Deliberately free of D3D types, like selection.hpp, so tools/test-frame-transport.sh
// can check every rule here on the host. The Java side owns the writer and the
// same rules are stated in FrameLayout.java; if the two ever disagree, the test
// that pins them together is what notices, not a composited frame.
//
// The shape is a double buffer with a sequence number per slot. One frame in
// flight is enough: the transport's job is to hand MHW's renderer the newest
// complete frame, and a second in-flight copy would cost bandwidth to be
// reordered anyway.
#pragma once

#include <cstddef>
#include <cstdint>

namespace crafterhunter::frame {

// Bumped when the layout changes incompatibly. A reader that finds a different
// version refuses the buffer rather than reading garbage out of it, which is the
// same fail-closed rule the terrain adapter and the depth selector both use.
inline constexpr std::uint32_t FormatVersion = 1;

// Matches FrameLayout.FORMAT on the Java side. Named rather than assumed: a
// reader that guessed would composite a flipped frame on one driver and the
// right way up on another.
inline constexpr char FormatName[] = "rgba8-topdown";

inline constexpr std::uint32_t BytesPerPixel = 4;
inline constexpr std::uint32_t SlotCount = 2;
inline constexpr std::uint32_t Magic = 0x43484652u; // "CHFR"

// A frame older than this is not worth compositing. Measured 2026-10-05: a
// frame takes 1.4 ms to read out of /dev/shm and the guest samples at 20-60 Hz,
// so tens of milliseconds is generous rather than tight. A frame past it is
// stale, and a stale frame drawn into MHW's view is worse than no frame: it
// shows a Steve who is somewhere the world has since left.
inline constexpr std::uint64_t MaxAgeNanos = 50'000'000ull;

// One slot's header. Padded to a cache line so the writer's sequence store and
// the reader's sequence load do not share a line with pixel data: a false
// sharing there costs more than the whole read.
// Field order is stated here, not left to the compiler's taste: the guest writes
// these bytes by hand, so the layout is a contract. Padding goes first because a
// struct that leads with padding is the classic way for the two sides to end up
// mirrored around the alignment boundary.
struct alignas(64) SlotHeader {
    std::uint32_t formatVersion;
    std::uint32_t reserved0;
    std::uint32_t reserved1;
    std::uint32_t reserved2;
    std::uint64_t sequence;      // 0 = empty; otherwise the frame's number
    std::uint64_t capturedNanos; // the guest's clock at readback
    std::uint64_t published;     // 1 once the pixels are complete
    std::uint32_t width;
    std::uint32_t height;
};

// What a reader learned about the newest complete frame.
struct FrameView {
    std::uint64_t sequence;
    std::uint64_t capturedNanos;
    std::uint32_t width;
    std::uint32_t height;
    const std::uint8_t* pixels; // null unless usable
};

// The buffer's fixed preamble, written once by whoever creates it.
struct alignas(64) BufferHeader {
    std::uint32_t magic;
    std::uint32_t formatVersion;
    std::uint64_t reserved[14];
};

// Byte offset of a slot's header and of its pixels. Exposed so the Java side and
// the native side can be checked against each other instead of agreeing by
// accident.
constexpr std::size_t headerBytes() { return sizeof(BufferHeader); }
constexpr std::size_t slotHeaderBytes() { return sizeof(SlotHeader); }

// Offset of the first slot header, and of slot `index`'s pixels.
constexpr std::size_t slotOffset(std::uint32_t index) {
    return headerBytes() + static_cast<std::size_t>(index) * slotHeaderBytes();
}

// Pixels for a slot follow all of the headers, so a writer never has a slot's
// header sharing a page with another slot's pixels.
constexpr std::size_t pixelsOffset(std::uint32_t index, std::uint32_t width, std::uint32_t height) {
    return headerBytes() + static_cast<std::size_t>(SlotCount) * slotHeaderBytes()
        + static_cast<std::size_t>(index) * static_cast<std::size_t>(width) * height * BytesPerPixel;
}

constexpr std::size_t frameBytes(std::uint32_t width, std::uint32_t height) {
    return static_cast<std::size_t>(width) * height * BytesPerPixel;
}

// Bytes a buffer needs for frames of this size.
constexpr std::size_t bufferBytes(std::uint32_t width, std::uint32_t height) {
    return pixelsOffset(0, width, height)
        + static_cast<std::size_t>(SlotCount) * frameBytes(width, height);
}

// A frame is usable only if every one of these holds. Each is a separate rule
// because each has been a separate failure: the wrong magic is another program,
// the wrong version is another layout, an empty slot is a frame that was never
// written, an unfinished one is a writer caught mid-copy, and a zero dimension
// would make every offset below meaningless.
//
// Note what is *not* here: any comparison against the host's resolution. The
// frame's dimensions are the guest's to declare and are checked against the
// mapping in `within()` instead. An earlier version took the host's width and
// height and demanded the slot match them, which meant a 1908x1028 guest could
// never be read by a 1920x1080 host. The two will essentially never be equal, so
// that check refused every frame the transport has ever been asked to carry, and
// the letterbox exists precisely because the sizes differ.
inline bool usable(std::uint32_t magic, std::uint32_t version, const SlotHeader& slot) {
    if (magic != Magic) return false;
    if (version != FormatVersion) return false;
    if (slot.formatVersion != FormatVersion) return false;
    if (slot.sequence == 0) return false;
    if (slot.published != 1) return false;
    if (slot.width == 0 || slot.height == 0) return false;
    return true;
}

// Does a slot's own geometry fit inside a mapping of `bytes`?
//
// This replaces the host-resolution comparison and is strictly better at catching
// what that one was reaching for: a slot claiming a size the mapping cannot hold is
// a writer caught mid-resize or a corrupt header, and reading its pixels would walk
// off the end of the mapping. The *sum* is tested rather than either term, because a
// slot can claim a size whose offset is inside the buffer while its pixels are not.
inline bool within(std::uint32_t index, std::uint32_t width, std::uint32_t height,
                   std::size_t bytes) {
    if (width == 0 || height == 0) return false;
    if (index >= SlotCount) return false;
    return pixelsOffset(index, width, height) + frameBytes(width, height) <= bytes;
}

// Do two slots describe the same geometry?
//
// A resize rewrites the slots one at a time, so a reader can arrive while they
// disagree. That is refused rather than resolved: a slot's pixel offset depends on
// the buffer's declared geometry, so using one slot's dimensions to locate the
// other's pixels reads the right number of bytes from the wrong place. The next
// frame settles it.
inline bool sameGeometry(const SlotHeader& a, const SlotHeader& b) {
    return a.width == b.width && a.height == b.height;
}

// Is a frame still young enough to be worth composing?
inline bool fresh(std::uint64_t capturedNanos, std::uint64_t nowNanos) {
    if (capturedNanos == 0 || nowNanos < capturedNanos) return false;
    return nowNanos - capturedNanos <= MaxAgeNanos;
}

// The newer of two slots, or NotNewest when neither holds a usable frame.
//
// `bytes` is the size of the mapping, and it is what the slots' own geometry is
// checked against. The host's resolution is not an input and cannot be: the guest's
// frame is a different size by construction, and that is the case the letterbox
// handles rather than a reason to refuse the frame.
//
// `reason`, when given, receives why the answer is NotNewest. A reader that refuses
// without saying why is indistinguishable from one that stopped being called.
inline constexpr std::uint32_t NotNewest = static_cast<std::uint32_t>(-1);
inline std::uint32_t newest(std::uint32_t magic, std::uint32_t version,
                           const SlotHeader* slots, std::size_t bytes,
                           const char** reason = nullptr) {
    if (reason) *reason = nullptr;
    if (magic != Magic) {
        if (reason) *reason = "not one of our channels (bad magic)";
        return NotNewest;
    }
    if (version != FormatVersion) {
        if (reason) *reason = "channel format version does not match";
        return NotNewest;
    }

    const bool selfA = usable(magic, version, slots[0]);
    const bool selfB = usable(magic, version, slots[1]);
    // Bounds are judged against each slot's own geometry, because that is the
    // geometry its pixel offset is computed from.
    const bool a = selfA && within(0, slots[0].width, slots[0].height, bytes);
    const bool b = selfB && within(1, slots[1].width, slots[1].height, bytes);

    if (!a && !b) {
        if (reason) {
            *reason = (selfA || selfB) ? "frame geometry does not fit the mapping"
                                       : "no complete frame";
        }
        return NotNewest;
    }
    if (a && !b) return 0;
    if (b && !a) return 1;

    // Both complete. If they disagree about geometry the buffer is mid-resize, and
    // either choice would locate one slot's pixels by the other's arithmetic.
    if (!sameGeometry(slots[0], slots[1])) {
        if (reason) *reason = "slots disagree on frame size (resize in progress)";
        return NotNewest;
    }
    // Ties are impossible in practice (sequences increase) but resolve to slot b
    // deterministically rather than depending on argument order.
    return slots[1].sequence >= slots[0].sequence ? 1u : 0u;
}

}  // namespace crafterhunter::frame
