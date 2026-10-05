// Reading the guest's frames out of shared memory.
//
// Free of D3D types, like selection.hpp and frame_composite.hpp, so
// tools/test-frame-source.sh can check the decision logic on the host. The
// mmap itself is a thin wrapper over the same rules, and every reason to refuse a
// frame is stated here rather than at the call site.
//
// Why mmap rather than reading the file each frame: the measured cost of reading
// 7.8 MB out of /dev/shm is 1.41 ms median, which is affordable but is a copy of
// the whole frame on the render thread every frame. Mapping once and reading
// only the slot that changed turns that into a header read plus a texture upload,
// which is what the GPU needs anyway.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>

#if defined(_WIN32)
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#include "frame_transport.hpp"

namespace crafterhunter::frames {

// Where the guest publishes frames. Fixed path: both processes need to agree
// before either can exist, and a configured path would add a place for them to
// disagree without removing anything.
inline constexpr char ChannelPath[] = "/dev/shm/crafterhunter/frame.channel";

// Is a mapping of this many bytes big enough to hold the channel's headers?
//
// Pulled out as a rule rather than left inline so the host can check it. The bug
// it exists to catch was not in this arithmetic - it was that `length()` reported
// zero on Windows - but a test of the rule at least pins the threshold, and the
// same test file asserts that the Windows branch derives its length instead of
// storing one.
inline constexpr bool channelLongEnough(std::uint64_t bytes) {
    return bytes >= crafterhunter::frame::headerBytes()
        + static_cast<std::uint64_t>(crafterhunter::frame::SlotCount)
            * crafterhunter::frame::slotHeaderBytes();
}

#if defined(_WIN32)
using MappingHandle = HANDLE;
#else
using MappingHandle = int;
#endif

// A mapped view of the guest's channel. Construction maps the file; nothing is
// read until `newest` is called, because a mapping that failed and a frame that
// is absent are different failures and should not look the same.
class Source {
public:
    Source() = default;
    ~Source() { close(); }

    Source(const Source&) = delete;
    Source& operator=(const Source&) = delete;

    // Maps the channel. Returns false when the guest has never published, which
    // is the normal state before Minecraft is loaded and must not be an error.
    //
    // A file that exists but is still empty is refused rather than mapped. The
    // guest creates the channel and fills it in the same constructor, so an empty
    // one means we caught it mid-creation; mapping it would produce a zero-length
    // section whose every read fails, which is a far more confusing symptom than
    // "not there yet".
    bool open() {
        close();
#if defined(_WIN32)
        HANDLE file = CreateFileA(ChannelPath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
            nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file == INVALID_HANDLE_VALUE) return false;
        LARGE_INTEGER size{};
        if (!GetFileSizeEx(file, &size)) { CloseHandle(file); return false; }
        if (size.QuadPart <= 0) { CloseHandle(file); return false; }
        // Map the file whole. Its length is written by the guest and bounds both
        // slots, so this is the only size that can be trusted before a header has
        // been read - which is why length() asks the mapping for its size rather
        // than remembering this one.
        mapping_ = CreateFileMappingA(file, nullptr, PAGE_READONLY, 0, 0, nullptr);
        CloseHandle(file);
        if (mapping_ == nullptr) return false;
        view_ = MapViewOfFile(mapping_, FILE_MAP_READ, 0, 0, 0);
        if (view_ == nullptr) { CloseHandle(mapping_); mapping_ = nullptr; return false; }
#else
        const int fd = ::open(ChannelPath, O_RDONLY);
        if (fd < 0) return false;
        struct stat st{};
        if (fstat(fd, &st) != 0 || st.st_size <= 0) { ::close(fd); return false; }
        void* mapped = mmap(nullptr, static_cast<std::size_t>(st.st_size),
            PROT_READ, MAP_SHARED, fd, 0);
        ::close(fd);
        if (mapped == MAP_FAILED) return false;
        view_ = mapped;
        length_ = static_cast<std::size_t>(st.st_size);
#endif
        mapped_ = true;
        return true;
    }

    void close() {
        if (!mapped_) return;
#if defined(_WIN32)
        UnmapViewOfFile(view_);
        if (mapping_) CloseHandle(mapping_);
        mapping_ = nullptr;
#else
        munmap(view_, length_);
#endif
        view_ = nullptr;
        mapped_ = false;
    }

    // Is the frame we were just handed still the frame in that slot?
    //
    // `newestFrame()` returns a pointer into shared memory, and the caller then
    // spends milliseconds uploading several megabytes out of it. Two frames later
    // the guest recycles that same slot and starts overwriting those bytes. The
    // upload would then composite a frame half old and half new, which looks like
    // a rendering fault and is not one.
    //
    // So this is asked again *after* the upload. A false answer means the slot was
    // recycled mid-read and the texture must not be drawn; the caller drops the
    // frame and the next one uploads afresh. Dropping is the right outcome: a
    // whole frame late is invisible, a torn one is not.
    //
    // The guest marks a slot incomplete before refilling it, so a slot being
    // written is detectable here as well as by its sequence.
    bool mapped() const { return mapped_; }

    bool stillHolds(std::uint64_t sequence) const {
        if (!mapped_ || !channelLongEnough(length()) || sequence == 0) return false;
        const auto* bytes = static_cast<const std::uint8_t*>(view_);
        const auto* header = reinterpret_cast<const crafterhunter::frame::BufferHeader*>(bytes);
        const auto* slots = reinterpret_cast<const crafterhunter::frame::SlotHeader*>(
            bytes + crafterhunter::frame::headerBytes());
        if (header->magic != crafterhunter::frame::Magic
            || header->formatVersion != crafterhunter::frame::FormatVersion) {
            return false;
        }
        for (std::uint32_t slot = 0; slot < crafterhunter::frame::SlotCount; ++slot) {
            if (slots[slot].sequence == sequence) {
                // Found it. Now check it is still a complete frame: a slot being
                // refilled keeps its old sequence but drops its published flag.
                return slots[slot].published == 1;
            }
        }
        // No slot claims this sequence any more, so it was recycled and finished.
        return false;
    }

    // How many bytes are actually readable through this mapping.
    //
    // On Windows this is asked of the mapping rather than remembered from open(),
    // and that is the whole point. A stored length is a second copy of a fact the
    // OS already knows, and the copy is exactly what went missing: length_ was
    // assigned only in the POSIX branch, so on Windows it stayed 0, every frame
    // failed the "shorter than its headers" check, and Steve could never appear
    // no matter how many frames were published. No host test could see it either,
    // because the branch that was wrong is the one Linux never compiles.
    //
    // Deriving it removes the assignment that could be forgotten, rather than
    // restoring the one that was.
    std::size_t length() const {
        if (view_ == nullptr) return 0;
#if defined(_WIN32)
        MEMORY_BASIC_INFORMATION info{};
        if (VirtualQuery(view_, &info, sizeof(info)) == 0) return 0;
        return static_cast<std::size_t>(info.RegionSize);
#else
        return length_;
#endif
    }

    // The newest complete frame, or a frame with null pixels when there is
    // nothing to draw. Never guesses: every refusal returns a null pointer and a
    // reason the caller can log once.
    //
    // `nowNanos` is accepted but deliberately unused for the age decision. This
    // function cannot judge freshness: the guest stamps frames with a clock
    // measured 3,422,487 ms away from ours, so comparing the two here would
    // refuse every frame that was ever published. The caller decides, using
    // frame_clock.hpp, which is where the measurement behind that decision is
    // recorded.
    crafterhunter::frame::FrameView newestFrame(std::uint32_t hostWidth,
        std::uint32_t hostHeight, std::uint64_t nowNanos, const char** reason) const {
        (void)nowNanos;
        crafterhunter::frame::FrameView view{0, 0, 0, 0, nullptr};
        if (!mapped_) { *reason = "channel not mapped"; return view; }
        if (!channelLongEnough(length())) {
            // A zero here is not a small channel: it means the mapping's length
            // could not be established, which on the platform this ran on is a
            // different failure from a truncated file and worth saying so.
            *reason = length() == 0 ? "mapping length unknown" : "channel shorter than its headers";
            return view;
        }

        const auto* bytes = static_cast<const std::uint8_t*>(view_);
        const auto* header = reinterpret_cast<const crafterhunter::frame::BufferHeader*>(bytes);
        const auto* slots = reinterpret_cast<const crafterhunter::frame::SlotHeader*>(
            bytes + crafterhunter::frame::headerBytes());
        // Qualified: the member and this free function share a name, and an
        // unqualified call inside the class would resolve to the member.
        const std::uint32_t pick = crafterhunter::frame::newest(header->magic,
            header->formatVersion, slots, hostWidth, hostHeight);
        if (pick == crafterhunter::frame::NotNewest) {
            *reason = "no complete frame";
            return view;
        }

        const crafterhunter::frame::SlotHeader& slot = slots[pick];

        const std::size_t offset = crafterhunter::frame::pixelsOffset(pick, slot.width, slot.height);
        if (offset + crafterhunter::frame::frameBytes(slot.width, slot.height) > length()) {
            // The guest rewrote the file at a new size while we held the old
            // mapping. Its own rules say a resized window gets a new buffer, so
            // remap and let the next frame through rather than reading past the
            // end of this one.
            *reason = "frame lies outside the mapping; remap needed";
            return view;
        }

        view.sequence = slot.sequence;
        view.capturedNanos = slot.capturedNanos;
        view.width = slot.width;
        view.height = slot.height;
        view.pixels = static_cast<const std::uint8_t*>(view_) + offset;
        *reason = nullptr;
        return view;
    }

private:
    MappingHandle mapping_{};
    void* view_{nullptr};
    std::size_t length_{};
    bool mapped_{false};
};

}  // namespace crafterhunter::frames
