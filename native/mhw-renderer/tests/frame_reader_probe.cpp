// A native reader, driven from the command line, so a Java-side test can check what
// the *other* half of the transport actually sees.
//
// This exists because the guest's lifecycle checks and the native reader had only ever
// been tested apart. A predicate that says "the channel file is gone" can be correct
// while the reader still sees an old frame, and the reverse: the guest can report a
// healthy channel while the reader is looking at an inode nothing writes to. Neither
// bug is visible from one side, which is exactly why this half exists.
//
// Prints one line and exits:
//   SEQ=<n> W=<w> H=<h> BYTES=<n> REASON=<text>
// with SEQ=0 and a REASON when no frame is available. Exit status is 0 when a frame was
// read and 1 when not, so a shell can branch on it too.
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "frame_clock.hpp"
#include "frame_source.hpp"

int main(int argc, char** argv) {
    // Optionally age the frame past the stall bound, so the liveness rule can be
    // exercised: `--stale-ms N` reports what happens N milliseconds after capture.
    long long staleMillis = 0;
    const char* path = nullptr;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--stale-ms") == 0 && i + 1 < argc) {
            staleMillis = std::atoll(argv[++i]);
        } else if (std::strcmp(argv[i], "--path") == 0 && i + 1 < argc) {
            path = argv[++i];
        }
    }

    crafterhunter::frames::Source source;
    if (!source.open(path)) {
        std::printf("SEQ=0 W=0 H=0 BYTES=0 REASON=channel could not be opened\n");
        return 1;
    }

    const std::uint64_t now =
        crafterhunter::clock::millisToNanos(1'000'000) + static_cast<std::uint64_t>(staleMillis) * 1'000'000ull;
    const char* reason = nullptr;
    const crafterhunter::frame::FrameView view = source.newestFrame(now, &reason);
    if (!view.pixels) {
        std::printf("SEQ=0 W=0 H=0 BYTES=%zu REASON=%s\n", source.length(),
            reason ? reason : "unknown");
        return 1;
    }

    std::printf("SEQ=%llu W=%u H=%u BYTES=%zu REASON=none VALID=%d\n",
        static_cast<unsigned long long>(view.sequence), view.width, view.height,
        source.length(), source.headerValid() ? 1 : 0);
    return 0;
}
