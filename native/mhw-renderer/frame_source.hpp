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

// The same path as a Windows drive letter, for the reader inside Proton.
//
// The reader is a Windows process, so it hands CreateFileA something Wine must resolve.
// Wine accepts a Unix-style absolute path and maps it onto Z:, but relying on that alone
// means a single unresolved spelling is indistinguishable from "the guest has not
// published yet" - the same silent failure this file has already produced twice. Both
// are tried, and which one worked is reported rather than guessed at. Z: is Proton's
// mapping of /, so both name the same host file.
inline constexpr char ChannelPathUnix[] = "/dev/shm/crafterhunter/frame.channel";
inline constexpr char ChannelPathDrive[] = "Z:\\dev\\shm\\crafterhunter\\frame.channel";
inline constexpr int ChannelPathCandidates = 2;

// The spelling at `index`, or the Unix one when the index is out of range.
inline constexpr char const* channelPathAt(int index) {
    return index == 1 ? ChannelPathDrive : ChannelPathUnix;
}

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
    // `path` defaults to the fixed channel path and is a parameter only so a test can
    // point a reader at its own file. A runtime-configured path in production would be a
    // second source of truth about where the two processes meet, which is the one thing
    // this file exists to avoid.
    bool open(const char* path = ChannelPath) {
        close();
        if (!path || path[0] == '\0') return false;
        // Both platforms produce a byte count and a view. The length is assigned once,
        // after the #if, so it cannot be forgotten on one of them.
        //
        // It was. Windows read the size with GetFileSizeEx and then discarded it, and
        // `length_` was only ever assigned in the POSIX branch - so on Windows it stayed
        // 0, every frame failed the "shorter than its headers" check, and Steve could
        // never appear however many frames were published. No host test could see it,
        // because Linux compiles the branch that was wrong.
        std::uint64_t bytes = 0;
#if defined(_WIN32)
        HANDLE file = INVALID_HANDLE_VALUE;
        pathSpelling_ = 0;
        for (int attempt = 0; attempt < frames::ChannelPathCandidates; ++attempt) {
            // The configured path is tried verbatim first; the drive-letter spelling is
            // only a fallback for the fixed path, so a test's own file is never
            // rewritten into a different name.
            const char* candidate =
                (attempt == 0 || path != ChannelPath) ? path : frames::channelPathAt(attempt);
            file = CreateFileA(candidate, GENERIC_READ,
                FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL, nullptr);
            if (file != INVALID_HANDLE_VALUE) {
                pathSpelling_ = attempt;
                break;
            }
        }
        if (file == INVALID_HANDLE_VALUE) return false;
        LARGE_INTEGER size{};
        if (!GetFileSizeEx(file, &size)) { CloseHandle(file); return false; }
        if (size.QuadPart <= 0) { CloseHandle(file); return false; }
        // The real file length, not the mapping's. See the note after the #if.
        bytes = static_cast<std::uint64_t>(size.QuadPart);
        mapping_ = CreateFileMappingA(file, nullptr, PAGE_READONLY, 0, 0, nullptr);
        CloseHandle(file);
        if (mapping_ == nullptr) return false;
        view_ = MapViewOfFile(mapping_, FILE_MAP_READ, 0, 0, 0);
        if (view_ == nullptr) { CloseHandle(mapping_); mapping_ = nullptr; return false; }
#else
        const int fd = ::open(path, O_RDONLY);
        if (fd < 0) return false;
        struct stat st{};
        // An empty file is refused rather than mapped. The guest creates the channel and
        // fills it in one constructor, so an empty one means we caught it mid-creation;
        // mapping it yields a zero-length section whose every read fails, which is a far
        // more confusing symptom than "not there yet".
        if (fstat(fd, &st) != 0 || st.st_size <= 0) { ::close(fd); return false; }
        bytes = static_cast<std::uint64_t>(st.st_size);
        void* mapped = mmap(nullptr, static_cast<std::size_t>(bytes), PROT_READ, MAP_SHARED, fd, 0);
        ::close(fd);
        if (mapped == MAP_FAILED) return false;
        view_ = mapped;
#endif
        // The one and only assignment, after the platform split.
        //
        // It is the file's length, not the mapping's. VirtualQuery would report the
        // region size, which is rounded up to the allocation granularity - for a
        // 15,691,648-byte channel that is thousands of bytes more than the file holds.
        // `within()` uses this number to decide whether a frame's pixels fit, so a
        // rounded-up length would accept a frame extending past the real end of the
        // file: no crash, since the mapping is larger, but pixels of nothing.
        length_ = static_cast<std::size_t>(bytes);
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

    // Does this mapping hold a channel header we recognise?
    //
    // Deliberately distinct from "is there a frame in it": a channel can be perfectly
    // valid and hold nothing publishable yet, which is the normal state before the
    // guest's first frame. The recovery path needs to tell those apart - a valid header
    // over an unchanged sequence is a stuck writer, while an unrecognisable one means
    // something else is at this path - and inferring it from whether a frame came back
    // is a guess that produces the wrong verdict exactly when it matters.
    bool headerValid() const {
        if (!mapped_ || !channelLongEnough(length())) return false;
        const auto* bytes = static_cast<const std::uint8_t*>(view_);
        const auto* header = reinterpret_cast<const crafterhunter::frame::BufferHeader*>(bytes);
        return header->magic == crafterhunter::frame::Magic
            && header->formatVersion == crafterhunter::frame::FormatVersion;
    }

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

    // How many bytes the channel file actually contains.
    //
    // One implementation for both platforms, because a platform-specific version of this
    // is what went missing before. It is deliberately the *file* length rather than the
    // mapping's: VirtualQuery reports a region rounded up to the allocation granularity,
    // and treating that as the file length would let `within()` accept a frame whose
    // pixels run past the end of the file.
    std::size_t length() const { return view_ == nullptr ? 0 : length_; }

    // Which path spelling opened the channel: 0 for the Unix-style one, 1 for the drive
    // letter. Reported in the log, because "which of these did Wine accept" is not
    // something worth guessing at from a screenshot.
    int pathSpelling() const { return pathSpelling_; }

    // The newest complete frame, or a frame with null pixels when there is
    // nothing to draw. Never guesses: every refusal returns a null pointer and a
    // reason the caller can log.
    //
    // The host's resolution is deliberately absent. An earlier version took it and
    // required the slot's geometry to match, which refused every frame: the guest
    // is 1908x1028 and the host is 1920x1080, and they are never going to be equal.
    // The frame is validated against its own declared geometry and against how much
    // of this mapping is actually there.
    //
    // `nowNanos` is accepted but deliberately unused for the age decision. This
    // function cannot judge freshness: the guest stamps frames with a clock measured
    // 3,422,487 ms from ours, so comparing the two here would refuse every frame that
    // was ever published. The caller decides, using frame_clock.hpp.
    crafterhunter::frame::FrameView newestFrame(std::uint64_t nowNanos,
        const char** reason) const {
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
        const char* why = nullptr;
        const std::uint32_t pick = crafterhunter::frame::newest(header->magic,
            header->formatVersion, slots, length(), &why);
        if (pick == crafterhunter::frame::NotNewest) {
            *reason = why ? why : "no complete frame";
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
    int pathSpelling_{0};
};

// How often the reader may re-open the channel.
//
// Bounded rather than eager because the reader is called every frame: re-opening
// on every refusal would turn a missing channel into a syscall storm, and the
// channel is missing for as long as Minecraft is closed. Once a second is enough
// to recover from an unlinked or recreated file - the guest rewrites it in
// milliseconds - while costing nothing when the guest is simply not running.
inline constexpr std::uint64_t RemapIntervalNanos = 1'000'000'000ull;

// May the channel be re-opened right now?
//
// The clock going backwards counts as due, and so does never having tried: a reader
// whose own time source jumped should not then refuse to recover forever because
// its subtraction never reaches the interval.
inline bool mayRemap(std::uint64_t nowNanos, std::uint64_t lastAttemptNanos) {
    if (lastAttemptNanos == 0) return true;
    if (nowNanos < lastAttemptNanos) return true;
    return nowNanos - lastAttemptNanos >= RemapIntervalNanos;
}

// What to do about a channel that is not yielding a frame.
//
// Re-opening is the only way back from three states, and all three end in the same
// symptom - no frame arrives - while needing the same remedy:
//
//   - the file was unlinked while the mapping was held, so the mapping describes an
//     inode nothing writes to any more. This is not hypothetical: a verification
//     script deleted the live channel and the guest carried on publishing to it.
//   - the guest recreated the file, at a new size after a window change.
//   - the guest restarted, so the sequence numbers began again from one.
//
// None of them is visible from inside the mapping. `haveFrame` false therefore means
// "the mapping may be describing the past", and the answer is to re-open it - but
// only once per interval, or a closed Minecraft would cost a remap per frame.
// What the reader remembers about the writer it is reading from.
//
// This survives a channel re-open, and that is the point: the memory is what decides
// whether a frame is fresh, so throwing it away on every re-open would throw away the
// only defence against a frozen frame.
struct FrameMemory {
    std::uint64_t sequence = 0;   // the newest sequence seen, 0 if none
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::uint64_t advanceNanos = 0;  // when the sequence last changed, on our clock
    bool established = false;       // whether this memory describes a real writer
};

// Why the channel looks different from what we last read.
//
// The distinction is between a channel that is *the same writer, stuck* and one that is
// *a different writer*. Both present as "no frame arriving", and re-opening the file
// separates them - but only if the reader does not then treat the unchanged file as a
// fresh start. That mistake is what let a frozen frame be resurrected once per second
// indefinitely: re-opening succeeded, the liveness clock was zeroed, and the very next
// observation of the same unmoving sequence looked like progress.
enum class Generation {
    Same,       // the same writer, still stuck: keep the memory, keep refusing
    Restarted,  // sequence numbers went backwards: a new writer began at one
    Resized,    // the geometry changed: a new buffer
    Replaced,   // not even our channel: a different file
};

inline Generation classifyGeneration(const FrameMemory& memory, bool magicOk, bool versionOk,
                                    std::uint64_t sequence, std::uint32_t width,
                                    std::uint32_t height) {
    if (!magicOk || !versionOk) return Generation::Replaced;
    if (!memory.established) return Generation::Restarted;
    if (width != memory.width || height != memory.height) return Generation::Resized;
    // Backwards is the signal that a counter restarted. Equal is the signal that nothing
    // happened, which is the case this whole function exists to keep distinct.
    if (sequence < memory.sequence) return Generation::Restarted;
    return Generation::Same;
}

// Should the reader forget what it knows? Only for a genuinely new generation.
//
// No memory argument: classifyGeneration already reports an unestablished reader as
// Restarted, so the generation alone decides it.
inline bool shouldForget(Generation generation) {
    return generation != Generation::Same;
}

inline bool shouldRemap(bool haveFrame, std::uint64_t nowNanos,
                        std::uint64_t lastAttemptNanos) {
    if (haveFrame) return false;
    return mayRemap(nowNanos, lastAttemptNanos);
}

}  // namespace crafterhunter::frames
