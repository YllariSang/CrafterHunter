# CrafterHunter agent contract

## Goal and current checkpoint

The goal is real Minecraft gameplay inside MHW on Linux/Proton: Minecraft owns
Steve/items/building; MHW owns its monsters, quests and world. Exactly one game
must own movement/input at a time. Combat and quest integration are NOT implemented.
Playable mode must preserve Minecraft's native F5 first-person, rear third-person
and front-facing third-person cameras. Use the actual render camera, not player
angles alone; camera displacement/collision and model visibility must follow MC.

Verified checkpoint: Minecraft colour-frame transfer and diagnostic sky-only
composition. This is not isolated Steve rendering or depth-correct guest geometry.
Preserve the working stone renderer and its validated scene-depth selection.
Read README.md, MODDING_PLAN.md, docs/depth-renderer.md and the latest MODLOG.md
entries before changes; distinguish plans from measured evidence.

## Next acceptance gate

Validate same-frame Minecraft world colour/depth capture before changing native
composition. WorldCapture currently fails closed permanently after a timeout or
setup/copy exception, retaining at most its existing buffer pair until process exit.
This deliberately prevents uncertain GPU work from causing buffer reuse/free or
unbounded allocation. Restart is required to retry. Do not weaken this policy
without verified backend completion and resource-lifetime semantics.

Callbacks signal completion, not valid pixels. Investigate the actual OpenGL
D32_FLOAT readback failure. Validate projection/depth/row packing against a real
Minecraft block at known distances. Buffer capacity is not row stride. Match
attachments and camera metadata by capture identity. Do not directly compare
Minecraft linear depth to MHW reversed-Z depth.

Capture now hooks the optional LevelRenderer always-on-top depth clear, with a
once-per-frame GameRenderer pre-hand-clear fallback when that pass is absent.
Run `python3 tools/test-world-capture-hook.py` against the local Loom 26.2 jar
when changing this boundary. `captureBoundary` records the issue-time hook.
This fix is headless-checked, not runtime-accepted; obtain fresh nonzero terrain
depth and known-block distance evidence before advancing the acceptance gate.
Update: manual bedrock captures on 2026-10-07 passed the two-distance gate
(about 4.895 and 12.122 blocks), both at the pre-hand-clear fallback boundary.
The optional always-on-top boundary and linked-camera scenes remain unaccepted.
Next transport is request-paced paired snapshots; see docs/world-frame-transport.md.
An opt-in upload-only native diagnostic now has local freshness and bounded
generation/identity selection (docs/world-frame-transport.md); it is not bound
to native composition or runtime-accepted. Keep diagnostic sky-only and
stone rendering unchanged until native paired composition has its own evidence.
CPU reconstruction/reprojection now lives in world_reprojection.hpp; see
docs/world-reprojection.md. It replays the two archived bedrock distances, not
live GPU composition. Snapshot v2 now carries actual level view/camera arguments,
final effect-modified projection upload and queried GL clip mapping/origin/range.
This is headless-checked only: fresh v2 captures in all F5 modes and shared-world
anchor conversion must be validated before binding the paired layer for drawing.
Follow `docs/world-depth-acceptance.md` for the two real-block measurements.
Update 2026-10-08: fresh archives 215619Z/215700Z/215730Z establish all three
F5 capture modes, opposite four-block camera offsets, matched hashes and finite
colour-aligned depth at the fallback boundary. Not host alignment/camera collision.
`world_alignment.hpp` is a headless-tested explicit XYZ origin/scale contract,
gated by guest generation and caller-owned host epoch. It is not wired into runtime.
Update: host-local align.request calibration and log-only mapped-camera diagnostic
are implemented, headless-tested and built, NOT installed/runtime-accepted.
CH_AlignmentHost supplies metres; upload freshness and host jump/gap invalidate.
No authoritative scene ID or guest ownership handshake is claimed. See the
diagnostic limitations and pacing prerequisite in docs/world-reprojection.md.
Next [AGENT]: bounded diagnostic pacing/trace collection and scoped runtime setup;
then [MANUAL]: shared-block/F5/collision/scene acceptance. Never re-anchor on F5.
Latest scope: stop expanding diagnostics. Paired GPU surface preview + bounded
4 Hz Fabric streaming now exist but are NOT deployed/runtime-accepted. Read
docs/playable-steve-mvp.md. Next [MANUAL]: scoped real composition acceptance;
do not expand gameplay subsystems until that result. It is not isolated Steve,
input ownership, host camera control or MHW terrain collision. No MVP done claim.
Use `tools/validate-world-depth.py --acceptance`: diagnostic exit zero alone
does not accept a milestone. New captures include colour/depth SHA-256 hashes
binding the artifact bytes to the issue-time metadata identity.

Update: paired draw submission and visible village samples were observed with
both games on the active workspace. Off-workspace capture dropped to about 1 Hz;
guards invalidated calibration, correctly. Do not relax them. Player-feet
calibration is now built/headless-tested using snapshot v3 and existing SPL
Player.MainPlayer.Position; NOT installed/runtime-accepted. v2 archives read but
cannot calibrate/draw this preview. Install all three rebuilt artifacts together.
No F5 re-anchor, guessed eye-height, shader rewrite or gameplay expansion. The
hunter can occlude coincident Steve; sparse holes and opposite-view source
coverage remain limitations. Next [MANUAL] after installation: player visibility,
fixed feet alignment and occlusion with F7/F9 OFF; no playable-MVP claim yet.

## Work discipline

- One bounded change at a time; regression tests must expose the original bug.
- No render-thread spin waits or pumping global tasks without verified semantics.
- Request writers publish complete files by atomic same-directory rename; consumers
  must claim a command before reading/deleting it. Never delete the live frame channel.
- Builds prove compilation, not visible rendering, synchronization or valid depth.
- User standing instruction: install verified matching game artifacts after changes,
  with both games safely closed and scoped backups first. Do not leave a completed
  build undeployed without reporting a concrete blocker. Do not launch games unless
  the current task authorizes it.
- Runtime acceptance requires screenshots plus fresh sequence/capture/log evidence.
- Packet receipt age measures freshness, not end-to-end latency.
- Do not expand into combat/inventory while capture is unverified.
- Preserve user changes; no launcher/account tasks or deployment without current scope.
- Publish original code, not game assets, saves, credentials or decompiled sources.

## Git and verification

Actual project history uses `.git-crafterhunter`, NOT the unrelated `.git`:
`git --git-dir=.git-crafterhunter --work-tree=. <command>`.
Commit isolated changes when authorized. Do not push unless requested.

For Fabric: `cd minecraft/fabric && ./gradlew build`.
Run relevant headless scripts in tools/ and `cargo test --workspace` for protocol
changes. Avoid runtime verification scripts when working headless: they mutate
request files and require live games. Record exact checks and unresolved gaps.
