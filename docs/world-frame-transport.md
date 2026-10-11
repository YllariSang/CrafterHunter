# Paired world snapshot v3 (v2 read compatibility)

Current checkpoint (2026-10-11): full-resolution paired composition and the real
player skin are visually accepted in all three F5 modes. Focused Minecraft
measured 48.3 publishes/s; the host observed 39.5 uploads/s at roughly 40 fps.
These are cadence measurements, not end-to-end latency. See the latest MODLOG
entries for evidence. The 60 fps spike gate remains open.

Both pacing floors are negotiated by flag content (milliseconds, 0 means no
floor; unreadable content falls back to 250 ms). WorldCapture retains one copy
pair in flight and permanent failure after uncertain GPU completion. Only complete
BOTH captures with matched issue-time camera metadata publish a paired snapshot.
The existing colour-only channel and native stone renderer are untouched.

## Once-per-frame composition requests (2026-10-11)

A matched ON/OFF/OFF/ON comparison kept capture/upload active and only toggled
full paired/player drawing. CH_Frame callback rates were 50.93/25.44/26.29/52.42
per second, with the guest publishing about 30/s throughout. GPU utilization fell
from 95–100% ON to 29–44% OFF. These are render callback counts, not independent
Present fps or E2E latency. Fresh screenshots/logs and the exact script are local
under `captures/composition-ab-20261011-202345/`; see MODLOG for conditions/limits.

The native renderer now samples full/preview/player enable files once per
CH_Frame. Draw dispatch and logs share that state, applying toggles on the next
frame. Previously disabled/refused drawing repeatedly queried these files at
eligible UI draw calls because no successful-draw latch had been set. Draw
refusals still retry; depth, projection, freshness and fixed-F5 anchor gates are
unchanged. `bash tools/test-composition-requests.sh` exposes repeated queries with
1,000 refused callbacks and checks next-frame toggles, independent modes and
reset. Build and relevant regressions pass; installed cmp-clean with scoped backup
`native/mhw-renderer/build/rollback.vQD9KX`. Live impact on cadence is unverified;
repeat the comparison before claiming improvement. The 60 fps gate remains open.

Live repeat after restart: `captures/composition-ab-20261011-203655/` preserves
screenshots/logs and the same script. CH_Frame rates ON/OFF/OFF/ON were
39.13/44.08/42.88/41.38 per second; guest publishes changed from 48.97 in ON 1 to
about 30/s thereafter. The previous OFF collapse was absent, but changed producer
rate, camera/weather and cross-session conditions prevent a causal before/after
speed claim. Fixed epoch 1220/generation 10018048051975, no invalidation, visible
full paired/player draws, flags restored. This is basic runtime regression
evidence, not new fine pixel/depth acceptance or 60 fps.

The actual 26.2 FramerateLimitTracker bytecode confirms AFK mode caps rendering
at min(maxFps,30) after 60 seconds without input, even while focused, then 10 fps
after ten minutes. User options select AFK. MINIMIZED skips those idle thresholds
while retaining the iconification limit. For a still-player cadence comparison,
obtain approval to select Minimized temporarily, keep both games visible and
restore AFK afterward. No setting was changed during the recorded repeat.

With the user-approved temporary Minimized setting, repeat 204326 held guest
publishing at 46.65/46.45/46.35/46.01 per second through ON/OFF/OFF/ON. Host
callbacks were 38.16/39.11/39.01/38.11 per second. Capture/upload stayed active;
turning paired/player drawing off alone did not reach 60. This isolates the
draw-toggle effect under roughly constant producer cadence, not vanilla MHW or
the cost of capture/upload/hooks. No sole CPU/GPU limiter is established.
Screenshots, fresh logs, exact script and results are in ignored
`captures/composition-ab-20261011-204326/`. Epoch 1220/generation 10018048051975
stayed armed, with no invalidation. Draw flags were restored exactly; the user
restored AFK, confirmed in options.txt. Next profile capture/upload and host hook
work before selecting another bounded change. The 60 fps gate remains open.

## Resident native upload pair (2026-10-11)

The next bounded cadence change replaces per-capture immutable texture/SRV
allocation with one resident RGBA8 UNORM/R32_FLOAT pair. Textures use DEFAULT
usage and only SHADER_RESOURCE binding. On a same-size, same-generation advance,
the immediate context queues two full-image UpdateSubresource calls with the
snapshot's tightly packed row pitch, then publishes that capture's metadata.
Subsequent draws use both updated attachments. Startup, resize, generation change
and watchdog/disable reset allocate a new pair; creation is private until both
textures and both views succeed. Failure or device removal clears the drawable
pair and calibration. Identity selection, watchdog and F5 anchor behavior remain
unchanged. No host depth resource is updated.

[D3D11's UpdateSubresource contract](https://learn.microsoft.com/en-us/windows/win32/api/d3d11/nf-d3d11-id3d11devicecontext-updatesubresource)
allows source memory to be released when the call returns. Outstanding draws may
cause driver-managed intermediate copies; reuse removes application texture/view
allocation, but does not guarantee a frame-time improvement. There is no explicit
GPU wait, Map, Flush, new queue or change to Minecraft's GPU lifetime policy.

Headless check: `bash tools/test-world-upload.sh`. 120 advancing same-size captures
create two textures/two views once, then perform 240 ordered attachment updates.
Disabling reuse reproduces the old allocation path and fails that regression.
Tests also cover exact row/attachment bytes, source release, equal-byte-count
resize, producer replacement, reset, all four creation-failure boundaries and
malformed-pair refusal. These prove upload policy, not D3D/DXVK pixels or speed.
The resident-pair change is built and installed cmp-clean with scoped backup
`native/mhw-renderer/build/rollback.W0pIsw`. Basic live regression evidence now
exists: the user's 25.28s recording `recording_2026-10-11_20.05.44.mp4` shows guest
surfaces and walking Steve through all three F5 modes. The fresh log keeps epoch
1646/generation 8042902613435 through those switches. A separate later 45-second
first-person log window contains 1,766 uploads (39.24/s), all textures=updated,
zero creations and no invalidations. Four full-resolution draw reports contain
403–424 submissions per approximately 10 seconds, with 14–15ms receipt age.
Median consumer observation spacing is 24ms, not measured E2E latency.

Original-resolution extracted screenshots/logs are retained locally under
`captures/resident-pair-20261011-200544/` (ignored by git). The 1920x1080 recording
shows both games tiled on the desktop; it is not a native 1920x1080 MHW-only
capture. No new fine skin/depth/occlusion acceptance follows from it. The figures
show reuse and drawing at roughly the prior rate; there is no controlled A/B,
demonstrated fps gain or 60 fps claim. The recording's ~37 encoded frames/s is the
recorder's rate, not an independent MHW fps measurement. Next isolate MHW frame
cost under matching conditions before another tuning change.

The file `/dev/shm/crafterhunter/world.frame` contains one little-endian header,
bottom-up RGBA8 colour, then bottom-up raw D32_FLOAT window depth. Source rows
are tightly packed; matrix coefficients are column-major. Clip-control mode/origin
and depth range are explicit: the matrix alone cannot define the window-depth
mapping. Never compare these floats directly to MHW depth.

| Offset | Field |
| --- | --- |
| 0, 4, 8, 12 | u32 magic `0x50574843`, version 3, header size 320, flags 1 (bottom-up) |
| 16, 20, 24, 28 | u32 width, height, row stride, reserved zero |
| 32, 40, 48 | u64 generation, capture identity, issue-time Java monotonic nanos |
| 56, 64 | u64 colour and depth byte counts |
| 72, 76 | f32 near, far |
| 80 | 16 f32 projection coefficients |
| 144 | five f32 camera x, y, z, pitch, yaw in Minecraft units/degrees |
| 164, 168 | u32 clip mapping (1 zero-to-one, 2 minus-one-to-one), origin (1 lower-left, 2 upper-left) |
| 172 | f32 clear depth 0 (pinned 26.2 main-pass clear) |
| 176 | 16 f32 actual level view-rotation coefficients |
| 240 | three f64 actual render camera x, y, z |
| 264 | u32 native camera mode (0 first-person, 1 rear third-person, 2 front third-person) |
| 268, 272, 276 | f32 depth-range min/max (currently requires 0/1), u32 provenance 1 (render arguments) |
| 280 | three f64 player feet x, y, z, interpolated at the level render's partial tick |
| 304 | u32 player provenance 1 (local player is the active camera entity) |
| 308–319 | reserved zeros |
| 320 | colour, followed immediately by depth |

Version 1 is rejected. Version 2 still reads for archived camera/depth evidence,
but lacks player provenance and cannot calibrate or draw the player-anchored
preview. Missing player references publish v2, never a fabricated v3 origin.
Rebuild/restart Fabric, managed plugin and native renderer together for v3.
Player feet are copied at LevelRenderer HEAD using the level delta's partial tick,
then frozen with the view/camera/projection for that capture identity, not sampled
when GPU callbacks finish. Nonfinite feet or unknown provenance refuse v3.
Exact camera
position plus view rotation specifies `eye = rotation * (guestWorld - cameraPos)`.
The projection is the final GameRenderer upload argument, after bob/hurt/nausea
effects, not the earlier camera projection. LevelRenderer's actual camera and view
arguments are copied before it renders; at readback issue these values are frozen
for that capture identity. No player-eye or resampled-state fallback is published
on v2. Missing view/projection or unavailable GL clip queries refuse paired transport
while retaining diagnostic output and leaving the previous atomic snapshot intact.
The host watchdog still expires that old file; it must not be treated as an update.
Clear-depth provenance is bytecode-pinned, not a query of unrelated GL clear state.

Each attachment is width × height × 4 bytes. Dimensions are capped at 4096 each;
one snapshot is at most 128 MiB plus header. One private same-directory temporary
and one published snapshot bound on-disk producer storage to two snapshots during
publication. The complete private file is closed before atomic replacement;
failure removes only its own temporary and keeps the previous published file.
No live channel is deleted. Diagnostics still publish if this transport is refused.

The portable C++ reader opens once, validates the header and exact file size before
allocating, then reads both attachments through that same handle. Atomic inode
replacement prevents mixing header/colour/depth generations on Linux. A refusal
never replaces the caller's prior output. Successful structural parsing does not prove valid depth,
freshness, camera alignment or renderability. Issue nanos are a Java-process clock,
not a cross-process latency measurement. A future compositor must use local receipt
freshness plus explicit generation/identity handling, and reject stale snapshots.

Headless check: `bash tools/test-world-frame-transport.sh`. Java writes actual
binary fixtures read by C++; verifies matrix/pose/depth offsets, complete pair,
restart/resize, concurrent publication, wrong payload refusal and eight malformed
native-reader cases (now twelve), all three native camera-mode values, exact f64
position/view offsets and rejection of absent provenance. These are transport
fixtures, not replacement game geometry.
Windows/Proton file sharing and live producer/consumer behaviour remain unverified.

## Historical upload-only checkpoint (2026-10-08)

The following records the original diagnostic before composition and negotiated
cadence. The current checkpoint and resident upload change above supersede its
upload-only, fixed-four-Hz and per-advance immutable-allocation descriptions.

Host-side fresh-snapshot selection and upload are now implemented, not runtime
accepted. The diagnostic is off by default: existence of
`nativePC/plugins/CSharp/CrafterHunter/render/world-upload.enabled` opts in.
It polls at most four times per second, opens each atomic snapshot anew (no frozen
inode mapping), and requires identity advance after startup or generation change.
The first file seen is warmup, never presumed fresh. Duplicate identities do not
refresh a one-second host-local watchdog; lower identities and retired generations
are refused. Four retired generations are remembered; further churn disables
selection until the diagnostic is disabled/re-enabled or the renderer restarts.
This bound prevents old generations from being forgotten and accepted as new.

Both immutable D3D11 textures and SRVs are created privately; only a complete pair
replaces staged resources and matching metadata. Colour is RGBA8 UNORM and depth
is R32_FLOAT, still raw bottom-up guest data. CPU attachment storage is released
after upload. Missing/malformed input, warmup, refusal, expiry or GPU failure clears
the staged pair. No shader binds these views: neither stone nor diagnostic sky-only
composition changes. Logs identify warmup, uploaded generation/identity/size, and
clear events. This path copies whole snapshots and allocates textures per advance:
it is request-paced verification scaffolding, NOT the final 60 fps architecture.
Single static captures normally expire before another manual request arrives.
Future live checks must use advancing paired requests, inspect these logs and
verify GPU contents; successful cross-compilation does not establish uploads.

CPU projection inversion/reprojection is now implemented and archived-block checked;
see [world-reprojection.md](world-reprojection.md). Native staging prepares the
matching inverse projection and eye-to-guest transform, but no reprojection shader
draws yet. View/depth provenance is implemented, not runtime accepted; next obtain
fresh v2 captures in each F5 mode and establish anchor conversion against real
host camera/depth. Guest layer isolation and
input ownership remain separate gates; this snapshot still contains Minecraft
terrain/sky, not isolated Steve.
