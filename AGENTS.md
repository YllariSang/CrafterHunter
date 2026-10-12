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

Latest authorization (2026-10-12): user accepts the installed F5 guard (“Everything
is good now”) and authorizes the next terrain-collision work. This supersedes
older no-collision-expansion scope for this bounded line. First slice connects
existing host rays to the guest endpoint; see docs/guest-terrain-query.md. No
physics/save edits yet. Verified guest installed (backup rollback.guest-terrain.nncspc4o),
all3 cmp-clean, native/managed/flags preserved, control OFF and games closed.
Live query evidence now passes: eight requests, six hits/two misses, floor agrees
with hunter feet, three horizontal hits. Evidence captures/guest-terrain/runtime-20261012-124613Z/.
Control OFF, games open. Normal scaling corrected and managed/guest installed cmp-clean with scoped backup
rollback.ground-normal.7d2x5xzl; native/flags preserved, games now closed, control OFF.
GroundContact finite flat-patch prototype passes actual Minecraft Shapes.collide
landing/jump/edge and lifecycle checks. Latest authorized slice now wires an
opt-in collider through Entity.collectCollidersIgnoringWorldBorder for the bound
local/integrated-server player only (Creative, not flying). GroundRuntime immutable
binding expires on data/ack/tick freshness and clears on F6. Bounded controller
reads our native fixed anchor and sends rolling five-ray samples; see docs/ground-contact-prototype.md.
Live hook counts/landing remain unaccepted. Guest runtime installed with all3
cmp-clean; backup rollback.ground-runtime.2_fyymao, native/managed/flags preserved.
Games closed, control OFF, sampler not yet run.
Normal correction now accepted live: same 8 rays (6 hits/2 misses), positions and
attributes unchanged, normals unit length; captures/guest-terrain/runtime-20261012-125913Z/.
Next after verified guest installation: reopen Creative offline scenes, fly MC
vertically ~3 blocks above clear ground, first-person/F7/F9 OFF, one calibration
there; arm control, sample fixed patch, fly up another block then stop flying
without horizontal motion. Collect both collider counters, landing/jump/F6
evidence. No automatic F5 re-anchor or terrain edits. See docs/ground-contact-prototype.md; no Steve physics acceptance. Preserve accepted rendering/F5/input, fixed anchor and
F6. Combat remains out of scope.

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

Current authorized change (2026-10-12): user proceeds and reports completed F5
cycle. Read-only own-renderer CPU frame matrix probe has 6081 coherent samples,
nine refused; nine at three transitions match old mode while capture/plan is new.
First/rear W translation differs about 400cm. Latest getter appliedCapture can
already be new; it is not the supplied frame camera. See docs/f5-frame-transition.md
and captures/player-camera-occlusion/runtime-20261012-111306Z/f5-camera-frame-report.json.
Native FrameModeGate retains 32 CPU plans, holds paired world/body across F5 mode
changes until supplied camera X/Y/W matches that mode, then permits same-mode
motion without exact latest-camera matching. Full refusal cannot fall back to
sky overlay unless neither paired mode requested; explicit preview still works.
All shaders/depth/anchor/freshness/lease/capture lifetime/guest/managed unchanged.
Near-eye numerical regression rejects both original mutants; nine actual mismatch
replays and relevant tests/native build pass. Native implementation 28c19e5 is
installed after PID-pinned SIGTERM closure of only MHW 4803/MC 4710, no SIGKILL;
normal window-close interfaces were unavailable. Fresh process/window checks,
scoped backup native/mhw-renderer/build/rollback.f5-frame.zbap48nt/ (pinned installer
also rollback.ffR6lo/), all three installed artifacts cmp-clean, guest/managed
preserved. Control OFF, composition/body separation and zero floors preserved;
live channels retained, no launches. No visible F5 improvement or GPU-frame pairing
claim yet; next manual restart, one first-person calibration, still/moving/flying
F5/cape/item/foreground/F6 evidence. Do not run detailed trace ACTIVE.

Latest user video (2026-10-12): reactivated at same generation 152840515317/epoch
1378, fresh token 8860886702079709933. Recording 192926: 43.00s/1920x1080/1287
decoded frames shows all views, walking/forward/vertical flight. Reviewed strips
support body/cape/held-block alignment; foreground/hunter cover Steve, striped
fragments remain. Not blanket stable occlusion, responsiveness, Present fps or
latency acceptance. Concrete residual F5 first-to-rear flash: huge Steve close-up
only in MHW at PTS 10.185744/10.205322 (normal 10.222678) and 26.908567
(normal 26.932811), Minecraft already normally framed. Evidence/video-review.json:
captures/player-camera-occlusion/runtime-20261012-111306Z/video-192926/.
Next bounded goal: identify actual host draw camera versus selected capture/body
at transitions, then eliminate flash. Render-phase mismatch is a hypothesis.
No detailed synchronous GPU trace ACTIVE; preserve depth/freshness/lease/F5 anchor.
F6 not shown; review-end control remains advancing ACTIVE/filter armed, left as
requested. No new implementation or live diagnostics authorized merely by a
question about the next goal.

Post-install runtime (2026-10-12): user manually reopened games/bridge. Loaded
native and all installed artifacts cmp-match. New generation 152840515317,
one epoch 1378/token 5759468977933993933; 2313 samples over 254.90s, 2300 ACTIVE,
13 released, zero sampling errors/alignment invalidations, all F5 and 467 feet.
Embedded body generation/feet always match atomic capture. User originals show
first-person hiding and both third-person views. Centre probe: 2130 coherent
third-person samples/311 feet, maximum absolute X/Y 0.000189/0.000089px; 4741
first-person and 463 straddling reads refused. Relative timing fix supported,
visual jitter/attachments/occlusion/responsiveness report pending. Independent
camera sequence temporarily held at 7640 before release while guest flag stayed
present; not F6 acceptance. Explicit cleanup confirms fresh armed=0; games open,
control OFF, channels/anchor preserved. Evidence:
captures/player-camera-occlusion/runtime-20261012-111306Z/report.json.
Do not infer native capture mode from request folder after a rapid F5 change.
Do not relax the lease or claim exact GPU/display timing from the centre probe.

Current authorized fix (2026-10-12): moving/flying Steve jitter includes cape and
held item and appears in real and white diagnostics. Stationary off/on is good.
Read-only native plan versus atomic capture probe: stationary <0.00002px;
261 moving feet samples median absolute X/Y 23.77/46.16px, 95th 119.59/153.21px.
This is a centre probe, not exact GPU/screen latency. Source body/attachments are
frozen together while the independent host camera was newer. Paired render plan
now uses the uploaded capture's actual camera/view/final projection/F5 mode;
independent feed still owns token/generation/sequence and 500ms lease. Source
capture freshness and all depth/anchor gates unchanged; standalone keeps fast
camera. New capture-ID logs distinguish render source from control sequence.
Regressions reject newer-camera mutant; 14 actual captures replay; native build
and relevant camera/body/depth/controller/transport tests pass. All shaders
text-identical. See docs/camera-body-time.md. Next verified native-only installation
after authorized game closure is COMPLETE: native 74f2437 installed cmp-clean,
guest/managed preserved and cmp-clean. Backup:
native/mhw-renderer/build/rollback.camera-body-time.bme7a_bt/ (pinned installer
also rollback.q8frzx). Expanded regression passes after a722763 test-name fix.
Control OFF, normal shading/body separation/full/player ON, floors zero, channels
preserved. Next manual restart, MC on ground/first person F7/F9 OFF, one calibration,
walking+flying/all-F5/F6 evidence including responsiveness. No live improvement
claim yet. Do not use the synchronous detailed GPU trace while ACTIVE.

User correction (2026-10-12): Steve's jumpy occlusion occurs only while
Minecraft owns the MHW camera (CONTROL ACTIVE). With camera control unarmed,
Steve's occlusion is "pretty good". Do not describe this as a general player
rendering failure. Next bounded goal: stabilize Steve occlusion in the ACTIVE
camera path. Compare the same scene/control ON vs OFF at a fixed anchor, then
identify whether the mismatch is in host scene depth/camera or private guest
depth/body projection before choosing a fix. Camera/depth synchronization is a
hypothesis, not a measured cause. Preserve accepted world occlusion, same-capture
body/attachments, first-person body hiding, native F5 and F6. No implementation
or new runtime run is authorized merely by asking what the next goal is.

Latest acceptance (2026-10-12): user reports environment occlusion "stable and
better", F5 works and F6 releases; Steve alone still has jumpy occlusion. User
permits deferring it if not substantial; retain it as an open body-rendering issue,
not accepted stable Steve occlusion. Evidence report:
captures/composition-timing/runtime-20261011-215804Z/report.json. Reactivated token
893139907560709530, generation 43698606417375/epoch 1763 fixed: 1872 samples over
211.41s, 1860 matching ACTIVE, 12 matching released, zero errors/invalidations.
Original 1920x1080 captures and moving feet exist in all three F5 views; rear
observed 7.73s, front 11.18s. All captured-body feet match atomic capture feet.
Guest request flag absent after F6; explicit cleanup confirms armed=0. No E2E
latency, Present fps, dynamic same-frame GPU camera/depth or gameplay acceptance.
Environment viewport fix is accepted; next work must preserve this checkpoint
and keep Steve's flicker distinct from world occlusion. Games left open, control OFF.

Viewport fix runtime (2026-10-12): matching installed/loaded ef56144, new
generation 43698606417375 and one epoch 1763. Full-size stationary pre-UI traces
show host foreground occlusion through right/bottom edges; GPU/CPU VP X/Y/W
matches exactly. Detailed existing `trace` blocks for >500ms and triggered
lease release at applied sequence 38 despite advancing guest feed/flag present.
Do not run this synchronous multi-buffer trace while ACTIVE during acceptance.
Explicit fresh start recovered at token 893139907560709530 with same anchor;
lighter native capture 44234092 stays ACTIVE. Evidence:
captures/composition-timing/runtime-20261011-215804Z/. User all-view walking and
F6 report/collector pending. Fine native dithering remains, temporal pairing and
latency unaccepted. No guard change or F5 re-anchor.

Latest depth correction (2026-10-12): fresh optimized session generation
42796309633712/epoch 1498/token 2179040981719813127 stayed ACTIVE for the 30s
first-person timing window. Metadata stage is approximately zero over 873 captures;
that removes measured unused work, not a controlled speed/E2E latency result.
Native capture 43203517 and own-symbol state establish a 1920x1080 depth allocation
written only through (0,0,1440,810), with right/bottom storage entirely zero.
Pre-UI GPU traces 3/4/5 match stationary CPU VP X/Y/W exactly; samples span frames.
World full/preview shaders wrongly sampled full-size coordinates; player already
mapped the measured viewport. All three now share the player's existing guarded
conversion; strict host rejection/private guest depth/body binding/F5 unchanged.
Original-address mutant, production helper/shader regression, Release build and
relevant depth/body/camera/coverage/shader checks pass. See
docs/paired-depth-viewport.md. Native-only ef56144 installed after authorized
closure and fresh process/window checks; all three artifacts cmp-clean,
guest/managed preserved. Backup:
native/mhw-renderer/build/rollback.paired-depth-viewport.pg7yujaz/ (pinned installer
also rollback.WSF4r3). Control OFF, composition/body separation and zero cadence
preserved; no launches or live channel deletion. Next fresh loaded scenes, one
calibration and all-F5 motion/hunter occlusion/F6 evidence. Do not claim all
jumpiness or temporal pairing solved by this spatial fix.

Measured optimization follow-up (2026-10-12): fresh stage build session generation
41941336025126, fixed epoch 1522, token 9002085177750611168 reports ACTIVE through
30s with all F5 modes observed. Metadata preparation averages about 9.03ms per
capture while native upload-submit averages 0.50ms; CPU spans, not screen latency.
Guest now omits discarded metadata scans/hashes only during live streaming;
explicit diagnostic archives keep identity/hash binding. Build, actual body/model,
transport, timing, capture-hook and new original-work mutant checks pass. Native
timing build remains unchanged. Guest-only ca25f09 installed after fresh game
closure checks; native/managed preserved, all three cmp-clean. Scoped backup:
native/mhw-renderer/build/rollback.stream-metadata.50olc_4s/. Control OFF;
composition/zero cadence/live channels preserved. Live improvement pending. User
reports jumpy occlusion and hunter overlap in ALL views while ACTIVE, then an
accidental F6 release. This is unresolved; do not claim occlusion acceptance or
that removing metadata fixes it. Next collect fresh actual GPU camera/depth
evidence after installation/restart; retain strict depth and fixed F5 anchor.

Next bounded timing slice (2026-10-12): guest copy callback timestamps and bounded
CaptureTiming aggregates now distinguish callback/service delay, buffer copying,
metadata/hashes, channel publication and issue-to-publication-return. Native
steady-clock aggregates distinguish read, duplicate-read, upload-submit and total
upload service, at most one report set/sec. See docs/composition-timing.md.
No wire-format, scheduling, shaders, control, F5, depth or capture lifetime change.
This profiles the remaining lag; no latency reduction is claimed. Matching
1073845 guest/native artifacts are installed after verified closure, managed
preserved, all three cmp-clean; scoped backup
native/mhw-renderer/build/rollback.stage-timing.mknkcojy/ (native installer also
saved rollback.UtS5zI). Control OFF, composition/body separation ON, cadence zero
and live channels preserved. Restart, one calibration and live advancing stage
reports remain pending. The
read-only collector is tools/collect-composition-timing.py; do not run it headless.

Live attachment follow-up (2026-10-12): user reports "so far so good" after
first/rear/front screenshots, with slight occlusion delay remaining. Fresh ignored
evidence: captures/player-attachment-sync/runtime-20261012-044738/report.json.
Generation 39413949334159, fixed epoch 1128, token 5212297445829145217:
8176 samples over 900s, all matching ACTIVE acknowledgements, zero sample errors
or native alignment invalidations. Each embedded body's feet/generation matches
its atomic world capture; movement and original native screenshots exist in all
F5 views. Independent live poses are 1–2 sequences newer. Basic single-body and
attachment alignment are accepted by the user; this is not measured millisecond
latency or a performance improvement. Current-run F6 was not observed; previous
F6 acceptance stands. Control remains ACTIVE, not cleaned up or recalibrated.
Next bounded goal: profile capture/readback, publication and host upload before
selecting a delay reduction. Read-only 30s handoff observation matched 706 capture
identities to upload logs; its 21.11ms median first-seen spacing has about 21.48ms
median combined polling uncertainty and excludes GPU completion/display. It is
not E2E latency or proof of the lag's cause. No guards/settings were changed.

Latest follow-up (2026-10-12): user reports all cameras work and F6 stops control,
but captured cape/held item lag behind the newest separately rendered body.
Fresh release matches generation 30989782490149/epoch 1736; cleanup armed=0.
Bounded collector only captured stationary first-person; all-F5 motion is user
reported, not new native screenshot acceptance. Snapshot v5 now appends the
current-render body pose frozen at capture issue, with u32 byte count at 308,
reserved zeros 312–319, unchanged colour/depth offsets and 4416-byte bound.
Paired body selection uses only that pose; standalone keeps the fast pose channel.
Per-render pose availability resets before cadence checks; missing current pose
keeps body in ordinary capture. Legacy separated v4 reads but cannot provide a
replacement body on the new host. Builds/headless selection/refusal/atomic and
newest-pose mutant pass. Installed bf62d29 after both games closed; matching
guest/native replaced, managed preserved, all three cmp-verified. Scoped backup:
native/mhw-renderer/build/rollback.pose-sync.sj77c_eu/. Control OFF, separation and
full/player enabled, zero cadence intervals and live channels preserved.
Body updates now follow world-capture cadence; no latency/performance/live timing
claim. Keep existing capture lifetime/freshness/depth/F5 guards.

Latest authorized slice (2026-10-12): render one local body. Read
docs/player-single-body.md. PlayerBodyCapture tags
only the local AvatarRenderState/PlayerModel main submit. Due third-person
captures execute the prepared level graph body-free, capture at the existing
pre-depth-clear boundary, then reset that completed graph's acquisition/release
schedule and replay with the body for normal Minecraft display. Installed-class
resource/display tests and original-behavior mutants pass; shader text unchanged.
Attachments stay captured; first-person hands remain excluded as before. Snapshot
v4 adds issue-time body-excluded flag bit 1; v2/v3 still read but cannot claim it.
Native suppresses duplicate model drawing for unfiltered paired snapshots, and
shares only same-frame/generation/identity/epoch/target/camera private guest depth
with the model for filtered ones; MHW depth remains read-only. Fabric/native
builds and relevant regressions pass. Both games closed; matching Fabric/native
installed, managed preserved, all three cmp-verified. Scoped backup:
native/mhw-renderer/build/rollback.single-body.3_t40x8e/. Control starts OFF;
body separation/full/player flags enabled and zero cadence intervals preserved.
No live single-body/held-item/occlusion or extra-pass cost acceptance yet.
Do not re-anchor on F5 or expand gameplay. Controller start/stop owns the new
player-body-separate.enabled flag alongside export/composition.

Current authorized goal (2026-10-11): Minecraft-owned host camera, alongside the
existing hunter movement suppression. Read docs/guest-camera-control.md and
docs/CONTRACT.md. User confirms camera following and all F5 angles work; walking
released control. Fresh trace: fixed epoch 2422/generation 15247686952110,
sequence 8437 is the first walking-bob projection rejected by the old rigid
p[11] == -1 check; release follows at 8436 with matched=0 and no invalidation.
Native now factors final projection into perspective plus rigid camera effects,
applies the effects to the mapped view (translations in centimetres), and retains
native projection Z/W. Measured-packet regression and old-guard mutant pass;
native build and all-1699-packet replay pass. Walking fix installed native-only
after both games exited; cmp verified all three matching artifacts. Previous
native files/manifest: native/mhw-renderer/build/rollback.08pPmO. Fresh live trace
now accepts first-person walking continuity: generation 17345001885735/epoch
1575 fixed, 359 feet positions and 172 old-rejected effect samples while ACTIVE;
user reports no problems so far. All F5 modes observed but rear/front were brief
and stationary; sustained walking in those modes and explicit F6 acceptance
remain unmeasured. Released after the guest flag disappeared; explicit cleanup
completed. Later paired-freshness invalidation is 69.926s AFTER release, not a
walking failure. See captures/guest-camera/walking-fix-20261011-223848/report.json.
User screenshot shows checkerboard gaps: full path really submits 483041 points
from 949x509 onto 1920x1080. Point footprints can leave gaps; this is not finished
surface rendering. Next diagnose actual GPU camera/depth pairing and coverage
before changing native composition. No automatic re-anchor, culling/depth/MVP
acceptance or gameplay expansion is implied.

New actual-camera atomic channel, token acknowledgement/HUD,
F7/F9 feedback prevention, F6 release, bounded lease and guarded main-camera
matrix getter hooks are implemented/headless-verified; all three builds pass.
Installed matching guest/managed/native artifacts after both games exited,
verified with cmp; scoped backup build/rollback.guest-camera.mrqb_l57 under
native/mhw-renderer. New control starts OFF; no live acceptance yet. Next: loaded
offline scenes, one calibration and walking/look/all F5/release screenshots plus
fresh sequence/epoch/applied-matrix evidence.
Getter counters alone do not prove scene use; native culling/cached-camera
consumers remain a live question. Do not expand into collision/combat or input
cases 5-8. Preserve existing capture lifetime and fixed F5 anchor.

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
- User authorizes closing only Minecraft/MHW when needed, even forcefully
  (2026-10-12). Prefer normal closure; verify exact game PID identity before
  SIGTERM/SIGKILL. This does not authorize game launches or closing other apps.
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
