# CrafterHunter agent contract

## Shell

fish is the shell for this machine (4.9.3 at `$SHELL`). Write fish syntax:
`for i in $list` with no `do`/`done`, `test` rather than `[[ ]]`, `set -l` for
locals, and no `for i in 1 2 3; do` C-style loops - fish rejects them
("Missing end to balance this for loop"). The harness has been handing commands
to `/usr/bin/bash`, so bash-only syntax can appear to work and then mislead the
next reader; run commands through `fish -c '...'` when the difference matters.
Repository scripts that carry a `#!/usr/bin/env bash` shebang (`tools/*.sh`)
are bash on purpose - do not rewrite them in fish.

## Goal and current checkpoint

The goal is real Minecraft gameplay inside MHW on Linux/Proton: Minecraft owns
Steve/items/building; MHW owns its monsters, quests and world. Exactly one game
must own movement/input at a time. Combat and quest integration are NOT implemented.
Playable mode must preserve Minecraft's native F5 first-person, rear third-person
and front-facing third-person cameras. Use the actual render camera, not player
angles alone; camera displacement/collision and model visibility must follow MC.

Verified checkpoint: exclusive-input suppression, cases 1-4 accepted live. With
the filter armed the hunter neither starts a move nor continues one: enabling at
rest stops it, enabling mid-run stops it without a key release, disabling
resumes it. Enabling it mid-run used to fail because the filter was pointed at
the wrong object - the engine's per-state transition tables are split, the idle
table containing `cMove` and every locomotion table containing `cMoveEnd` and no
`cMove`. Turning is now suppressed as well; that half is derived and headless
tested but unmeasured. Read README.md, MODDING_PLAN.md,
docs/exclusive-input-design.md, docs/exclusive-input-handoff.md and the latest
MODLOG.md entries before changes; distinguish plans from measured evidence.

The composition/transport work below is a separate, still-open line; do not
regress it while working on input.

## Next acceptance gate

The hunter's movement suppression is now complete in code: `cMove` (start),
`cMoveEnd` (end a run already under way) and `cMoveTurn` (turning) are all
rewritten, the first two accepted live and turning derived from the same
transition tables but unmeasured. Do not start new acceptance runs to look for
bugs; build the next thing and fix what surfaces.

Cases 5-8 (repeat toggles, scene reload and re-resolution, plugin unload while
enabled, menus/UI) are untested and stay that way until asked for. The end
rewrite changes only a `-1` result, so a cutscene whose end predicate returns
`-1` would be rewritten; that is untested.

Build outputs are not byte-reproducible (PE timestamp), so identify artifacts
with `cmp`, not a quoted hash.

Addresses are never persisted: they change per session and per scene load.
Re-resolve with `python3 tools/resolve-exclusive-input.py --pid <MHW_PID>` in a
loaded offline scene. The request format is
`on <player> <human> <cmove,...> <turn,...> <cMoveEnd>`; every local move and
turn slot must be listed, because the engine's locomotion tables address slots a
single-hit scan does not return.

### Still-open composition line

Latest scope correction (2026-10-11): user observes smooth world composition but
slow separately rendered player. Do not conflate CH_Frame callbacks, world upload
cadence and player animation cadence. PlayerModelExport had a separate hardcoded
250ms gate; fresh pose sequence 5153–5176 advances about every 263–283ms (~3.8/s).
This is a confirmed player-update bottleneck, not proof of overall Present fps.
Guest now reads player-export.enabled as 0..1000ms using StreamCadence.Pacer;
0 samples each render frame, legacy/unreadable content defaults to 250ms. Player
controller defaults --pose-ms 0 and preserves existing world floors. Headless
60Hz/4Hz regression, old-floor mutant, actual normal/slim models and Fabric build
pass. Guest jar installed cmp-clean, pose flag 0, scoped jar/flag backup
minecraft/fabric/build/rollback.player-cadence.7py_t945. Native/managed/bridge,
capture lifetime, skin/geometry, depth/F5/freshness gates unchanged. Runtime fix
accepted: user reports "Much smoother now" after walking/head-motion/F5 check.
Fresh generation 11771983026613, fixed alignment epoch 1452, no invalidation:
43.33s trace gives 35.05 guest pose advances/s and 33.44 observed distinct host
poses/s, 1740 observed successful player frames. Original desktop screenshots
show all three F5 views; front-facing mode was sampled only briefly, not a
sustained motion test in that mode. Evidence in ignored
captures/player-pose-cadence/runtime-20261011-211117/. Native consumption counts
are sampled lower bounds, not Present fps or latency; exact 60fps remains
unaccepted. Player cadence work is complete; no new gameplay scope is implied.

The Steve-rendering spike is the active line. Two of its three requirements
existed separately and neither had both: `drawFrame` composites the whole
captured frame at native resolution as a letterboxed sky overlay, and
`drawPaired` places guest geometry correctly but samples one point per 4x4 cell.
`drawPairedFull` merges them - one point primitive per captured texel, same
gates, constants, private depth target and host-depth test as the preview - and
is opt-in via `paired-full.enabled` (`tools/control-paired-preview.py
start-full`), which takes precedence over the preview flag. It has now rendered
and been seen: guest geometry composites into MHW at native resolution and the
player model draws in world space, in all three F5 modes, at 483,041 points/frame
and ~41 fps. It is now accepted: six native-resolution screenshots confirm guest
geometry composites into MHW at native resolution with correct colour and depth
and the player model draws in world space with the real skin, in all three F5
modes. The "pale player and pale block" defect recorded earlier did not exist -
it was an artefact of reading a 960-wide screenshot of a 1920x1080 window, where
dark suit against pale interior cannot be distinguished from grey. UVs were
verified good (debug skin, correct per-face gradient in all three modes) and the
guest colour attachment measured healthy (mean luminance 117/255, 88.5%
saturated, 0% near-white), but neither changed the conclusion. Measure the thing
at its real resolution before believing a defect is in the thing.

The remaining half of the spike is **cadence**. Both transport floors are now
negotiable rather than hardcoded: each flag file's content is an interval in
milliseconds, zero means no floor, anything unreadable falls back to 250 ms.
`StreamCadence.java` (guest) and `stream_cadence.hpp` (host) implement the same
contract and are tested against the same input table. The guest reports achieved
cadence (count/min/mean/max) in `world-capture.txt`; the host reports upload age.

**Measured with Minecraft focused:** 48.3 publishes/s at no floor (best 11 ms),
39.5 uploads/s observed by the host, ~25 ms median consumer observation. The
host polls once per MHW frame, so ~40 fps is MHW's rate, not the transport's. An
unfocused guest measures ~15 publishes/s, which is focus pausing - do not read
that as a readback limit.

What remains for the spike: MHW's frame rate and unmeasured end-to-end latency. Neither is a
redesign. Note that log lines must report state, not slogans: two hardcoded
claims ("composition disabled", "drawing disabled") survived into the session
where a 483,041-point composite was running, and were believed.

Next bounded cadence change (2026-10-11): native paired uploads now reuse one
DEFAULT-usage RGBA8/R32_FLOAT texture/SRV pair on same-size, same-generation
advance, using ordered immediate-context UpdateSubresource calls. Resize/reset
creates a complete private replacement. Headless policy and native build pass;
installed cmp-clean with backup build/rollback.W0pIsw. A 25.28s user recording
200544 shows guest surfaces and walking Steve through all three F5 modes, with
one calibration epoch 1646 in the fresh session. A separate later 45s log window
has 1,766 uploads, all textures=updated, no invalidation; ~40-42 draw submissions/s.
This is basic live reuse/composition evidence, not native MHW-only pixel/occlusion
acceptance or a controlled performance A/B. No fps/latency gain or 60 fps claim.
See docs/world-frame-transport.md and captures/resident-pair-20261011-200544/.
Next: isolate MHW frame cost under matching conditions before more tuning;
keep all freshness/F5 gates. ~25ms observation spacing is not E2E latency.

Update 2026-10-11: matched ON/OFF/OFF/ON (4s settling + 20s each, focused MC,
fixed first-person feet/generation, zero floors, capture/upload always active)
measured 50.93/25.44/26.29/52.42 CH_Frame callbacks/s, guest about 30 publishes/s.
These are render callback rates, not independent Present fps. Shared GPU usage
fell to 29–44% OFF from 95–100% ON. Evidence and exact script are in ignored
captures/composition-ab-20261011-202345/. No sole GPU bottleneck or 60 fps claim.
Bounded native change now samples full/preview/player flags once per CH_Frame,
avoiding repeated filesystem queries when disabled/refused draws never set the
success latch. Actual draw refusals still retry; all capture/depth/F5 gates stay.
Headless regression/mutant and build pass; installed cmp-clean with scoped backup
build/rollback.vQD9KX. Next repeat comparison after restart/calibration; live
cadence improvement is unverified. Do not infer the slowdown's cause from code.

Live repeat 203655: loaded/build cmp matched, fresh calibration epoch 1220 and
generation 10018048051975, no invalidation, visible full paired/player draws;
flags restored. ON/OFF/OFF/ON callbacks/s = 39.13/44.08/42.88/41.38. Guest changed
from 48.97 publishes/s in ON 1 to ~30/s afterward. The prior OFF collapse was
absent, but producer/camera/weather differences prevent causal speed attribution.
Actual local 26.2 FramerateLimitTracker bytecode confirms AFK caps to 30 after
60s without input even while focused, and 10 after 10min. User setting is AFK;
MINIMIZED skips idle caps while visible. Next user-approved temporary Minimized
comparison, restore AFK afterward. No sole CPU/GPU limiter, new pixel acceptance,
E2E latency or 60 fps claim. Evidence captures/composition-ab-20261011-203655/.

Latest 204326 repeat used user-approved temporary Minimized (AFK restored and
independently verified afterward). Guest stayed ~46 publishes/s throughout;
ON/OFF/OFF/ON host callbacks/s = 38.16/39.11/39.01/38.11. Both draw flags restored
exactly, epoch 1220/generation 10018048051975 armed, no invalidation. OFF left
capture/upload/hooks active and did not reach 60; paired drawing alone accounts
for little of the current rate gap. Next profile remaining capture/upload/host
hook work before another bounded change. Brief CPU ticks show busy threads,
not a hot function; eu-stackprof's sparse/lost samples are insufficient. Do not
claim a sole CPU/GPU limiter, causal cross-session improvement, Present fps or
E2E latency. Full evidence/script captures/composition-ab-20261011-204326/.

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

Latest player-only slice: baked complete PlayerModel triangles + resident skin
and lightweight per-part matrices are implemented/headless-tested. Read
docs/player-model-preview.md. Actual normal/slim baked models have 72 faces;
this is not live player draw/skin acceptance. Keep frame/stone paths unchanged.
Next runtime gate is player asset upload + player draw submission, then fixed
Minecraft-front / independent MHW-side/back orbit acceptance. No world exporter,
input ownership or collision expansion. Do not re-anchor on F5.

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
User standing instruction (2026-10-08): commit isolated verified changes and push
them to the existing CrafterHunter origin alongside scoped mod installation.
Do not include unrelated user changes, generated assets or game binaries.

For Fabric: `cd minecraft/fabric && ./gradlew build`.
Run relevant headless scripts in tools/ and `cargo test --workspace` for protocol
changes. Avoid runtime verification scripts when working headless: they mutate
request files and require live games. Record exact checks and unresolved gaps.
