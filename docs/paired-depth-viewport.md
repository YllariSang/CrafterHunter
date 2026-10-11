# Paired world depth viewport

2026-10-12: the full and preview world compositors now convert host pixel centres
to the selected scene depth's measured writer viewport before integer Load.
The player already used this conversion; all three paths share its existing
current-frame validity checks. Verified ef56144 native renderer is installed;
the user accepts improved environment occlusion after all-F5 movement and F6
release. Steve's own occlusion flicker remains open. All three installed artifacts cmp
match their tested builds; guest/managed were preserved. Scoped backup:
`native/mhw-renderer/build/rollback.paired-depth-viewport.pg7yujaz/`, with the
pinned installer additionally saving `rollback.WSF4r3/`. Both games exited before
installation under the user's explicit closure permission. Control starts OFF;
composition, body separation, cadence flags and live channels are preserved.

Fresh evidence is in ignored
`captures/composition-timing/runtime-20261011-214505Z/`. Generation 42796309633712,
one calibration epoch 1498 and token 2179040981719813127 stayed ACTIVE throughout
the 30-second first-person timing window. Loaded/installed artifacts matched the
tested pre-change builds by cmp. Guest metadata preparation is approximately
0ms over 873 advances, versus about 9.03ms in the previous mixed-view session.
This confirms removal of that work, not a controlled speed or screen-latency gain.
Other approximate guest means: 25.05ms issue-to-callback, 0.76ms callback-to-service,
1.24ms buffer copy, 5.35ms publication and 32.51ms issue-to-publication-return.
Native weighted means: read 4.61ms, duplicate-read 5.00ms, upload-submit 0.47ms,
service 6.39ms. Clock domains remain separate.

Native capture 43203517 has a 1920x1080 scene depth allocation with geometry only
in its 1440x810 writer rectangle. The right and bottom unused storage is entirely
zero. Read-only native state independently records writer viewport
(0,0,1440,810), selected depth 0. Original full-size colour shows the world layer
overlapping host foreground at those borders. Pre-UI GPU camera traces 3/4/5
agree exactly with the stationary CPU VP in X/Y/W; reversed-Z is kept native.
These observations span frames, so they do not prove synchronization in motion.

The world shaders previously sampled `floor(SV_Position.xy)` directly. A pixel
such as (1800.5,1000.5) therefore read clear storage rather than scene depth at
(1350,750), allowing foreground overlap. Both paired shaders now use
`floor(pixel * writerSize / targetSize + writerOrigin)`, exactly as the player.
Constants carry the measured scale/origin. No fixed 0.75 scale is stored.
Missing, stale, nonfinite or out-of-bounds writer viewports refuse drawing.
The strict reversed-Z comparison, source footprint/plane reconstruction, private
guest depth, same-capture body pose, camera lease and fixed F5 anchor remain.

`python3 tools/test-paired-depth-viewport.py` executes the actual production CPU
preparation helper in a headless fixture and evaluates both actual shader address
expressions. It covers measured reduced resolution, identity/offset viewports,
edge pixel centres, stale/invalid refusal and foreground-vs-clear-storage rejection.
Restoring direct full-size shader addresses fails. Native Release, all 13 shader
entries, paired preview/coverage/plane, player preview/body, depth selection and
guest-camera checks pass. This is one correction to a measured spatial defect;
the user's all-view jumpiness still needs movement screenshots and fresh logs.

Next reopen loaded offline scenes, focus Minecraft in first person with F7/F9
OFF, calibrate once for the fresh session, then walk/look in all three F5 views.
Check that the hunter and other foreground geometry hide guest surfaces through
the entire target, and press F6 once afterward. Never re-anchor on F5.

## First installed-build observation

Fresh generation 43698606417375, calibration epoch 1763, matching installed and
loaded native artifacts by cmp. Ignored evidence:
`captures/composition-timing/runtime-20261011-215804Z/`. Full-size pre-UI traces
show foreground occlusion continuing through the right/bottom target edges;
GPU camera X/Y/W matches the stationary CPU matrix exactly. Fine floor/body
dithering remains visible. These are stationary samples, not all-view acceptance.

The existing detailed `trace` request blocked the native render thread for more
than the camera's 500ms lease. Guest sequences kept advancing and its request flag
remained present, but the native guard correctly latched release at sequence 38.
This was diagnostic interference, not a user F6 check or proof of a movement bug.
Do not use that synchronous multi-buffer trace during an ACTIVE acceptance run.
No lease/freshness guard was weakened. An explicit fresh control request recovered
with the same generation and epoch, token 893139907560709530. Lighter native
capture 44234092 completed while control remained ACTIVE. The user walking/F5/F6
check and bounded screenshot/identity collector are pending; no re-anchor occurred.

## Environment acceptance

User reports: "Environment occlusion is stable and better, f5 works and f6
releases. The only difference is that Steve has the jumpy occlusion now" and
permits deferring that remaining issue if not substantial. This accepts the world
viewport correction, preserving the distinction from unstable body occlusion.

The reactivated run collected 1872 samples over 211.41s: 1860 matching ACTIVE,
12 matching released, zero errors and zero alignment invalidations. Generation
43698606417375 and epoch 1763 remained fixed. Original 1920x1080 captures and
moving feet exist in all F5 modes; rear spans 7.73s, front 11.18s. Every sampled
captured-body foot position matches the atomic world capture. F6 removes the
guest request flag and produces a matching released acknowledgement at sequence
9153. Explicit camera/filter cleanup confirms armed=0; channels/anchor preserved.
Games remain open with control OFF. No exact fps, screen latency or same-frame
dynamic GPU camera/depth pairing is accepted from these observations. Steve's
remaining flicker is recorded for a separate rendering follow-up.
