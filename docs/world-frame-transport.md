# Paired world snapshot v1

Implemented, headless-tested; not installed or runtime-composited.
The existing live colour-only channel and native stone renderer are untouched.
WorldCapture publishes this only after a requested BOTH capture completes with
recorded projection metadata. This is not an automatic 60 fps stream.

The file `/dev/shm/crafterhunter/world.frame` contains one little-endian header,
bottom-up RGBA8 colour, then bottom-up raw D32_FLOAT window depth. Source rows
are tightly packed; matrix coefficients are column-major. Near/far and the matrix
define the depth convention. Never compare these floats directly to MHW depth.

| Offset | Field |
| --- | --- |
| 0, 4, 8, 12 | u32 magic `0x50574843`, version 1, header size 256, flags 1 (bottom-up) |
| 16, 20, 24, 28 | u32 width, height, row stride, reserved zero |
| 32, 40, 48 | u64 generation, capture identity, issue-time Java monotonic nanos |
| 56, 64 | u64 colour and depth byte counts |
| 72, 76 | f32 near, far |
| 80 | 16 f32 projection coefficients |
| 144 | five f32 camera x, y, z, pitch, yaw in Minecraft units/degrees |
| 164–255 | reserved zeros |
| 256 | colour, followed immediately by depth |

Each attachment is width × height × 4 bytes. Dimensions are capped at 4096 each;
one snapshot is at most 128 MiB plus header. One private same-directory temporary
and one published snapshot bound on-disk producer storage to two snapshots during
publication. The complete private file is closed before atomic replacement;
failure removes only its own temporary and keeps the previous published file.
No live channel is deleted. Diagnostics still publish if this transport is refused.

The portable C++ reader opens once, validates the header and exact file size before
allocating, then reads both attachments through that same handle. Atomic inode
replacement prevents mixing header/colour/depth generations on Linux. A refusal
never replaces the caller's prior output. This reader is deliberately NOT wired
into renderer.cpp: successful structural parsing does not prove valid depth,
freshness, camera alignment or renderability. Issue nanos are a Java-process clock,
not a cross-process latency measurement. A future compositor must use local receipt
freshness plus explicit generation/identity handling, and reject stale snapshots.

Headless check: `bash tools/test-world-frame-transport.sh`. Java writes actual
binary fixtures read by C++; verifies matrix/pose/depth offsets, complete pair,
restart/resize, concurrent publication, wrong payload refusal and eight malformed
native-reader cases. These are transport fixtures, not replacement game geometry.
Windows/Proton file sharing and live producer/consumer behaviour remain unverified.

Next bounded step: host-side fresh-snapshot selection and upload, then projection
normalisation/reprojection with real host camera/depth. Guest layer isolation and
input ownership remain separate gates; this snapshot still contains Minecraft
terrain/sky, not isolated Steve.
