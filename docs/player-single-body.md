# One local player body

2026-10-12: implemented, headless verified and matching builds installed;
live acceptance pending. Scoped backup:
`native/mhw-renderer/build/rollback.single-body.3_t40x8e/`. Control starts OFF.

Follow-up: user reports all cameras work and F6 stops control, but cape and held
item lag behind the body during movement. Fresh matching release was observed;
the bounded collector only captured stationary first person before that later
manual test. All-F5 screenshots and attachment/occlusion acceptance remain open.
The issue-time pose fix below is built/headless verified, awaiting scoped install.

The paired world previously included Minecraft's third-person player body while
the native renderer drew the exported body again. Keep the real baked model and
its Minecraft pose/skin as the body renderer. Exclude only the local main-body
submission from paired capture. Held items, armor, capes, shadows, other entities
and terrain remain in the captured world. First-person hands remain outside the
existing pre-hand capture boundary; this change does not introduce a hand export.

## Capture and normal display

`control-player-preview.py start` enables `player-body-separate.enabled` alongside
player export/composition. Stop removes that flag without deleting live channels.
The guest requires a recent successful body export and the local player owning
the render camera before separation. Disabled/failed/unavailable export uses the
ordinary captured scene instead. Existing producer freshness and permanent
GPU-copy failure handling remain in force.

Installed 26.2 `LivingEntityRenderer.submit` has one main-model submit, followed
by separate attachment-layer submits. Retag only that main submit when its model
is `PlayerModel` and `AvatarRenderState.id` equals the local player's ID. A private
RenderType batching identity uses exactly the original RenderSetup; its outline
gets a separate identity too. Only those prepared body draws are omitted during
the capture pass, including their depth writes. No alpha mask, recoloring or
post-capture hole punching is used.

When a third-person paired capture is due, execute one already-prepared level
frame graph with the body omitted. Issue the existing color/depth copies before
the optional always-on-top depth clear, or immediately after the graph if that
pass is absent. Then replay the prepared graph with the body restored for the
normal Minecraft display. Simulation, state extraction, pose publication and
feature preparation occur once. GameRenderer's hand/HUD passes occur afterward
as usual. The first-person or no-capture case executes the graph once.

The pinned frame graph appends its resource acquisition schedule on execute;
naively executing it twice tries to acquire an already assigned resource. After
a successful capture graph, clear only that graph's completed acquisition/release
plan before the display execution. Physical resources are released/reacquired by
Minecraft's allocator, and feature buffers stay alive until PreparedFrame.close
after both executions. The actual-class test verifies bounded live resources,
complete background/attachments, display restoration and copy/display ordering.
A failed graph is never replayed with uncertain resources. A failed capture
callback after a successful graph still restores display; existing WorldCapture
copy failures retain their buffers and disable capture until restart.

This adds an extra GPU scene draw only for due third-person captures. Its runtime
cost is unmeasured. It is a correctness slice, not a performance improvement.

## Host selection and occlusion

Paired snapshot v4 keeps the 320-byte layout and adds flags bit 1: the local body
was excluded at issue time. Normal v4 frames have flags 1; separated frames have
flags 3. Legacy v2/v3 archives remain readable but cannot claim exclusion.
Unknown flags and exclusion without local-player provenance are refused.
Metadata reports `localPlayerBody`; native upload logs report `local-body`.

While paired composition is requested, unfiltered frames already contain the
player, so native body drawing is suppressed. Filtered frames allow the model
only after paired drawing produced private guest depth for the same host frame,
guest generation/capture identity, alignment epoch, target and camera buffer.
The player shares that guest DSV without clearing it. Minecraft blocks and
captured held items/armor can therefore cover the model using normal reversed-Z
depth testing, while the existing pixel shader still tests MHW scene depth.
MHW's depth is never written. Standalone model preview retains its own private
self-depth target. Actual F5 camera handling, first-person body suppression,
fixed anchors and input/F6 control are unchanged.

## Body and attachment timing

The former separated body read newest `player.pose`, independently of the older
GPU-captured attachment pixels. Even zero cadence floors allow readback and
publication to span newer player poses, so their feet and animation can disagree.
Do not infer GPU latency from packet receipt age.

Snapshot v5 retains the 320-byte header and unchanged colour/depth offsets.
Header offset 308 holds a u32 body-pose byte count; bytes 312–319 remain zero.
After depth, append the actual serialized `player.pose` sampled during this level
render invocation and cloned at readback issue. Its existing version-1 format
includes generation, asset identity, pose sequence, interpolated feet and bounded
affine bone matrices/visibility. At most 4416 bytes are appended. It is frozen
with the issue-time camera and published by the same atomic file replacement.
Completion never reads the later pose channel. Metadata reports `bodyPoseBytes`;
native upload logs report `body-pose` sequence.

Only a successful body export in the current render invocation permits separation;
the per-frame reference is cleared before cadence/disabled checks. If unavailable,
retain the ordinary captured body and suppress its separate draw. Existing export
cadence settings remain honored. For separated paired drawing the host uses only
the embedded pose; absence, malformed data, another generation or unmatched asset
refuse drawing without falling back to newest state. The containing snapshot's
existing advancing-identity/receipt gates provide liveness; rereading its pose
cannot renew them. Standalone preview retains independent pose freshness.
Legacy v2/v3/v4 snapshots remain readable, but separated v4 lacks the required
pose and cannot draw a replacement body on the new host. Install both builds.

This keeps the exported mesh, skin and depth policy, but ties composited body
updates to world-capture cadence and its transport delay. It does not accelerate
capture, export attachment geometry, or claim visible timing acceptance.
Regression presents a captured pose at sequence 17 and an independently advancing
live pose at 18, five blocks ahead. Paired drawing must never read the newer pose;
standalone must still use it. Restoring newest-pose selection fails this oracle.
Concurrent-reader tests bind both attachments and pose to the capture identity.

## Checks and live gate

- `./gradlew testBodyCapture build` uses the installed Minecraft frame graph.
- `python3 tools/test-player-body.py` pins submit/draw/replay boundaries and tests
  native selection/depth matching. Mutants omit body exclusion, omit schedule
  reset, omit display restoration, and restore the old duplicate/private-depth
  selection; each must fail.
- Existing paired transport, capture-hook, actual baked normal/slim player,
  controllers, camera, surface coverage/plane, shader and native build checks
  remain required. No game code or assets are copied into tests or Git.

After matched guest/native installation with both games closed, load the same
offline scenes, calibrate once in first person and activate Minecraft control.
Check first person, rear and front F5 without re-anchoring: one body in MHW,
held block visible, Minecraft's own display complete, nearer Minecraft blocks
and MHW foreground covering Steve correctly, walking staying attached and F6
releasing. Save original screenshots plus fresh v4 exclusion/capture, pose,
generation/epoch and loaded-code evidence. Builds/headless replay cannot accept
visible rendering or added runtime cost.
