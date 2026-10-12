# CrafterHunter mod journal

## 2026-10-12 — Reduce repeated guest request-pickup delay

Verified guest-only installation complete after PID-verified SIGTERM of MHW720643
and Minecraft720188, both exited. Backup/manifest:
`native/mhw-renderer/build/rollback.ground-query-polling.h2co2q0j/`. All three
artifacts cmp-clean; native/managed and seven flags preserved. Games closed,
control OFF, no launches. Next manual reopen into loaded offline scenes, Creative
hover about3 blocks above clear ground, first person/F7/F9 OFF. Then compare
pickup timing, calibrate once for the new session and collect centred landing,
jump, both collision counters and F6 with a fresh sampler. No runtime speed or
landing acceptance claim yet.

Control-OFF file observation of 60 existing endpoint queries measured median
78.15ms issue-to-reply, 38.47ms issue-to-request-disappearance, and 44.51ms from
observed disappearance to reply (medians are separate, not additive). Maximum
observer polling gaps ranged1.60–4.57ms. Evidence and exact probe:
`captures/ground-contact/query-timing-20261012-141309Z/`. Request disappearance
approximates guest claim, not exact UDP send or host execution. These observations
do not attribute the remaining round trip to a particular process.

TerrainQuery previously charged a50ms idle poll interval even when a new request
followed the previous reply. Its immediate empty poll could impose that delay
before the writer published its next ray. Now a valid request opens a bounded
2s responsive window, using5ms poll intervals; reply completion resets nextPoll,
and disconnect clears the window. Idle returns to50ms. No worker spin, parallel
requests, timeout/expiry change, host query-budget change, collider/render/lease
change or wire-format change. The existing20ms UDP receive timeout still bounds
worker opportunities, so5ms is a poll floor, not a claimed live service rate.

Headless sequential-writer tests reproduce empty post-reply polls, exercise five
consecutive rays, enforce one pending query and active poll floor, and verify
idle fallback/disconnect. Original fixed-idle mutant fails this regression.
Terrain protocol/lifecycle tests, actual Minecraft ground/channel regressions,
Fabric build and camera/controller checks pass. Live faster pickup, sweep
reliability and host-floor landing remain pending restart; preserve500ms expiry.

## 2026-10-12 — Centred landing retries refused on sample age

Player position verified exactly (98.38,-55.5,229.58), above the existing patch;
no recalibration. Fresh control token 7370988025598103233, same generation
9007844689815/epoch1806. landing-02 published11 patches then refused at1.507s;
landing-03, after requested mouse activity, published19 then refused at2.348s.
Evidence under captures/ground-contact/runtime-20261012-133412Z/landing-02 and
landing-03. Final five observed reply spacings were about89–128ms in the latter,
so rolling five-ray ages exceeded500ms. These are end-to-end query observations,
not attribution to guest polling, host scheduling or a proven idle-cap cause.
Neither attempt reached landing instructions; no new collision acceptance.
Sampler failed closed; agent released control explicitly afterward. Games remain
open. Next bounded goal: measure the sequential query stages and reduce avoidable
waiting before another landing run; preserve500ms expiry, host ray budget, fixed
anchor and collision guards. Do not ask for another identical readiness retry.

## 2026-10-12 — First ground runtime report; landing evidence incomplete

User reports “yes everything works” for centred landing above grass, jump and
F6 drop. Preserve this as a user report, not independently measured collision.
Fresh generation 9007844689815, fixed epoch 1806, token 3075147716335039891;
matching installed artifacts and loaded native checked. Evidence:
`captures/ground-contact/runtime-20261012-133412Z/` and `landing-01/`.
Initial stationary 120s sampler published 1967 patches. Landing window published
1603 before refusing a five-ray sweep over its freshness bound; 870 samples over
101.58s end while control is still ACTIVE. Recorded client/server collider counts
remain zero; feet Y spans only -56.88006..-56.63884 (patch top -56.71139).
Thus this recording does not establish host-floor landing or jump. The sampler
stopped before the later observed release; do not attribute patch removal to F6.
Subsequent reply matches released token/generation/epoch at sequence 11237,
guest control flag and ground.patch absent; explicit cleanup acknowledges armed=0.
Original screenshots, logs, samples and release-check.json retained. Games remain
open, control OFF. No code/installation or freshness relaxation in this follow-up.
Next bounded work: synchronize a centred landing capture with the active sampler
and investigate sweep timing if it recurs, before extending floor coverage.

## 2026-10-12 — Opt-in flat patch wired to vanilla collision

User explicitly authorizes connecting the prototype to movement. Added a bounded
fixed-anchor five-ray sampler and immutable short-lived guest patch channel.
Entity collider collection appends the patch only for the bound UUID and exact
client/integrated-server worlds, Creative single-player, not flying. F6/token
change clears immediately; data/ack/tick expiry, pause and world changes refuse.
No player teleport, velocity rewrite, save/block edits, native renderer or host
query-budget changes. Query poll/UDP wait now bound sweep scheduling. Headless
vanilla landing/jump/edge, message age/duplicate/identity tests, Fabric build and
existing terrain/camera/controller checks pass. Live hook counts, actual landing
and release remain pending; see docs/ground-contact-prototype.md.

Verified guest runtime build installed after control release and PID-verified
closure of MHW 572057 and MC 571596. Scoped backup/manifest:
`native/mhw-renderer/build/rollback.ground-runtime.2_fyymao/`. All three artifacts
cmp-clean; native/managed and seven enabled flags preserved. Games closed,
control OFF, no launches; ground sampler has not run. Next manual restart and
centred Creative landing/jump/F6 with client/server counters and screenshots.


## 2026-10-12 — Terrain normal correction accepted live

Same eight segments after restart: six hits/two misses, identical hit positions
and attributes, all six normals now unit length within 0.000001. Three floor
normals exactly (0,1,0). Matching-nonce replies, advancing host frames and original
screenshots in captures/guest-terrain/runtime-20261012-125913Z/. Installed all3
and loaded native cmp-clean. Games open, control OFF, no physics enabled.
Ground contact still requires actual-anchor/query and client/server collision wiring.

## 2026-10-12 — Terrain normal units and flat-ground solver prototype

User authorizes normal correction and ground-contact work. The adapter now divides
only positions by 100; raw finite normals are preserved, nonfinite hits refused.
Tests expose the former unit-normal-to-0.01 regression. Existing host tests and
managed build pass. GroundContact builds a bounded five-hit flat patch using the
fixed XYZ anchor with ownership/epoch/generation and freshness refusals. Actual
installed Minecraft Shapes.collide confirms landing, jumping and finite edges;
this is headless only, with no runtime movement hook or save changes. See
docs/ground-contact-prototype.md. Corrected live normals are the next runtime
gate before connecting patch acquisition to client/server collision.

Verified managed correction and guest prototype installed after release and
PID-verified closure of MHW 503751 / MC 503565. Scoped backup:
`native/mhw-renderer/build/rollback.ground-normal.7d2x5xzl/`. All three artifacts
cmp-clean; native renderer and all seven enabled flags preserved. Control OFF,
games closed, no launches. Next manual restart and ground/horizontal normal
comparison; runtime ground-contact wiring is still pending.


## 2026-10-12 — Guest terrain requests accepted live

Eight bounded queries in the freshly loaded scene: six hits, two misses, matching
nonces; three floor heights agree with hunter feet and three horizontal segments
hit nearby surfaces. Original screenshots/logs/report are in
captures/guest-terrain/runtime-20261012-124613Z/. All3 installed and loaded native
cmp-clean; control OFF, no calibration or movement. No physics acceptance.
Source now identifies the 0.01-length normal cause: TerrainAdapter divides normals
by 100 as well as positions. Correct before slope use; see guest-terrain-query.md.

## 2026-10-12 — F5 accepted; guest terrain query path

User reports “Everything is good now” after the installed F5 guard test. Bounded
recording has 3329 ACTIVE observations over 359.91s, 33 transitions held for one
host callback each, all three views, 211 distinct feet and zero invalidations.
Body feet match each atomic capture throughout. Current-run F6 was not observed;
prior F6 acceptance stands. Evidence: captures/f5-frame/runtime-20261012-122224Z/.

User now authorizes the proposed terrain collision work. Existing MHW segment
adapter is implemented/live measured; the missing guest request/result path is
the first bounded slice. Added explicit host-metre queries through the existing
guest connection, one pending request, bounded input and monotonic timeout,
strict result decoding and atomic command/reply. No movement or save changes.
Tests pin existing Rust/C# wire bytes and stale/unrelated/invalid/lifecycle cases;
Fabric build and cargo workspace tests pass. See docs/guest-terrain-query.md.
Live queries and terrain-driven movement remain pending.

Verified guest jar installed after control/filter release and PID-verified SIGTERM
of only MHW 376733 and Minecraft 376648; both exited. Scoped backup:
`native/mhw-renderer/build/rollback.guest-terrain.nncspc4o/`. All three installed
artifacts cmp-match their builds; native/managed and seven enabled flags preserved.
Control OFF; no launches. Next manual reopen into loaded offline scenes, then
explicit host-coordinate floor/wall queries. No live guest-query acceptance yet.


## 2026-10-12 — Verified F5 frame guard installed

Native implementation 28c19e5 installed after fresh process/window closure checks.
The legacy Hyprland window-close interface rejected the request and xdotool is
absent; PID-pinned SIGTERM closed only verified MHW 4803 and Minecraft 4710 under
standing user permission, no SIGKILL or other-app closure. Camera/input stop
preceded closure. Record: captures/player-camera-occlusion/runtime-20261012-111306Z/
closure-f5-frame.json.

Scoped backup/manifest: native/mhw-renderer/build/rollback.f5-frame.zbap48nt/;
pinned installer also rollback.ffR6lo/. Native replaced, guest and managed
preserved; all three installed artifacts cmp-match tested builds. Every existing
render/guest enabled flag remains byte-identical, including zero capture/upload/
pose floors and body separation. Control OFF; live channels retained, no launch.
Next manual loaded offline scenes, grounded MC first person/F7/F9 OFF, one
calibration; repeat F5 while still/walking/flying, cape/item/foreground and F6.
Visible F5 flash removal and exact GPU frame pairing remain unaccepted.

## 2026-10-12 — Frame-camera F5 guard built and checked

User authorizes the next changes and completes the requested still F5 cycle.
Read-only own-symbol probe: 6081 coherent observations/nine refused reads; nine
samples at three mode transitions have a supplied CPU VP matching the older
F5 plan while selected capture/plan is newer. First-to-rear W translation differs
by about 400cm; old-plan X/Y/W comparison is within about 0.000002. Latest getter
applied capture can already be newer, so that counter is not the frame's supplied
camera identity. Evidence: captures/player-camera-occlusion/runtime-20261012-111306Z/
f5-camera-frame-{samples,report}.json. Not simultaneous GPU camera/depth evidence.

Bounded correction holds guest paired/body drawing across mode changes until the
supplied frame camera matches a plan of the selected F5 mode. Bounded 32-plan CPU
history admits moving captures without requiring the newest camera matrix; steady
same-mode motion keeps its cadence. Scope reset on token/generation/epoch, existing
first-person hiding and all lease/freshness/depth/anchor rules preserved. Refused
full paired draws no longer become a sky overlay when preview is disabled. No
shader, wire format, guest/managed, GPU wait/resource-lifetime or gameplay change.

Numerical near-eye flash regression rejects original premature-mode and overlay
fallback mutants; nine real mismatches replay. Expanded float/effect/aspect/native-Z,
F5/history/ownership/expiry checks, camera/body, depth viewport/coverage, preview,
controller/request tests and native Release build pass. All five HLSL literals
unchanged. See docs/f5-frame-transition.md. Scoped installation follows verified
closure; visible F5 flash removal remains pending restart/runtime evidence.

## 2026-10-12 — User video supports attachment timing; exposes F5 flash

Reactivated on request using fresh input resolution, same generation
152840515317/epoch 1378, token 8860886702079709933, no recalibration. User supplies
/home/yllaris/Videos/recording_2026-10-12_19.29.26.mp4: 43.00s, 1920x1080,
1287 decoded frames. Reviewed all views, walking and forward/vertical flight.
Steve/cape/held block remain associated in examined consecutive strips, supporting
the relative timing correction. Host foreground/hunter cover Steve during
crossings; striped fragments remain, not blanket stable occlusion acceptance.

Concrete residual: first-to-rear F5 changes briefly render a huge close-up Steve
only in MHW while Minecraft already has normal rear framing. PTS 10.185744 and
10.205322, normal at 10.222678; PTS 26.908567, normal at 26.932811. Full-size
frames, consecutive strips, source identity and reactivation camera/capture logs
are ignored under captures/player-camera-occlusion/runtime-20261012-111306Z/
video-192926/, including video-review.json. Timestamp-derived log window is
approximate; not exact GPU frame correspondence, Present fps or E2E latency.

Next bounded goal: diagnose actual host draw camera versus capture/body selection
at F5 transitions, then eliminate the flash. Render-phase mismatch remains a
hypothesis. No new code/runtime diagnostic or guard/shader/settings change during
review. F6 not shown; fresh review-end acknowledgement advances ACTIVE, guest
flag present/filter armed. Control left ACTIVE for user, channels/anchor preserved.

## 2026-10-12 — Installed camera/body fix observed; visual report pending

User manually reopened games and bridge. Loaded native shadow and all installed
artifacts cmp-match tested outputs. Fresh generation 152840515317, one calibration
epoch 1378, token 5759468977933993933; input objects resolved for this process.
Evidence: captures/player-camera-occlusion/runtime-20261012-111306Z/report.json.
Bounded observation: 2313 samples over 254.90s, 2300 matching ACTIVE and 13
matching released, zero sampling errors/alignment invalidations. All F5 modes,
467 distinct body feet, all embedded body generation/feet match atomic captures.
User original screenshots show native first-person body hiding and rear/front
Steve. Native capture requests can finish after a view change; their folder mode
alone does not establish the captured camera mode.

Read-only centre probe: 2130 coherent third-person samples, 311 distinct feet;
maximum absolute X/Y discrepancy 0.000189/0.000089 pixels. Refused 4741
first-person and 463 frame/capture-plan-straddling reads. Relative timing fix is
supported; visible walking/flying jitter, cape/item, occlusion and responsiveness
acceptance still awaits user report. No exact GPU/screen latency or fps claim.

Independent camera sequence briefly held at 7640 before release while the guest
request flag remained present. This release is not an observed F6 action.
Explicit stop cleanup confirmed fresh armed=0; live channels preserved, both
games remain open and control OFF. Preserve lease/freshness and fixed anchor.

## 2026-10-12 — Captured camera/body timing fix installed

Closed only freshly verified MHW 1497410 and Minecraft 1496818 under explicit
user permission. Guarded normal close was accepted for focused Minecraft but
the process remained after grace; MHW was not focused. SIGTERM sent only to those
two games, both exited without SIGKILL. Camera/filter stop preceded closure,
normal diagnostic shader flags restored, launchers/other apps untouched. Closure
record is in captures/player-camera-occlusion/runtime-20261011-221924Z/.

Native implementation 74f2437 build was unchanged while an expanded test check
introduced a duplicate local variable name. Corrected it in a722763 and reran
the complete new regression/mutant successfully before installing. No failing
test or changed production artifact was installed. Fresh process/window checks
before/after install; native replaced, guest ca25f09 and managed preserved;
all three cmp-match tested builds. Scoped backup/manifest:
native/mhw-renderer/build/rollback.camera-body-time.bme7a_bt/; pinned installer
also rollback.q8frzx/. Ignored pointer captures/player-camera-occlusion/install-path.txt.
Control starts OFF, normal shading, full/player/body separation ON, zero floors
and live channels retained. No game launch or save/settings changes.

Next manual reopen loaded offline scenes, put MC on ground in first person,
F7/F9 OFF and Minecraft focused; one new-session calibration then walking/flying,
all F5, cape/held-item/model stability, foreground occlusion, responsiveness and
F6 evidence. This install does not itself accept visible jitter reduction or
performance/latency. Read docs/camera-body-time.md.

## 2026-10-12 — Moving Steve/camera time mismatch selects a bounded fix

User authorizes stabilizing the ACTIVE camera path, clarifies flicker only during
walking/look and then identifies cape/held-item involvement. White diagnostic and
real shading both flicker during movement; flying makes Steve jitter strongly.
Existing host-depth bypass affects only the separate body; captured attachments
still use production host rejection and shared guest depth. Do not infer that
all host-depth effects were bypassed. Stationary off/on normal/source captures
keep Steve intact. Same generation 43698606417375, fixed epoch 1763, current
control token 7814624920158794757; normal shader flags restored after comparison.
Evidence ignored under captures/player-camera-occlusion/runtime-20261011-221924Z/.

Read-only own-symbol sampling compares the native camera plan with its uploaded
capture's embedded body and actual camera. Stationary probe discrepancy is less
than 0.00002 pixels over 959 coherent samples. Requested flying run: 2160 coherent
samples, 262 distinct captured feet positions, seven frame-straddle samples
refused. Projection disagreement becomes large during movement (full-run 95th
absolute X/Y probe deltas about 58.77/90.24 pixels). This is an analytic centre
probe from native plan/metadata, not synchronous GPU draw or E2E latency.

Body and captured attachments are correctly frozen together, but the host camera
plan uses the newer independent camera.pose. Select one bounded native correction:
build the rendered camera from the same uploaded capture's actual camera/view/final
projection. Keep independent camera.pose for token/generation/advancing-sequence
ownership and the unchanged 500ms lease. Keep paired receipt freshness, immutable
getter plan, native Z/W, F5 capture mode, fixed anchor and body/attachment binding.
Log the capture identity separately from the fast control sequence. No mesh/skin,
depth-test relaxation, capture lifetime, protocol or gameplay change. Camera now
follows capture cadence; latency/responsiveness and visible improvement require
fresh runtime acceptance. Add movement/flying/F5/effect regressions plus an
original newer-camera mutant, build and install native-only after verified closure.

Implemented makePairedPlan from uploaded capture metadata, with the independent
control sequence retained separately and capture identity logged/applied. Standalone
keeps makePlan on the fast sample. New regression verifies numerical flying
relative-position/effect/F5 invariants and rejects the newer-position mutant.
Fourteen actual captures replay. Native Release, guest camera, player body,
v2-v5 transport, paired depth viewport, player/paired controller and depth selection
checks pass; all five HLSL literals are text-identical. No protocol or shader change.
Scoped native installation follows authorized closure. Capture cadence now paces
the rendered camera; no visible stability or responsiveness acceptance yet.

## 2026-10-12 — Steve occlusion issue narrowed to ACTIVE camera control

User corrects the acceptance report: Steve's jumpy occlusion occurs only when
Minecraft owns the MHW camera; unarmed camera occlusion is "pretty good".
Environment acceptance and F5/F6 results stand. This is a mode-specific issue,
not evidence that standalone body occlusion is generally broken.

Next proposed bounded goal: stabilize Steve's occlusion under ACTIVE camera
control. Compare the same scene with control ON/OFF using the fixed anchor;
distinguish the host scene-depth test from private guest-depth/body projection
before selecting a fix. Camera/depth synchronization remains a hypothesis.
Preserve world occlusion, same-capture body/attachments, first-person body hiding,
native F5 and F6. This turn records the correction/next target only: no game
requests, camera activation, source changes, builds or artifact installation.

## 2026-10-12 — Environment occlusion fix accepted; Steve flicker remains

User reports environment occlusion "stable and better", F5 works and F6 releases.
Steve alone has remaining jumpy occlusion; user permits deferring it if not
substantial. Record it as an open body-rendering issue, not accepted stable Steve
occlusion or proof that every temporal rendering issue is solved.

Fresh reactivated token 893139907560709530, generation 43698606417375, epoch 1763:
1872 samples over 211.41s, 1860 matching ACTIVE and 12 matching released, zero
sample errors and no alignment invalidations. Movement and original native
1920x1080 screenshots exist in all three F5 modes; rear observed 7.73s, front
11.18s. All captured-body feet match their atomic capture. Evidence and user
report: ignored captures/composition-timing/runtime-20261011-215804Z/report.json.

F6 removes the guest request flag and returns matching released acknowledgement
at sequence 9153. Completed explicit camera/filter stop; fresh input reply armed=0.
Capture channels and anchor preserved, games left open, control OFF. Initial
diagnostic-induced release remains archived separately and is not counted as F6.
No exact fps, E2E latency, dynamic same-frame GPU camera/depth or gameplay scope
acceptance. Environment viewport correction is accepted; Steve's flicker remains
a distinct follow-up. No source/build/artifact changes in this acceptance turn.

## 2026-10-12 — Viewport correction stationary capture and control reactivation

Ready session: verified all installed builds and loaded native shadow by cmp;
MHW 1497410, Minecraft 1496818, generation 43698606417375. One first-person
calibration epoch 1763 accepted. First control token 8879856895882257958 reaches
ACTIVE with fresh sequences. Read-only state again measures depth writer
1440x810 in a 1920x1080 allocation. Original pre-UI GPU traces 3/4/5 agree with
stationary CPU VP X/Y/W exactly; colour shows foreground occlusion through the
right/bottom target edges. Native floor/body dithering remains. Not all-view motion
or temporal depth/camera pairing acceptance. Evidence ignored under
captures/composition-timing/runtime-20261011-215804Z/.

Detailed existing trace spans host-ms 43932481..43933025 (>500ms), blocking the
native render thread. Guest sequence advances and request flag stays present,
while last applied sequence is 38. Guard latches release at 43933113, paired=1
matched=1. This is diagnostic interference, not user F6 or a movement failure.
Preserved initial samples/logs under trace-triggered-release/. Do not arm that
synchronous multi-buffer trace in ACTIVE acceptance runs; no guard relaxation.

Explicit fresh control start re-resolves input and recovers token
893139907560709530 with the original generation/epoch; no recalibration. Matching
ACTIVE sequences advance, same-capture body feet agree. Lighter native capture
44234092 completes while control stays ACTIVE. Bounded collector runs for the
requested user walking/look/all-F5/F6 check; movement and release acceptance are
pending. No code, game artifacts, settings, saves or cadence changes this turn.

## 2026-10-12 — Paired depth viewport correction installed

User permits closing only the games, even forcefully. Requested camera/filter
release first. Guarded normal close targeted focused Minecraft 1413919; compositor
accepted but the process remained after the grace period. MHW 1414500 was not
focused and no other window was closed. Sent SIGTERM only to those two freshly
verified game PIDs; both exited without SIGKILL. Closure evidence is in the
current composition-timing runtime folder. No launchers/other apps were closed.

Fresh process/window checks before and after installation confirm both absent.
Installed verified ef56144 native renderer, preserving tested guest ca25f09 and
managed code; all three cmp-match build outputs. Scoped old artifacts/flags and
manifest: native/mhw-renderer/build/rollback.paired-depth-viewport.pg7yujaz/;
pinned installer additionally saved rollback.WSF4r3/. Ignored pointer:
captures/composition-timing/install-depth-viewport-path.txt. Control starts OFF,
full/player/body separation ON, zero floors and live channels preserved. No game
launch, graphics/settings/save edits or anchor reuse. Next manual reopen loaded
offline scenes, Minecraft focused first person F7/F9 OFF, one calibration and
all-F5 movement/hunter occlusion/F6 acceptance. Spatial correction is installed;
visible improvement and any remaining temporal jumpiness remain unaccepted.

## 2026-10-12 — Measured paired world depth address correction

Fresh matching optimized guest/native/managed installation and loaded native
shadow cmp verified. New offline session generation 42796309633712, one feet
calibration epoch 1498, token 2179040981719813127; ACTIVE acknowledgement advances.
30s first-person collection: 298 summaries, 120 native interval rows, zero errors,
873 guest timing advances, no invalidation. Metadata preparation is approximately
0ms, confirming measured unused work removal. Other CPU stages and limits are in
docs/paired-depth-viewport.md; no controlled cross-session speed or latency claim.

Read-only own-symbol state records selected depth 0, allocation 1920x1080 and
writer viewport (0,0,1440,810). Existing bounded pre-UI GPU traces 3/4/5 agree with
stationary CPU VP X/Y/W exactly. Native original colour/depth capture 43203517
confirms all depth outside that writer rectangle is zero and foreground overlap
at right/bottom borders. Evidence ignored under
captures/composition-timing/runtime-20261011-214505Z/. Cross-frame stationary
inspection does not accept camera/depth temporal pairing during movement.

Bounded fix extends the player's measured writer-viewport conversion to paired
full/preview world pixels. Shared production helper preserves the player's
current-frame/finite/in-bounds refusal checks. No guessed scale, looser depth
comparison, source interpolation change, camera/input or GPU capture-policy change.
Paired constants append measured scale/origin; both pixel shaders convert before
integer host depth Load. Player/stone/frame shaders remain text-identical.
Production helper/expression regression rejects the original direct-address
mutant and checks clear-storage vs foreground occlusion. Native Release, all 13
shader entries, paired preview/coverage/plane, player preview/body, depth selection
and guest-camera tests pass. No wire format changes. Scoped native installation
follows authorized closure; live all-view motion/hunter occlusion remains pending.

## 2026-10-12 — Stream metadata optimization installed

Fresh process/window checks after authorized exact-PID termination confirm both
games closed. Installed ca25f09's verified Fabric jar atomically; native timing
and managed artifacts preserved unchanged. All three installed artifacts cmp
match tested builds. Scoped backup/manifest:
native/mhw-renderer/build/rollback.stream-metadata.50olc_4s/; ignored pointer
captures/composition-timing/install-stream-metadata-path.txt. Control starts OFF,
body separation/full/player composition ON, zero floors and live channels
preserved. No game was launched or save/settings edited. User permits only
closing the games, so reopening remains manual.

Next loaded offline scenes, Minecraft focused first person with F7/F9 OFF;
one calibration, fresh advancing stage measurements, and actual GPU camera/depth
captures for the reported all-view jumpy occlusion/hunter overlap. This install
does not itself accept a latency improvement or an occlusion fix.

## 2026-10-12 — Fresh stage reports select a bounded guest optimization

Installed/loaded native shadow and all three installed artifacts cmp-match the
tested stage builds. Ready session: MHW 1338234, Minecraft 1338267, generation
41941336025126, one accepted calibration epoch 1522, token 9002085177750611168.
Fresh ACTIVE acknowledgement advances. Evidence in ignored
captures/composition-timing/runtime-20261011-213008Z/. Thirty-second collection
records 297 guest summaries and 120 native interval rows, ACTIVE throughout and
all three F5 modes observed (user changed views despite requested still scene).
One incomplete summary read was recorded and recovered; this is not an atomic
channel failure. No settings were changed; this is not controlled performance A/B.

Across 750 guest capture advances, approximate interval CPU means derived from
rounded lifetime reports: issue-to-callback 21.30ms, callback-to-service 0.76ms,
buffer copy 1.29ms, metadata/hash preparation 9.03ms, channel publication 5.75ms,
issue-to-publication-return 38.21ms. Native weighted interval means: read 5.08ms
(937 samples), duplicate-read 5.48ms (217), upload-submit 0.50ms (720), total
service 7.04ms (937). Separate clock domains; no E2E/screen latency, sole
bottleneck or millisecond occlusion claim. Stage instrumentation is observed live.

Code confirms metadata preparation does a full depth scan, colour scan and two
hashes even when streaming discards the metadata instead of archiving it.
Next bounded change: build diagnostic metadata/statistics/hashes only when the
existing non-streaming archive path will write it. Preserve complete capture
arrays and the identical atomic v5 publication, body-pose binding, diagnostics
hashes/provenance, fixed anchor/F5 and permanent GPU failure policy. This removes
measured unused work; visible delay improvement remains to be measured after
verified guest installation with both games closed.

Implemented the non-streaming guard around metadata preparation only. The
same complete arrays and body pose still reach WorldFrameChannel; archive
statistics/hash generation is unchanged. The regression executes the actual
production preparation block with full-size buffers, checks streaming does no
diagnostic work, checks requested archive hashes/identity and unchanged pixels,
and rejects the old unconditional-work mutant. Passed test-capture-metadata.py,
test-stage-timing.sh, transport round-trip/refusal/mutant tests, actual Fabric
build/testBodyCapture/testBakedPlayer, capture-hook and player-body checks.
No native changes or measured live gain yet.

User reports jumpy occlusion and hunter overlap in ALL three F5 views while
ACTIVE, then accidental F6 release. Flag absent, preview HUD observed; no fresh
matching release acknowledgement was available, so this is not new explicit
F6 acknowledgement acceptance. Existing camera/depth evidence must be collected
before claiming an occlusion fix. Metadata removal does not establish its cause.

User explicitly permits closing only the games, even forcefully. Normal targeted
window-close dispatch was rejected by the current compositor's Lua dispatcher.
SIGTERM sent only to freshly verified MHW 1338234 and Minecraft 1338267; both
exited without SIGKILL. No launcher/other app was closed or game relaunched.
Closure events are in this evidence folder's closure.json. Camera/filter stop
requests were published first; live channels retained. Scoped guest installation
follows with native/managed preserved and cmp checks.

## 2026-10-12 — Stage timing builds installed

User says go; fresh exact-process and host-window checks confirm both games have
exited. Installed verified 1073845 Fabric/native artifacts, managed preserved.
All three installed artifacts cmp-match their tested build outputs. Previous
artifacts, flags and manifest: native/mhw-renderer/build/rollback.stage-timing.mknkcojy/.
Pinned native installer additionally saved rollback.UtS5zI/. Control/input requests
start OFF; guest-control flag absent. Body separation/full/player composition
remain enabled and world/upload/pose floors remain zero. Live channels preserved;
no game launch or save/settings changes. Ignored backup pointer:
captures/composition-timing/install-path.txt.

Next load both offline scenes, keep both visible and focus Minecraft in first
person with F7/F9 OFF. Calibrate once for the new session, reactivate control and
collect advancing guest/native stage reports during movement/all F5. No delay
reduction or live instrumentation acceptance is claimed from installation.

## 2026-10-12 — Stage timing implementation plan

User authorizes the next delay work. Keep the existing Fabric/SPL/native
passthrough and add bounded aggregate timings, without changing the snapshot
format or render/control policy. Guest records issue-to-last-copy-callback,
callback-to-publication-service, buffer mapping/copy, metadata/hash preparation,
atomic channel publication and issue-to-publication-return in its own nanoTime
clock. A callback is a completion signal, not a GPU timestamp or valid pixels.
Host records snapshot read/validation (including duplicate reads separately),
CPU texture upload submission and total upload service in steady_clock. Reuse
the existing guest summary and emit native interval aggregates at most once/sec.
No per-capture archive, wait, new GPU resource or cross-clock subtraction.

Headless injected-clock cases must distinguish callback delay from service delay,
retain the first callback timestamp on duplicates, omit incomplete/failed
publications, and keep duplicate read cost separate from successful upload cost.
Build and install matching guest/native only after both games safely close;
then one restart/calibration and a short live timing capture selects the next
bounded optimization. This profiling build does not itself fix occlusion lag.

Implemented the planned bounded aggregates. Passed test-stage-timing.sh including
duplicate-callback and folded-service-delay mutants; test-fabric-frame.sh;
test-world-frame-transport.sh (v2-v5 and newest-pose mutant); Fabric build,
actual testBodyCapture/testBakedPlayer; native Release; capture-hook, world-upload,
guest-camera, paired-preview and player-body checks. Production HLSL literals
remain text-identical; collector --help/py_compile and diff --check pass. No
transport layout change or live timing/latency acceptance. Both games are still
running in fresh process/window checks, so installation awaits normal closure.
Scoped installation is prepared for the matching Fabric/native builds; managed
stays unchanged. Details and read-only collector: docs/composition-timing.md.

## 2026-10-12 — Same-capture attachments accepted; slight occlusion delay remains

User supplies first/rear/front desktop screenshots and reports "so far so good",
with occlusion slightly late. Archived original user images and report alongside
fresh native captures in ignored
captures/player-attachment-sync/runtime-20261012-044738/. Reviewed original
1920x1080 native rear/front captures as well as the earlier first-person capture.
Fresh generation 39413949334159, one calibration epoch 1128 and control token
5212297445829145217 stay fixed. The bounded 900s collector completed 8176 samples,
all matching ACTIVE acknowledgements, zero sample errors and no native alignment
invalidations. Embedded body feet/generation match their atomic world snapshot in
all samples; native uploads carry a bound pose. World-mode samples contain 774,
70 and 227 distinct feet positions in first/rear/front respectively. View spans
are multiple intervals with stationary time, not sustained walking durations.
Four independently read camera/world samples differ during view transitions;
body and world data are instead read from the same opened inode. Independent
latest poses are 1–2 sequences newer than the bound captured pose.

User accepts basic movement/attachment alignment, with remaining slight occlusion
delay. A screenshot cannot measure a millisecond delay. No current-run F6 release
was observed; the prior matching F6 acceptance stands. Control remains ACTIVE.
No requests, flags, calibration, game settings or shaders were changed, and no
game was launched, stopped or installed in this follow-up.

Started the next bounded timing goal with a read-only 30s observation of existing
world headers and renderer upload logs. Source script, raw observations and
report are preserved in handoff-observation/ within the same evidence folder.
708 distinct publications, 706 upload log events and 706 matched identities,
zero read errors. Producer issue spacing median 39.68ms is within the producer's
own clock. Publication-first-seen to upload-log-first-seen median 21.11ms has
21.48ms median combined event/poll uncertainty; neither is screen latency.
Initial retained publication is excluded from timing matches. All observer
timestamps use one external monotonic clock; Java nanos and Windows ticks are
not subtracted across clocks. Capture/view/settings stayed unchanged, so this
is neither controlled A/B nor a moving visual-latency measurement.

Next measure capture/readback, publication and host upload separately before
choosing a bounded delay reduction. This observation does not identify the
cause of the occlusion lag, claim GPU completion/Present fps, or complete the
60fps spike. Preserve capture lifetime, strict host depth, fixed F5 anchor and
control ownership; no gameplay expansion is authorized by this rendering result.

## 2026-10-12 — Body/attachment synchronization builds installed

User confirmed both games closed; fresh exact-process and host-window checks
confirmed both absent. Installed bf62d29's verified Fabric/native artifacts;
managed code was preserved. Installed native, Fabric and managed artifacts all
cmp-match their corresponding tested build outputs. Previous artifacts, scoped
flags and install manifest are in
native/mhw-renderer/build/rollback.pose-sync.sj77c_eu/; the pinned native installer
also preserved rollback.S54Ct5/.

Camera/input requests start OFF, guest-control flag absent. Body separation and
full/player composition remain enabled; world/upload/pose intervals remain zero.
Live channels were preserved. No game was launched and no save was changed.
Next is loaded offline scenes, one fresh first-person calibration, then walking
with cape/held log in all F5 views, foreground occlusion and F6 release. Live
timing and body smoothness at world-capture cadence remain unaccepted.

## 2026-10-12 — User reports attachment timing mismatch

User reports all cameras work and F6 stops control, but cape and held item lag
on movement. Fresh release acknowledgement matches token 5628684270122769202,
generation 30989782490149 and fixed epoch 1736; guest flag absent. Explicit stop
then obtained armed=0 and both requests consumed. Loaded native shadow cmp
matches the tested renderer; initial paired v4 flags=3 and native
depth=shared-current-guest were observed. Ignored evidence is in
captures/player-single-body/runtime-20261012-022657/. Its bounded collectors
captured stationary first-person only before the user performed the later test;
all-F5 motion is user-reported, not fresh native screenshot acceptance. Do not
claim held-item, single-body or foreground-occlusion acceptance from this report.

The body draw independently reads newest player.pose; attachment pixels belong
to an earlier issue-time world.frame. This allows different animation/feet times
even with zero cadence floors. Proposed bounded fix: freeze the actual serialized
player pose at capture issue, atomically carry it with the colour/depth snapshot,
and use that pose for separated paired-body drawing. Standalone keeps the fast
pose channel. Same-capture body/attachment timing trades independent body updates
for world-capture cadence; user preference requested before choosing this route.
No faster GPU readback or E2E latency claim. Regression must expose use of a newer
pose when the retained capture is older, plus generation/asset/affine/refusal
checks, atomic concurrent publication and legacy fallback behavior. Preserve
actual F5 cameras, fixed anchor, capture failure/lifetimes and strict depth.

Implemented the bounded same-capture route. Snapshot v5 retains the header and
attachment offsets, stores pose byte count at 308 and appends the immutable
serialized issue-time body pose after depth (max 4416 bytes). Current-render pose
availability resets before export cadence checks; unavailable export retains the
ordinary captured body. Host separated drawing selects only the embedded pose,
using existing capture liveness and matching asset; missing/malformed/generation
mismatch refuses without live-pose fallback. Standalone retains independent pose
advance/freshness. Old v2/v3/v4 archives read; separated v4 cannot supply a body
replacement on the new host. Upload logs include bound body-pose sequence.

Passed Fabric build/testBodyCapture/testBakedPlayer (actual normal/slim models),
native Release, test-world-frame-transport.sh (Java/native v2-v5 all F5, malformed
poses, atomic concurrent pose/pixel readers, and newest-live-pose mutant),
test-player-body.py with existing replay/display/resource mutants,
test-player-preview.py, test-paired-preview.py, test-world-capture-hook.py,
test-world-reprojection.sh, test-world-upload.sh, test-stream-cadence.sh,
test-guest-camera.sh, test-exclusive-input.sh, test-composition-requests.sh and
cargo workspace (26 tests). All five production shader literals match HEAD;
diff --check passes. Both games remain open, so matching guest/native install
awaits safe closure. New single-body timing and its cadence tradeoff are untested
live; no attachment geometry export or latency/fps improvement claim.

## 2026-10-12 — Single-body builds installed

After the user reported both games closed, fresh exact-process and host-window
checks confirmed both absent. Installed e1002a8's verified Fabric and native
artifacts; preserved managed code. All three installed artifacts cmp-match their
tested build outputs. Unified previous artifacts, scoped flags and install
manifest are in native/mhw-renderer/build/rollback.single-body.3_t40x8e/;
the pinned-executable native installer also saved rollback.iE08EA/.

Body separation is enabled alongside existing full/player composition. World,
upload and pose intervals remain zero; camera/input requests are OFF and the
guest-control flag is absent. Live channels were preserved, and no game was
launched or save changed. Live single-body, held-item, all-F5, occlusion and F6
acceptance remains pending restart and one fresh-session calibration. Extra
third-person pass cost remains unmeasured.

## 2026-10-12 — Single player-body rendering plan

User requests keeping the exported player body and removing its duplicate from
paired world capture. Continue the existing two-process Linux/Proton passthrough;
no game assets, simulation, collision or combat changes. Local 26.2 bytecode
separates LivingEntityRenderer's main-model submit from its attachment layers.
A capture-only body exclusion must preserve held items/armor/capes, all other
entities and the normal Minecraft display, and must not erase body pixels into
holes after capture.

Bounded implementation: tag only the local AvatarRenderState/PlayerModel main
submit; omit its prepared draw during a requested third-person capture pass,
then replay the same prepared frame graph with the body restored for display.
No second simulation/extraction or render-thread waits. Verify actual frame-graph
resource schedule reset and retained feature buffers against installed classes
before using replay. Capture's existing one-flight/permanent-failure policy stays.
First person needs no extra body pass; hands remain outside the existing capture
boundary. Explicit paired metadata identifies body exclusion, so native draws
only one body even during startup/fallback. Native shares only the matching
current-frame private guest depth with its player, retaining MHW depth tests.
This preserves captured held-item/armor occlusion rather than making them vanish.

Oracle: body absent from the capture, attachments/background intact, complete
normal display restored, and no duplicate body from legacy/unfiltered snapshots.
Actual-class frame-graph replay regression must expose missing schedule reset;
headless native depth/order tests must expose the old independent-depth clear.
Build matching Fabric/native, scoped install with both games closed, cmp,
commit/push. Fresh loaded-code, all-F5 screenshots, capture identity/exclusion
metadata, model/occlusion and F6 evidence remain required after deployment.
Third-person extra GPU rendering has unmeasured cost; no performance claim.

Implemented and passed ./gradlew build testBodyCapture testBakedPlayer (actual
normal/slim 72-face models); tools/test-player-body.py (actual-graph schedule,
body-pass/display restoration mutants and old duplicate/private-depth mutant);
test-world-frame-transport.sh (Java/native v2/v3/v4, all F5/exclusion provenance
and malformed flags); test-player-preview.py, test-paired-preview.py,
test-world-capture-hook.py, test-world-reprojection.sh, test-world-upload.sh,
test-guest-camera.sh, test-exclusive-input.sh, test-stream-cadence.sh,
test-composition-requests.sh, test-paired-grid.sh, test-paired-full-coverage.py,
test-paired-plane.py, all 13 shader entries, native Release and cargo workspace
(26 tests). diff --check passes; all production shader bodies match the prior
build exactly. Extra graph work occurs only when a third-person capture is due.
User's original first/rear/front screenshots are archived, unmodified and ignored,
in captures/player-single-body/baseline-user-20261012-0137/. They establish the
reported overlap setup, not GPU color correctness or continuous motion.
Matching native/Fabric installation awaits both games' normal closure. No new
build has run in either game, and single-body/items/occlusion acceptance is pending.

## 2026-10-12 — Planar ground-depth renderer installed

User closed both games normally; fresh host window and exact process checks
confirmed both absent. Installed native-only eeae2d4's tested build using the
pinned-executable installer. Previous native DLL, renderer marker, MinHook notice
and scoped install manifest are preserved in
native/mhw-renderer/build/rollback.ycAqC3/. Installed native plus preserved managed
and Fabric artifacts all cmp-match their corresponding tested builds. Camera/input
requests remain OFF, guest-control flag absent, full/player composition and zero
world/upload/pose floors preserved. No game launched or frame channel deleted;
no graphics, saves or anchor reused. Runtime ground-band/F5/occlusion acceptance
and added sampling cost remain pending restart and one new-session calibration.

## 2026-10-12 — Bounded planar-footprint depth correction plan

Continue the paired colour/depth passthrough; no new gameplay subsystem. Saved
runtime capture 21600025's ground patch (610,730)-(738,826) has near-exact planar
Minecraft depth (fit residual <=2.19e-9). CPU camera rotation matches its captured
view within 8.20e-8. Using the native reversed-Z convention measured in earlier
GPU traces, the old constant-depth footprints predict the observed green/host
pixel mask exactly (100% of this 128x96 patch), including horizontal bands and
native depth dithering. Fitting the actual plane removes the horizontal bands,
leaving only the host's dithered silhouette. Old footprint window-depth error
reaches 1.318e-4. Analysis is a stationary cross-frame replay, not a new same-frame
GPU acceptance or proof of every ground defect. Ignored ground-analysis.json and
flat/plane predicted masks reside alongside the archived native readback.

Bounded native-only fix: use source-window-depth gradients only when every sample
in a valid 3x3 neighbourhood agrees with one local plane within float precision.
Evaluate that plane at each source-pixel corner before unprojection; retain flat
centre-depth fallback for borders, invalid/nonplanar samples or uncertain edges.
No geometry connects separate pixels. Preserve per-texel colour, draw budget,
host/private depth, player/preview/frame/stone, capture lifetime, cadence, input
and fixed F5 anchor. Regression oracle: magnified tilted plane against a smooth
host plane must avoid alternating visibility bands; original flat-depth mutant
must reproduce them. Test axis/diagonal discontinuities, holes, borders, clip
mapping and unchanged strict host occlusion. Build/verify, install native-only
with closed games/backup/cmp, commit/push. Actual ground appearance, all-F5 views
and additional sampling cost remain runtime questions after deployment.

Implemented native-only 3x3 validated plane slope for full-path corner depths.
Passed production-HLSL-body numerical tests via tools/test-paired-plane.py and
its old-flat mutant; tools/test-paired-full-coverage.py, test-paired-preview.py,
test-paired-grid.sh, all 13 entries via test-shader-source.sh,
test-world-reprojection.sh, test-world-upload.sh, test-guest-camera.sh and
test-exclusive-input.sh. Native Release build and diff --check pass. Other shader
literals, drawPairedFull budget/bindings, preparePaired and selectSceneDepth match
HEAD exactly. Ground replay script is archived with the ignored evidence.
No changed binary has run yet. Installation awaits both games' normal closure.

## 2026-10-11 — User confirms solid surfaces; explicit cleanup verified

User reports "Surfaces look solid; control stayed attached; F6 released" and
supplies a new first-person ACTIVE screenshot with continuous block/ground
surfaces. Preserved as user-surfaces-solid.png in the existing ignored runtime
archive. This supports basic live footprint coverage; it does not establish
complete ground rendering or performance. Remaining fine ground patterns and
native dithered depth stay open, with no depth bypass or further binary change.

Distinguish the report from recorded evidence: both bounded collectors saw only
mode 0. After the reply, camera acknowledgement was still ACTIVE at sequence
11317 and guest request flag present; no recorded rear/front or release lines
preceded cleanup. Therefore all-F5 walking and F6 are user-reported, not
independently accepted by this run. Stopped only the agent's scoped collector
with SIGINT after saving its samples/logs. Explicit stop then verified released
sequence 11414, guest flag absent and exclusive-input reply armed=0. Capture
channels/anchor/cadence preserved; no calibration repeated. Updated report.json
records the report, screenshot, pre-cleanup state and verified cleanup separately.

## 2026-10-11 — First-person footprint coverage observed; F5 check pending

User reopened both offline scenes and focused Minecraft. Loaded native copy
cmp-matches e5c7da2's build. Preserved 949x509 guest/1920x1080 host, F7/F9 OFF,
full/player composition and zero stream/upload/pose floors. Calibrated once at
generation 21296949949849, epoch 25299; fresh input resolution activated token
1560138725085006626. Bounded 180-second read-only collector records 1662 advancing
ACTIVE camera samples, one fixed feet position/mode 0/epoch, no invalidation.
New draw reports 2,898,246 footprint vertices. Native capture 21600025 is
1920x1080: block faces are continuous, hunter/foreground structures cover guest
surfaces, native UI remains above them. This is partial stationary first-person
coverage evidence, not all-F5, walking, release, colour fidelity or performance
acceptance. Fine ground patterns remain; the same native depth readback has a
dithered silhouette in the inspected patch. Entire remaining cause unproven;
do not bypass host depth or call all holes solved. Control remains ACTIVE for
the requested user walking/F5/F6 check. No re-anchor after calibration.

Ignored screenshots, exact collector, raw paired frame/hash, native full-size
colour/depth readbacks, fresh logs/samples and report:
captures/surface-coverage/runtime-20261011-234513/. Corrected the paired controller's
stale point-rendering description/output to projected footprints; existing
tools/test-paired-preview.py passes. No binary/protocol/input-policy change.

## 2026-10-11 — Full-surface footprint renderer installed

User closed both games normally. Fresh host window and exact process checks
confirmed both absent before installing native-only implementation e5c7da2 with
the pinned-executable installer. Prior native DLL, opt-in marker and MinHook notice
plus the scoped install manifest are retained in
native/mhw-renderer/build/rollback.ZN0mSy/. Installed native, preserved managed
and Fabric artifacts each cmp-match their corresponding tested builds.
Camera/input requests are OFF; guest control flag absent. Existing full/player
composition flags and zero world/upload/player-pose intervals are preserved.
No games launched, channels deleted, anchor reused or graphics/saves changed.
Runtime coverage/performance/F5/occlusion acceptance of this build is pending
restart, one new-session calibration and fresh native-resolution evidence.

## 2026-10-11 — Bounded full-surface coverage plan

Continue the existing two-process paired colour/depth passthrough on Linux/Proton.
The stationary first-person baseline archives GPU pre-UI traces 3/4/5, capture
18508755 and snapshot v3 identity 34043, generation 17345001885735, epoch 1575,
under ignored captures/surface-coverage/baseline-20261011-225343/. GPU VP X/Y/W
matches the read-only CPU VP exactly; GPU Z is the expected reversed convention,
not the CPU forward Z. Selected depth 0 is 1920x1080, clear 0, full coverage,
with a matching observed writer viewport. Its inspected native-resolution patch
contains continuous geometry, not the composite's regular grid. These captures
span frames: they do not establish same-frame occlusion or all-F5 acceptance.

Reconstructing the archived paired depth through its own projection/view and
fixed anchor yields 2.1218076 host pixels per source-pixel axis: the 949x509 full
path still rasterizes one point per texel. Missing footprint coverage is measured;
this does not exclude other defects or native dithered fade silhouettes.
Temporary camera/filter control explicitly released after baseline collection.

Bounded change: replace full-path points with six vertices per source texel,
reprojecting each pixel's corners at that pixel's own depth. Reuse the established
preview approach at stride 1; never triangulate between different depth samples.
Preserve preview, frame, stone, player, capture, input, anchor and all depth/lifetime
and freshness gates. Headless oracle: scaled/translated perspective coverage,
invalid samples and distinct depth edges; original point form must fail coverage.
Compile shaders and native renderer, then install the native-only change after
both games close with scoped backup/cmp and push isolated code. Runtime oracle:
fresh native-resolution before/after and log identities, host occlusion and all
three F5 views without re-anchoring. Six vertices per texel costs more GPU work;
no cadence or playable-MVP acceptance is claimed by a build.

Implemented the bounded footprint draw (2,898,246 vertices at the measured size).
Passed tools/test-paired-full-coverage.py (three fractional origins, perspective
magnification, original-point >75% holes, invalid column between 2/80-unit depths,
reversed-Z rejection reference), original-shader mutant rejection,
tools/test-paired-grid.sh, tools/test-paired-preview.py, all 13 shader entries via
tools/test-shader-source.sh, tools/test-world-reprojection.sh,
tools/test-world-upload.sh, tools/test-guest-camera.sh and
tools/test-exclusive-input.sh. Native Release build and diff --check pass.
Preview/frame/player shader text, preparePaired and selectSceneDepth match the
previous implementation exactly. Tests are headless; no changed build has been
rendered yet. Installation awaits normal closure of both running games.

## 2026-10-11 — Walking fix survives live bob; surface gaps remain

After reopening, installed native/managed/guest and the loaded native copy all
cmp-match their tested builds. Minecraft focused, both offline scenes visible,
first-person F7/F9 OFF, zero world/upload/pose floors unchanged. Calibrated once:
generation 17345001885735, epoch 1575. Token 4272173214285826285 advanced ACTIVE.
The 180-second read-only trace contains 1205 active samples: first-person walking
has 359 feet positions, 296 view states and 172 effect projections rejected by
the old decoder. All three F5 modes were recorded; rear has 31 sampled frames
with stationary feet, front just one sampled frame. This accepts first-person
walking/bob continuity, not sustained third-person walking. User says "no problems
so far" and supplies an ACTIVE screenshot. No alignment invalidation during
active control. Release follows the camera feed stopping; the guest flag was
observed absent afterward. Explicit F6 action/return was not verbally confirmed.
Camera/filter cleanup completed with the existing stop controller.

Paired freshness invalidates 69926ms AFTER camera release, correctly leaving
the anchor unarmed. This is not the walking defect returning. Do not silently
re-anchor. Ignored evidence, exact collector, screenshots, packets, fresh log
appends and report: captures/guest-camera/walking-fix-20261011-223848/.

The user's rendering screenshot has pronounced checkerboard gaps. Logs prove
the full path, 949x509 -> 483041 points onto a 1920x1080 host, not stride-4
fallback. Current full shader draws one rasterized point per texel and has no
reprojected cell footprint; enlarged/reprojected samples can leave uncovered
pixels. This is a prototype limitation, not the intended finished appearance.
Resolution and shader inspection do not establish the whole visual defect's
cause. Actual GPU camera/depth pairing, field/cache consumers and complete
surface coverage remain unaccepted. Next bounded work is that investigation,
not collision/combat/inventory or weakening freshness gates. No artifact change
or new deployment was needed for this runtime evidence.

## 2026-10-11 — Walking projection caused camera control to release

User confirms the MHW camera follows Minecraft and all F5 angles work, but
walking releases control. Fresh reactivation token 6989320817062829108 stayed
at generation 15247686952110 and epoch 2422 through all three F5 modes. The
read-only trace captures walking feet changes and the first rejected final
projection at sequence 8437, p[11] = -0.9999894499778748. The old decoder required
p[11] == -1 within 1e-5; walking bob exceeds it. At the next release log the
host's last accepted sequence is 8436, matched=0 with camera/filter/paired=1,
hooks disabled and no anchor invalidation. Guest request flag remains present,
so this second release is not an F6 request. Evidence, actual matrix samples,
screenshots and user observation are archived under ignored
captures/guest-camera/runtime-20261011-220826/reactivation/. The collector was
stopped with SIGINT after saving 1699 samples; only our collector was signalled.
Explicit camera/filter stop subsequently cleaned up the released request.

Bounded native fix: factor the actual final projection into perspective and
rigid affine bob/hurt effects, including off-centre perspective. Validate finite
invertible perspective, rigid effect and complete reconstruction. Apply the
effect to the mapped view, converting effect translation to centimetres, and
use recovered perspective XY with unchanged MHW projection Z/W and near/far.
No channel/version, Fabric, managed, capture, input-policy, anchor, freshness,
timeout or shader change. Unsupported projections still fail closed.

Passed measured-walking-packet decode regression, bob/hurt/Z-translation and
off-centre clip-XY/W equivalence, unchanged native depth rows and existing
camera lease/F5/controller tests. A headless replay accepts all 1699 archived
stationary/F5/walking packets. Restoring the old rigid guard in a temporary
mutant fails the measured-packet assertion. Native Release build and existing
world reprojection/alignment, world upload, composition flag cache, shader
compilation and exclusive-input tests pass. After the user closed both games,
fresh exact host process/window checks confirmed exit. Installed only the native
renderer using the pinned-executable installer; previous native DLL, renderer
opt-in and MinHook notice plus a scoped manifest are preserved in
native/mhw-renderer/build/rollback.08pPmO/. Native/managed/guest all pass cmp against
their corresponding builds; the latter two were preserved. Camera mode had been
explicitly stopped before closure. No channels, anchors, pacing, graphics or saves
were changed. Implementation 74b155e is pushed. Next loaded offline scenes, one
new-session calibration and fresh walking/F5/release evidence.
Walking continuity after this fix remains unaccepted;
camera/F5 user observation does not establish culling/depth compatibility or MVP.

## 2026-10-11 — Matching Minecraft-owned camera build installed

After the user quit both games normally, fresh host window and exact process
checks confirmed neither game remained. Verified the pinned MHW executable and
existing managed-install marker before replacing the three matching tested
artifacts: Fabric jar, CrafterHunter.MHW.dll and CrafterHunter.Render.dll.
Each installed artifact passes cmp against its build. Prior artifacts, managed
marker, renderer opt-in/license and any existing camera/input requests were
backed up before replacement, with a manifest, in
native/mhw-renderer/build/rollback.guest-camera.mrqb_l57/.

Updated the managed marker, removed only the new camera-control opt-in flag and
atomically published camera/input OFF requests so no old per-session addresses
can arm after restart. Existing world/player channels, cadence flags, renderer
opt-in, alignment files, graphics settings and saves were preserved. Games were
not launched by the agent. Implementation commit 69d04df is pushed; next gate
remains fresh offline scenes, one calibration, visible walking/look/all-F5 and
F6 release with screenshots and advancing capture/control/log evidence. No live
host camera, release, culling or playable-MVP acceptance claim yet.

## 2026-10-11 — Minecraft-owned camera implementation started

User authorized the next bounded goal: actual Minecraft render camera drives
the MHW view, all native F5 modes, fixed anchor and existing hunter suppression.
Route: retain existing paired world/player rendering and bridge; publish a small
atomic actual-camera channel; override only the acknowledged main camera's native
matrix getters before scene rendering. Preserve native near/far/depth convention.
No camera-field persistence, shader replacement, collision or combat expansion.
Pinned SPL 1.0.0 IL and small read-only loaded-code slices establish getter
signatures; matrix timing/culling compatibility still require live evidence.
Local inspection output is in /tmp/ch-spl-camera-il.txt and /tmp/ch-camera-native/,
outside published source. See docs/guest-camera-control.md and docs/CONTRACT.md.

Implemented the 192-byte independent camera.pose channel, bounded token/sequence
acknowledgement with HUD, F7/F9 mutual exclusion/restoration and F6 release.
Host gates on two advancing samples, the existing calibrated anchor and fresh
paired upload, bound main player/camera and armed filter. Timeout is 500ms on the
host receipt clock, pending setup is bounded to 2.5s; released tokens never revive.
Override only the main camera's returned view/projection XY terms. First-person
full-body preview is hidden while control is active. On release camera hooks are
disabled, private trampolines stay resident, and the owned input gate forwards.
Controller explicitly removes prior input patches before fresh resolution; no
per-session addresses are saved. Active logs report state and application counts.
The original view getter's cached-basis updates are preserved; field/cache-only
consumers are not redirected by this slice. Application counters can include
telemetry/helper getter calls, so neither counters nor builds prove scene use.

Passed tools/test-guest-camera.sh (Java/native roundtrip, fixed-anchor all-F5,
shortened camera, rolled/translated view, centimetres, retained native Z,
startup/duplicate/rollback/epoch/process/freshness release and fake-install
controller preservation); Fabric ./gradlew build; native Release and SPL builds.
Temporary omitted-centimetres mutant fails the mapping regression. Passed existing
test-fabric-camera/player/frame, world-capture-hook against local 26.2,
test-player-model (real normal/slim 72-face models and Java/native poses),
world-frame-transport, world-reprojection, world-upload, composition-requests,
shader-source, exclusive-input, SPL-plugin, paired/player-preview and cargo test
--workspace (26 tests), plus git diff --check and controller py_compile.
Initial Java compile caught an unavailable debugFeedback method (replaced by
logging); strict C++ test compile caught fixture narrowing (corrected). Final
builds/checks pass. No deployed change yet: both games must close safely for the
standing scoped install rule. No live camera-following/release/60fps claim.

## 2026-10-11 — Player cadence fix accepted live

After reopening both games, Minecraft was focused in first person with F7/F9
OFF, both windows visible in loaded offline scenes. Guest jar remained
cmp-identical to the tested build; the loaded native renderer matched its existing
build. Requested one new-session player-feet calibration. Generation
11771983026613 and alignment epoch 1452 stayed fixed throughout; no F5 re-anchor
or freshness/alignment invalidation. Pose/world/upload intervals all remained
zero, Minecraft's agreed AFK setting stayed restored, and capture channels,
input ownership and game settings were not changed by the collector.

Read-only sampling of the atomic player.pose header/bone matrices and our own
native renderer's Pose state, frame counter and successful-player-draw flag:

| Capture | Duration | Guest pose advances/s | Observed distinct host poses/s | Successful player frames observed |
| --- | ---: | ---: | ---: | ---: |
| Idle first person | 45.00s | 29.87 | 29.62 | 1807 |
| Mixed idle/walking/head motion/F5 | 43.33s | 35.05 | 33.44 | 1740 |

The second trace records guest pose sequences 11671–13175, changed feet and
bone matrices in first-person/rear-third-person modes, and all three world
camera modes. Original 1920x1080 desktop screenshots show the player inside
MHW in each view. Front-facing mode was sampled only briefly (one sampled mode
entry before the bounded collector completed), so this is not a sustained
front-facing animation test. The user answered "Much smoother now" to the
walking/head-motion/F5 check. This accepts removal of the separate ~3.8Hz player
pose bottleneck and the visible smoothness improvement, not overall 60fps or
a complete playable MVP.

Host counts are sampled lower bounds. Native pose state updates precede final
depth/draw gates; successful draw flags and screenshots establish submission
separately. No independent Present rate, exact visible animation fps, export-cost
isolation or end-to-end latency claim. Idle cadence near 30/s is consistent with
the restored AFK limit; no temporary FPS-setting change was required here.
Local ignored evidence includes exact read-only collectors, preconditions,
samples, fresh renderer log appends, reports and screenshots in
captures/player-pose-cadence/runtime-20261011-210836/ and
captures/player-pose-cadence/runtime-20261011-211117/.
Code/install commit c058ed3 is already pushed. This entry records its runtime
acceptance; no further artifact change or deployment is needed.

## 2026-10-11 — Player-only cadence defect: separate 250ms pose cap

The user reports smooth world composition, placing/moving/breaking, while the
separately rendered character updates slowly. Their visual 60fps estimate is not
an independent Present measurement. The prior host callback comparisons did not
measure player animation cadence and should not have been used to treat all
layers as one performance problem.

Project-source inspection found PlayerModelExport.capture hardcoded
next = now + 250,000,000ns, independent of the negotiated world stream floor.
A fresh six-second host-namespace read of player.pose observed sequence 5153–5176,
generation 10018048051975; after the initial partial interval, advances typically
arrived every 263–283ms (~3.8/s). Native already polls/applies advancing poses at
eligible per-frame player draws, so repeating cached poses at host cadence cannot
animate them faster. Evidence: ignored captures/player-pose-cadence/baseline.json.
An initial sandbox-namespace sample saw no files and is not runtime evidence;
the real host namespace sample supersedes it.

Bounded route: keep the existing Java exporter and binary channels, reuse the
StreamCadence interval parser with a headless-tested synchronous Pacer. Numeric
player-export.enabled selects 0..1000ms, zero samples the real interpolated player
state on each LevelRenderer render invocation. Missing/unreadable/prose/invalid
content retains the conservative 250ms behavior. Skin/geometry remain resident
assets, atomic pose replacement and native freshness/asset/generation/depth/F5
guards remain. WorldCapture's permanent GPU fail-closed policy is untouched.
No shader, host renderer, bridge, gameplay or animation reimplementation.

control-player-preview.py start now defaults to --pose-ms 0, validates the bound
before writing, and preserves existing world capture/upload cadence instead of
overwriting those flags with prose that falls back to 250ms. A missing shared
flag is initialized conservatively to 250ms for calibration. The controller does
not re-anchor on F5. Installation only changed the guest jar and pose flag.

Passed: Fabric ./gradlew build; actual normal/slim baked player and Java/native
roundtrip via tools/test-player-model.sh; test-player-preview.py;
test-fabric-frame.sh (including cadence regression: simulated 60Hz yields 60
updates at zero versus four at 250ms); world-frame-transport and
world-reprojection tests; git diff --check. Restoring the original 250ms scheduling
in a temporary mutant fails the render-clock regression. Gradle required cache
write access outside the workspace; retry with scoped escalation passed.
No wire format or native change, so no Rust/native rebuild required.

After the user quit both games and fresh host process checks found neither,
installed only crafterhunter-fabric-0.3.0.jar, verified build/install cmp, and
atomically wrote player-export.enabled = 0. Prior guest jars/flag and manifest
are preserved in minecraft/fabric/build/rollback.player-cadence.7py_t945.
Managed/native artifacts, world cadence, graphics settings, channels and saves
were not changed. Next fresh player sequence/host-consumption evidence and visible
walking/head motion through all three F5 modes after one new-session calibration.
New live pose cadence/smoothness and 60fps remain unaccepted until that check.

## 2026-10-11 — Constant guest cadence comparison without the AFK cap

The user approved and selected Reduce FPS when = Minimized. Verified the saved
setting and Minecraft focus before and throughout another ON/OFF/OFF/ON run
(4s settling + 20s measurement each). Both windows remained visible at 949x509;
guest feet, generation 10018048051975 and first-person mode stayed fixed.
Capture/upload floors remained zero. Only full paired/player draw flags toggled;
both were restored byte-for-byte, no held temporary flags remained. Calibration
epoch 1220 remained armed; no freshness/reset events. Full paired/player draws
are visible in original-resolution desktop screenshots. Local ignored evidence
and exact script: `captures/composition-ab-20261011-204326/`.

| Drawing | CH_Frame callbacks/s | Guest publishes/s | Host uploads/s |
| --- | ---: | ---: | ---: |
| ON 1 | 38.16 | 46.65 | 37.71 |
| OFF 2 | 39.11 | 46.45 | 38.56 |
| OFF 3 | 39.01 | 46.35 | 38.36 |
| ON 4 | 38.11 | 46.01 | 37.41 |

Unlike the earlier run, guest publishing stayed about 46/s throughout. Host
callbacks differed by about 0.9/s between drawing ON and OFF; OFF did not approach
60. Capture/upload and the existing renderer hooks remained active, so this does
not measure vanilla MHW, capture-free rendering, or upload-only overhead. GPU
readings were 63–87% ON and 78–85% OFF, around 1710MHz and 83–85C. The result
supports profiling capture/upload and host hook costs next, rather than assuming
paired drawing is the main cause of the current ~38/s rate. No sole CPU/GPU
bottleneck, independent Present fps, E2E latency or 60 fps acceptance claim.

A separate read-only five-second process CPU tick sample after the run found
MHW at 388.6% of one CPU and Minecraft at 125.2%; busy host threads included
Rendering Thread 66.6% and dxvk-cs 46.4%, guest Render thread 63.8%. This is not
a function profile or a matched phase. A bounded eu-stackprof attempt collected
only 48 samples and reported 19 lost, insufficient to name a hot function.
No debugger pause, input injection, kernel permission change or game file change.
The user restored AFK afterward; options.txt independently confirms AFK. Full
paired/player flags and zero upload floor match the original preconditions.
The flag-cache change remains installed and regression-checked; causal speed
improvement across the two sessions is unproven. Next bounded task: profile the
remaining capture/upload/host hook work before changing its implementation.

## 2026-10-11 — Flag-cache live repeat; still-player AFK cap confirmed

After the user reopened both games, the fresh MHW PID 160189 loaded a renderer
byte-identical to the new build. Both windows were visible at 949x509, Minecraft
focused, F7/F9 OFF, first person. Requested one new-session feet calibration;
epoch 1220/generation 10018048051975 stayed fixed through the comparison. Full
paired and player drawing were submitted visibly; screenshots and fresh logs
are in ignored `captures/composition-ab-20261011-203655/`, along with the same
comparison script, preconditions and results. All flag changes were restored;
capture/upload channels were left intact. No freshness/alignment invalidation.

| Drawing | CH_Frame callbacks/s | Guest publishes/s | Host uploads/s |
| --- | ---: | ---: | ---: |
| ON 1 | 39.13 | 48.97 | 38.63 |
| OFF 2 | 44.08 | 29.93 | 29.78 |
| OFF 3 | 42.88 | 29.74 | 29.59 |
| ON 4 | 41.38 | 29.84 | 29.84 |

The guest rate changed during this ABBA run, so the complete four-phase run is
not a constant-producer-rate composition comparison. The last three phases have
roughly 30/s guest cadence, with OFF 42.88–44.08 versus ON 41.38 host callbacks/s.
MHW's camera angle and Minecraft weather/view also differ from the pre-install
session. The prior OFF 25–26 collapse was not observed here, but these differences
prevent attributing its disappearance quantitatively to the cache change.
Shared GPU readings were 53–73% in ON 1, 67–94% OFF, 66–89% ON 4; neither sole
GPU nor sole CPU bottleneck is established. No independent Present measurement,
E2E latency, new skin/depth acceptance or 60 fps claim.

Checked the actual local Loom 26.2 client bytecode with javap, keeping output
outside the repository at `/tmp/ch-framerate-tracker.javap`.
`com.mojang.blaze3d.platform.FramerateLimitTracker` applies SHORT_AFK after
60,000ms since input when InactivityFpsLimit is AFK, limiting to min(maxFps,30).
After 600,000ms it uses 10 fps. MINIMIZED omits those AFK checks; iconification
still caps to 10. The user's options currently select AFK, maxFps 120 and VSync.
This explains why a focused still-player test can publish at 30/s; focus alone
does not guarantee an unthrottled producer. No settings changed by the agent.
Next temporarily select Minimized with user approval and repeat while both games
remain visible, then restore AFK. Continue profiling from that controlled result;
do not infer a transport limit or turn the measured checkpoint into a 60 fps claim.

## 2026-10-11 — Matched composition comparison and bounded flag polling

Ran ON/OFF/OFF/ON with four seconds settling and twenty seconds measured per
phase. Minecraft remained focused, both windows visible, first person, F7/F9 OFF,
949x509 guest capture, zero guest/host floors, fixed feet and generation
8042902613435. Capture/upload stayed enabled; only full paired/player draw flags
were temporarily renamed and restored. No input injection or continuous recording.
Each phase has an original-resolution desktop screenshot and fresh log evidence
under ignored `captures/composition-ab-20261011-202345/`, with the exact script,
preconditions and results. Calibration/freshness remained valid throughout.

| Drawing | CH_Frame callbacks/s | Guest publishes/s | Host uploads/s |
| --- | ---: | ---: | ---: |
| ON 1 | 50.93 | 29.89 | 29.89 |
| OFF 2 | 25.44 | 29.99 | 25.44 |
| OFF 3 | 26.29 | 29.99 | 26.29 |
| ON 4 | 52.42 | 29.93 | 29.88 |

The counter is the existing native CH_Frame counter read from our matching loaded
DLL, resolved afresh through symbols and process mappings, read-only. It measures
SPL render callbacks, not independent Present fps. Aggregate shared-GPU utilization
was 95–100% ON versus 29–44% OFF; guest cadence was roughly 30/s in all phases.
Thus this comparison does not establish the paired GPU draw as the sole limiter,
an unthrottled guest rate, E2E latency or 60 fps. Temperature/clock changes and
shared GPU load remain limits on cross-session comparisons.

Source inspection found per-eligible-UI-callback filesystem queries for the full,
preview and player flags. Disabled/refused draws do not set the success latch, so
these queries repeat throughout the frame. The bounded change samples those three
flags once at CH_Frame and uses that state for draw dispatch and logs. Toggles
apply next frame; failed GPU/depth draw attempts still retry. No shader, capture,
depth, freshness, anchor, F5, gameplay or GPU lifetime policy change. This removes
repeated flag I/O; its connection to the observed OFF slowdown remains unverified.

Passed native Release build; `tools/test-composition-requests.sh` (1,000 refused
callbacks issue three queries, next-frame toggles, independent combinations,
reset/wrap). A temporary per-callback-polling mutant fails the query-count test.
Also passed world-upload, world-frame-transport, world-reprojection,
depth-selection, paired-grid, frame-composite, stream-cadence, exclusive-input
headless scripts and `python3 tools/test-player-preview.py`. No protocol/Fabric
change. Installed native DLL only after the user quit both games and fresh host
process checks found neither; installer backup `native/mhw-renderer/build/rollback.vQD9KX`,
build/install cmp passed. Next repeat the same comparison with the installed
flag-cache build. Live improvement and the 60 fps gate remain open.

## 2026-10-11 — Resident pair observed live; cadence remains about 40 fps

Reviewed the user's `/home/yllaris/Videos/recording_2026-10-11_20.05.44.mp4`:
25.28 seconds, 1920x1080 desktop, both games tiled. Extracted original-resolution
frames show guest surfaces changing and Steve walking in MHW while the guest
cycles first-person/rear/front/first-person. The hunter remains stationary.
F7/F9 are OFF on the guest HUD. This supplies basic live composition regression
evidence for the installed resident pair, not fine native MHW pixel/skin/depth
acceptance: the host is only a portion of the desktop recording. No pale-skin bug
was inferred from the recording. No host-camera orbit or controlled occluder gate.

Fresh native logs establish player-feet calibration at epoch 1646, generation
8042902613435, identity 4360; complete player asset/draw submission follows. The
later F5 cycle is mode 0 through identity 8363, mode 1 identities 8382–8662, mode 2
8682–8991, and mode 0 from 9012, all at the same epoch/generation. Resize earlier
in this session allocated pairs at 949x509 -> 949x1028 -> 949x509 as expected.

A separate later 45-second window ending at the initial log snapshot
(host ticks 8239945–8284945, not precisely video-aligned) contains 1,766 paired
uploads: 39.24/s, all textures=updated, zero creations, no calibration invalidation.
Four full-resolution reports contain 424/403/407/403 draws per ~10 seconds,
483,041 points each, host target 1920x1080 and receipt age 14–15ms. Median consumer
observation interval is 24ms. These are upload/draw cadence and receipt freshness,
NOT E2E latency. The earlier journal's "transport latency ~25ms" conclusion was
unsupported: it used the spacing between host observations, not time in transit.
The recorder has 935 video frames at ~37 frames/s; that is not host fps.

No controlled before/after comparison was recorded, and recording itself adds
load. The current figures remain near the prior ~40 fps checkpoint; no performance
gain, 60 fps or complete playable milestone is claimed. Next isolate host frame
cost with matched focus/resolution/scene and recording conditions before tuning.

Local ignored evidence: `captures/resident-pair-20261011-200544/` contains selected
PNG frames, a renderer-log snapshot, exact later-window log/summary, camera-mode
ranges, video PTS and review/flag metadata. Source video is preserved in Videos.
No game input, flags, calibration, binaries or saves were changed during review.

## 2026-10-11 — Bounded cadence tuning: resident native paired textures

Continued the accepted native-resolution spike's remaining cadence line. Source
inspection found two texture and two SRV creations on every advancing capture.
The native upload now keeps one DEFAULT-usage RGBA8 UNORM/R32_FLOAT pair for
same-size, same-generation snapshots and queues both full-image updates on the
existing immediate context before publishing the matching metadata. No host
depth writes, wait, Map, Flush, ring, new exporter or gameplay subsystem. Startup,
resize, producer replacement and clear/reset allocate privately; any creation
failure or device removal clears the drawable pair and calibration. Existing
identity selection, watchdog, projection, anchor, shader and F5 behavior remain.
Existing upload log reports textures=created/updated, without a new trace channel.

Source-lifetime basis: Microsoft's documented UpdateSubresource contract copies
the CPU bytes before returning:
https://learn.microsoft.com/en-us/windows/win32/api/d3d11/nf-d3d11-id3d11devicecontext-updatesubresource
Outstanding GPU draws can cause driver-managed intermediate copies. This removes
application resource allocation; it does NOT establish improved fps or latency.
Minecraft's permanent fail-closed readback policy is unchanged.

Passed: `bash tools/test-world-upload.sh` (120 advances: two textures/two views,
240 ordered updates; exact rows/halves, resize, restart, reset, all four creation
failures, malformed input); a temporary mutant disabling reuse reproduces the
original per-capture allocation behavior and fails the advancing-capture test.
Also passed `bash tools/build-mhw-renderer.sh`, `bash tools/test-world-frame-transport.sh`,
`bash tools/test-world-reprojection.sh`, `bash tools/test-depth-selection.sh`,
`bash tools/test-paired-grid.sh`, `bash tools/test-frame-composite.sh`,
`bash tools/test-stream-cadence.sh`, `bash tools/test-exclusive-input.sh`,
`bash tools/test-shader-source.sh` (13 HLSL entries),
`python3 tools/test-player-preview.py` and `git diff --check`. Protocol and Fabric
code are unchanged, so no Rust/Fabric rebuild was required. Fish config attempted
writes outside the sandbox during the suite; all test commands exited zero.

Installed only the native renderer after host checks initially found Minecraft
PID 16962 and MHW PID 20639 running, the user quit both normally, and a fresh host
check found neither. `bash tools/install-mhw-renderer.sh` backed up the previous
native DLL/enable marker/MinHook notice under
`native/mhw-renderer/build/rollback.W0pIsw`; build/install `cmp` passed. Guest and
managed artifacts are unchanged. No game was launched or save edited.

Next: fresh screenshots and cadence evidence for resident-pair uploads, comparing
the same resolution, focus, floors and scene against the prior ~40 fps checkpoint.
No new runtime acceptance or 60 fps claim. Ghidra's official repository supplied
by the user is available if native engine inspection becomes necessary; this
change required only project source and documented D3D11 upload semantics.

## 2026-10-11 — Spike accepted at native resolution: there was no pale-surface defect

Six screenshots at native resolution, both games live, `paired-full.enabled` and
the player preview armed, anchor calibrated at epoch 1825: **everything renders as
intended**. Guest geometry composites into MHW at native resolution with correct
colour and depth, and the player model draws in world space with the real skin,
across all three F5 modes.

So the two defects recorded an hour ago - "the player figure reads flat/pale" and
"the composited block reads pale" - were not defects. They were artefacts of
judging a 1920x1080 window from a 960-wide screenshot, where the figure is about
40 pixels tall and dark suit against pale MHW interior cannot be distinguished
from grey. One native-resolution shot of the MHW window alone settled it, exactly
as flagged, and the thing I would have gone on to instrument did not exist.

Worth stating plainly because it cuts against the work, not for it: the two
probes run to "localise" it were both correct and both unnecessary. UVs were
verified good, the guest colour attachment was measured healthy, and neither
changed the conclusion. They cost nothing and would have been the right calls had
the defect been real - but they were not, and reading a thumbnail as a bug is the
failure mode here, not a missing measurement.

Final measured state alongside the acceptance:

    paired FULL-RESOLUTION draw submitted: 949x509 guest -> 483041 points,
      host 1920x1080, upload age 16-20ms
    alignment epoch=1825 generation=4688807262529 identity=83313 mode=2
      mapped-camera-metres=(-5.662906,-5.745375,1.679934); composition=full
    guest streamCadence=n83309 min=10ms mean=26ms max=1053ms, interval 0 (no floor)
    131,098 paired world uploads in this session

This is the Steve-rendering spike only. It is not isolated Steve, not input
ownership, not host camera control and not MHW terrain collision, and no playable
MVP is claimed. Standing diagnostic flags still set: `paired-full.enabled`,
`world-upload.enabled`, `player-compose.enabled`, `player-skin-check.enabled`.

## 2026-10-11 — UVs exonerated; the pale surfaces are downstream of the guest bytes

The UV debug skin separates "UVs are wrong" from "the skin samples wrong" in one
screenshot, and the answer is that UVs are **correct**: every face renders its own
colour with a vertical gradient down the body, in all three F5 modes, through the
same `input.uv` the real path uses. Geometry, pose and placement are therefore
right too, and the pale player figure is downstream of the vertex stage.

Then the second question, settled without the host at all: **is the guest colour
attachment itself healthy?** It can be checked directly, because the raw bytes are
in the shared frame. They are:

    frame 949x509, mean luminance 117.1/255, 88.5% of pixels saturated,
    0.0% near-white, first samples (87,114,54) / (78,101,48) - grass green

So the guest capture is not washing anything out, and the composite's input is
correct and saturated. Anything pale on screen is introduced after those bytes,
which puts the defect on the host side of the upload or in the draw - not in the
capture, and not in UVs. The upload path itself reads correctly on inspection
(R8G8B8A8_UNORM, SysMemPitch from the snapshot, immutable resource, private
creation, nothing published on partial failure), which is inspection rather than
evidence.

Still open, and deliberately not resolved by assertion: whether the real-skin
figure and the composited guest block are actually wrong, or whether they merely
read that way in a 960-wide screenshot of a 1920x1080 window. That needs one
screenshot of the MHW window alone, at native resolution. Debug shading has been
returned to `real`; no debug flag is left set.

## 2026-10-11 — Spike accepted visually: guest composite and player both render

Three screenshots with both games live, full-resolution path armed, anchor
calibrated at epoch 1825 / identity 45200:

1. first person - a guest block composites into MHW's scene;
2. F5 rear third person - the composition persists where the block was, and the
   player model is drawn in the world beside the hunter;
3. F5 front third person - the same, from the opposite side.

So the spike's visual claim holds: guest geometry composites into MHW at native
resolution with real depth, the player model renders in world space, and both
survive camera-mode changes. Measured alongside it this session: 483,041 points
per frame at ~41 fps, upload age 15-24 ms, host observing 39.5 uploads/s.

**Two things the images do not explain, recorded rather than smoothed over.**

The player figure reads flat and pale, while the skin it is drawn with is
verified correct: `player skin CPU/upload crc32=ab329927` (the value recorded in
July as matching the cached user skin), `player skin GPU crc32=ab329927
CPU=ab329927 equal=1`, UV range [0,1]x[0,1], SRV format 28, sampler point/clamp,
`PlayerPS` reading `skin.Sample(skinSampler,input.uv)` with an alpha cutoff. The
asset is right, the binding is right, and the pixels are still wrong - so the
defect is in the draw, not the data, and it is not yet localised.

The figure also appears to sit slightly above the ground rather than standing on
it, which points at the feet anchor rather than the skin.

Neither is claimed as understood. The UV debug skin is the next step: one flag,
one screenshot, and it separates "UVs are wrong" from "the skin samples wrong" -
if the per-face gradient pattern is correct, the defect is downstream of UVs.

## 2026-10-11 — Spike measured: the transport is not the wall

Ran the negotiated cadence with both games live, Minecraft focused with focus
pausing disabled, first person, both floors at zero.

    guest publishes 12325 -> 14502 in 45.04s
      achieved 48.33 publishes/s (20.7 ms apart), best case 11 ms
    host 1776 uploads in the same window = 39.5/s
      producer issue step ms: min 12.1  median 22.7  max 100
      consumer observation ms: min 21    median 25    max 76

The host polls once per MHW frame and can only observe one identity per frame, so
39.5/s against a guest capable of 48.3/s places MHW's own frame rate at roughly
40 fps. Transport latency is ~25 ms against a 16.7 ms budget at 60 Hz.

An earlier run with Minecraft unfocused measured 15 publishes/s. That was focus
pausing, not the readback, and it is why no architectural conclusion was drawn
from it. The mesh-path fork raised earlier - "if the numbers are bad, stop
streaming pixels" - is **not** forced by this data and is dropped.

Then the draw itself, which had never been rendered:

    paired FULL-RESOLUTION draw submitted: 949x509 guest -> 483041 points,
      host 1920x1080, upload age 14-40ms, 362-437 draws per 10s report

~40 fps sustained with a 483,041-point full-resolution composite in the frame.
Resolution is achieved; cadence is bounded by MHW, not by the composite. The
remaining gap to 60 fps is MHW's frame rate and ~25 ms of latency, both of which
are tuning problems rather than redesign.

**Defect found by reading this session's own log.** Two lines asserted a state
that had stopped being true: every upload ended in "; composition disabled" and
every alignment line in "; drawing disabled", both hardcoded, both left over from
when those paths were diagnostic-only. They were believed - "composition is
disabled" is why the first alignment attempt looked like a failure - while a
483,041-point composite was being submitted thirty times a second. Both now
report actual state. A log that states a falsehood is worse than no log, and this
one had been sitting there long enough to mislead a reader who wanted to believe
it.

Also corrected: the probe's own 19.27/s figure was its 20 Hz sampling limit, not
the transport, and is not used.

Verified: `bash tools/test-fabric-frame.sh`, `tools/test-stream-cadence.sh`,
`tools/test-paired-grid.sh`, the exclusive-input, reprojection, depth-selection
and frame-composite suites, and the MinHook cross-build. The cadence and draw
figures are measured live; the corrected log strings are built but not yet
installed.

Installed once MHW was closed (installed SHA-256
`f21ac801678796b7b5866e8efaf591efb7365bad7c2ac496432585f4373186e3`, `cmp`-clean
against the build, corrected strings verified present in the binary; previous DLL
preserved in `build/rollback.OeLnLc`). The log tells the truth from the next
launch.

## 2026-10-11 — Make transport cadence a negotiation, and measure it

The full-resolution composite is still slow, and the reason was never measured:
both ends of the transport carried a hardcoded 250 ms floor, introduced together
as a deliberate "request-paced diagnostic, not a video reader". Neither side was
ever asked what cadence it could sustain, so "can this reach frame rate" had no
answer to be had from the code.

Both floors are now negotiated. The flag files double as the negotiation: each
file's content is the interval in milliseconds, and zero means no floor - the
guest publishes on every render tick that has no copy in flight, the host looks
on every frame. Back-pressure is unchanged and is what actually bounds the guest:
one copy in flight, no queue growth, and the permanent fail-closed policy on an
incomplete copy. Defaults are unchanged at 250 ms, so nothing moves until asked.

Every refusal falls back to 250 ms rather than to the fastest setting, because a
typo silently becoming "per frame" is the kind of failure nobody notices until the
machine is on fire. The contract is implemented twice - StreamCadence.java for the
guest, stream_cadence.hpp for the host - and both halves are tested against the
same table of inputs, because two languages reading one contract is exactly how
they drift apart.

The guest now reports what it achieved rather than what it was asked for:
world-capture.summary carries streamIntervalMillis and a cadence line with count,
min, mean and max. First samples only establish a baseline, and a backwards clock
step yields no interval rather than a negative one. That number, plus the host's
existing upload-age report, is what the next runtime session reads - and it is the
answer to the question the spike actually turns on.

Writing the three cadence tests caught three wrong expectations in them - a
miscounted interval, a wrong mean, and a count that contradicted the assertion
next to it. The implementation was right each time; the tests were wrong, and they
were wrong in the direction of not having been thought through.

Verified: tools/test-fabric-frame.sh (cadence contract and report),
tools/test-stream-cadence.sh, tools/test-paired-grid.sh, the exclusive-input and
reprojection suites, and the MinHook cross-build. **Not measured at runtime and not
installed** - both games are running. No cadence claim has been made; the change
only makes the question answerable.

## 2026-10-11 — Steve spike: full-resolution paired composite

The spike wants a full Minecraft frame at native resolution and 60 fps with the
player visible. Two of those three already existed, in different places, and
neither had both: `drawFrame` composites the whole captured frame at native
resolution but as a letterboxed sky overlay, while `drawPaired` places guest
geometry correctly - real depth, host reprojection, host occlusion - but samples
one point per 4x4 cell, recorded as an explicit bounded budget.

`drawPairedFull` merges them. Same gates, alignment, constants, private D32
target and host-depth fragment test; the only difference is one point primitive
per captured texel instead of a quad per 4x4 cell. At stride 1 the cell footprint
is the texel itself, so the quad form would spend six vertices to cover one pixel
while a point rasterizes as exactly that pixel.

Cost stated rather than assumed: 975,572 points at 949x1028 against the
preview's 366,996 vertices - 2.7x the geometry for 16x the sampled texels, and a
sixth of what a stride-1 quad splat would cost. The arithmetic lives in
`paired_grid.hpp` with regression tests (including one that reproduces the
recorded 366,996), because the equivalent inline arithmetic in
`frame_composite.hpp` was inverted and shipped unnoticed.

The shared prologue (gates, constants, private depth) is now `preparePaired`, so
the preview and the full path cannot drift apart; the preview's inline
`((w+3)/4)*((h+3)/4)*6` is gone.

Cadence is untouched and deliberately separate: the guest still publishes at its
own request pacing and the host still polls at its own interval, so this is full
resolution and still slow. The path logs point count, host-local upload age and
draw count every ten seconds so the next runtime session produces a measurement
rather than another estimate. `world::Freshness` gained a tested `age()` for
that, because it is receipt age and not end-to-end latency.

Opt in with `paired-full.enabled`, which takes precedence over the preview flag;
`tools/control-paired-preview.py start-full` sets it, and `stop` now clears it.

Verified: `bash tools/test-paired-grid.sh` (draw arithmetic, plus the freshness
age accessor), the existing exclusive-input, reprojection, depth-selection and
frame-composite suites, and the MinHook cross-build. **Not rendered and not
installed** - both games are running. The projection math is unchanged from the
preview, so what is new is resolution, topology and geometry volume, none of
which a host-side test can judge.

## 2026-10-11 — Suppress cMoveTurn: closing the last movement hole in exclusive input

The last measured hole in exclusive input: the hunter could not start a run and
could not continue one, but `cMoveTurn` was forwarded, so it could still strafe
while Minecraft owned the keyboard. Turning is movement.

`cMoveTurn` (index `0x9b`) sits in every locomotion table immediately after
`cMoveEnd`, so it is consulted while a run is being ended. It is now suppressed
by the same shape as the move predicate: run the real predicate, return `-1`,
let the engine take its own "not eligible" path.

Not a one-line change, because the turn slots are duplicated like the move
slots: the local set's group-1 object array maps index `0x9b` to an object that
a single-hit heap scan does not return (one session published `0x5f427070` while
the engine's table addressed `0x5f426cf0`). The resolver now emits every local
turn slot - three in the session measured - and the renderer patches them all
against one shared private vtable, restoring each only if it still carries ours.
Request format is now `on <player> <human> <cmove,...> <turn,...> <cMoveEnd>`;
an older single-address turn field still parses, so a stale request cannot
misfire. Reply file gained `turned=`.

Policy tests invert the documented-gap test into the fix's regression, plus
foreign turn object, foreign player link and multi-slot coverage. Verified:
`bash tools/test-exclusive-input.sh`, MinHook cross-build, resolver dry run
against the live pinned build (3 turn slots resolved).

**Not verified at runtime and not installed** - both games were running, and the
user asked to stop manual acceptance testing and fix problems as they surface
instead. The derivation is the same transition-table evidence that made the move
and end rewrites work; no run has measured turning yet.

## 2026-10-11 — Exclusive input accepted: cases 1-4 pass

Ran the acceptance against the installed end-transition rewrite, MHW live, key
held across the whole run. `tools/accept-exclusive-input.py` resolves the
instances, arms mid-run, disarms, and reports units/s for every player field
that moved, so the numbers come from the game's own position rather than an
assumed offset. Two consecutive runs, armed at t=7s with W never released:

  P+0x390 (world)  373.35 -> 0.58 -> 290.75    and   333.46 -> 0.67 -> 290.42

The transition state agrees: `(1,2)` -> `(1,3)` (the end transition's own
destination) 0.6s after the arm -> `(1,0)` idle -> back to `(1,2)` 0.6s after
the disarm. `released` advanced by exactly one per arm cycle; `suppressed` kept
climbing because the idle table consults cMove continuously. The residual
0.3-0.8 units/s while armed is the same order as the at-rest figure (0.94 units
over 6s), i.e. jitter, not movement. Cases 1, 2, 3 and 4 now pass.

Two scenes, same structure: the run occupies state `(1,4)` in one and `(1,2)` in
the other, and both tables carry cMoveEnd and cMoveTurn and no cMove. The state
id is scene-dependent; the split is the rule.

Also fixed: the resolver raised OverflowError on a wild heap pointer instead of
rejecting the candidate, which aborted a whole run; implausible addresses are now
refused before dereference. New tool `tools/accept-exclusive-input.py`
(read-only, never attaches, never launches). The filter was left disarmed after
every run. Open: cases 5-8, cMoveTurn (deliberately unfiltered), and the
untested cutscene case.

## 2026-10-11 — Why case 2 failed: the run's exit is governed by cMoveEnd

MHW happened to be running in a loaded scene, so the decrypted image was read
once through /proc/<pid>/mem (read-only, no attach, no writes, no launch) and
the transition machinery was mapped out instead of being guessed at.

The local hunter's transition set is `T = H+0xa00`, scanned per frame by
`0x14026bd10` (thunk `0x14118e480`) with a per-state fallback at `0x14026bf70`;
the dispatch `0x14026ba00` resolves the transition object from
`[T+(group+12)*16][index]` and calls its `+0x30` predicate. Each current state
selects its own record table, and the two that matter differ:

- idle `(1,0)` -> `0x143f374b0` (39 records) contains `cMove` (index 0x93) and
  no `cMoveEnd`;
- running `(1,4)` -> `0x143f37fc0` (33 records) contains `cMoveEnd` (0x94,
  required result 0, dest (1,3)) and `cMoveTurn` (0x9b) and no `cMove`.

That is the whole case-2 failure: the trampoline was silent during a run because
the engine was consulting an object the filter never touched. Suppressing
`cMove` could not have stopped a run. `cMoveEnd`'s predicate (`0x1419344f0`)
returns only 0 or -1, eligible meaning "stick magnitude below threshold and the
object's own timer elapsed".

Fix: while exclusive mode is on, the local `cMoveEnd` object also gets a private
vtable copy whose `+0x30` runs the real predicate and rewrites only an
ineligible result to 0, asking the engine for the end transition it would have
selected had the stick been released. Scoped by object identity and
`[obj+0x10]==P` like the move rewrite; `cMoveTurn` is still never rewritten; the
rewrite is inert at rest because the idle table does not contain `cMoveEnd`.

Build output is not byte-reproducible (the PE header carries a timestamp, so
the same source hashes differently every build); identity is `cmp` against the
installed file, as the installer already does. The reply file now reports
`released=` beside `suppressed=`.

Verified headless: `bash tools/test-exclusive-input.sh` (policy tests extended:
end rewrite, foreign end object, foreign player link, aliased resolution still
suppresses the move) and the MinHook cross-build. Verified static, read-only:
the tables, objects and predicates above, from the loaded pinned image.
**NOT verified at runtime.** Installed 2026-10-11 once MHW was closed
(installed SHA-256
`257b9cc6514f5e81c75fdd98bfbe57f92352e1965e6c59dcd4798b0284a79a9e`, `cmp`-clean
against the build; the previous DLL is preserved in
`native/mhw-renderer/build/rollback.78uMwX`). Case 2 still needs its
acceptance run: hold W, arm without releasing, expect the hunter to stop and
`released` to advance. Design evidence in docs/exclusive-input-design.md,
updated handoff in docs/exclusive-input-handoff.md. Scratch dumps and
disassembly helpers live outside the repository.

## 2026-10-11 — Exclusive-input filter installed and live-tested

Built the exclusive-input suppression filter (native/mhw-renderer),
installed it (SHA-256 981858aec9780b0114098e0c69df39f68e1a7392c63961e5f52f2e22d4598cde),
and live-tested the acceptance matrix against pinned build 421810.
Fixed resolver non-determinism (ad54597): the owner keeps several cMove
slots for one player and uses one at a time, so all local slots are now
covered (one shared private vtable copy) and the resolver emits every
slot, sorted.

Acceptance (docs/exclusive-input-design.md): cases 1 (baseline), 3
(enable at rest, W held -> 0.94 units/6s, frozen) and 4 (disable ->
resumes) PASS. Case 2 (enable mid-run, W held) FAILS: the hunter keeps
moving at 195 units/s and the trampoline stays silent. Mechanism proven
live: the vtable +0x30 predicate (0x141934220) gates the TRANSITION
into a move state (consulted every frame at rest, ~85/s) but is not
consulted during a move, so the filter prevents STARTING movement, not
continuing it. An active run ends only via cMoveEnd (on key release),
after which re-press is blocked. Matches the observed behaviour:
movement is not stopped until W is released and re-pressed.

Filter is disarmed; nothing is patched at rest. Open: case 2 (stop an
in-progress run - needs cMoveEnd invoked on arm or the move-state update
gated) and cases 5-8. Full handoff in docs/exclusive-input-handoff.md.

## 2026-10-10 — End-of-day input-investigation handoff

Stopped investigation at the user's request and recorded the next bounded goal
in docs/exclusive-input-investigation.md. Save the accumulated investigation in
incremental documentation commits and push the existing CrafterHunter origin;
no production code, installed artifacts, game assets or scratch disassembly/logs
are included. Earlier "no commit/push" entries describe their investigation turns,
not this subsequently authorized documentation handoff.

Next: resolve positive activating writers of exact local H+0x4af8/+0x4afc
from the verified 0x1411a8750 maintenance path; identify a legitimate nonphysical
movement fixture, then compare it with physical movement/held-button release
at local predicates, including C+0x9e3. Revalidate build and fresh instances
before any separately authorized runtime observation. Do not synthesize state.
The exact writer, conditional keyboard chain and native run capture are progress,
not a safe full hunter-only boundary. Exclusive mode remains unimplemented;
attacks/items, controller, UI/camera isolation and restoration/lifecycle are open.

Documentation scope/whitespace checks only for this handoff; no new game capture,
hooks, input/memory writes, build/install, rendering or bridge changes.

## 2026-10-10 — Manual W reaches the exact local movement transition and native action

Continued the user-authorized offline investigation with universal-modder
reverse-engineering, bounded hardware observations and instruction-level checks.
Pinned verifier passed. Local direction read P+0x1510 led to camera-associated
processing, not an isolated locomotion hook. Local LStick magnitude observation
instead identified exact cMove, cMoveTurn and cMoveEnd transition instances;
initializer, live player/controller links, vtables and DTI agree. Stop/end uses
an inverted threshold and timer, while cMove has other state gates/side effects.

After a timed attempt without a qualifying request, a user-confirmed repeat W
hold captured record (1,0x93,0,0,1,4), accepted SPL-wrapped request with pending
(1,4), actual local nHmAction::cActRun entry and native P+0xe214 write 1.0 ->
0.75 on LWP 14178. Thus native execution is established, not only action metadata
or queued selection. Subsequent release snapshot was magnitude zero/current idle;
this is not an exclusive-mode restoration test. Native entry used 0x14026a820's
pending-to-current loop through 0x14026a1e0. Separate bounded aggregate observation
identified exact copy writers and both main/job-thread scalar reads. Static trace
links keyboard mapped bit 29 -> keyboard Y scalar -> aggregate overwrite ->
local LStick magnitude -> cMove. This is a conditional instruction-supported
keyboard path, not a single-event dynamic device-provenance trace or controller
acceptance. Pad, mouse (other pair), state filters and nonphysical overrides remain.

No safe full hunter-only suppression boundary yet. Whole queries, preparation,
transition methods and action dispatchers remain unsuitable for blanket bypass.
Next gate: distinguish physical movement from a valid nonphysical case at the
local predicates while preserving held-button release/end behavior; fixture and
state producers are missing. Attack/item/UI/camera/controller and hook lifecycle
coverage are not accepted. No suppression or production implementation.

Only report/journal edits; preserved existing changes. Verified pinned identity,
exact objects, instruction/unwind/immediate-caller edges and bounded manual native
execution; git diff --check. Every hardware observation detached/removed probes;
no target-memory writes, software breakpoints, hooks, simulated controls, game
launch/restart, build/install, rendering/bridge change, commit or push.

A final bounded read/access check found maintenance of H+0x4af8/+0x4afc
in 0x1411a8750, immediately called by master update. Positive branches restore
near-zero player direction, decrement timers, test command/magnitude state,
and expire/reset; their positive setters were not observed (both values zero
in the 24-stop window). This strengthens the stateful-processing counterexample,
not a scripted-input classification or safe-hook claim.

## 2026-10-10 — Scalar pair consumers, position override and shared camera-associated work

Continued the exact local writer trace using universal-modder reverse-engineering
and bounded static loaded-code analysis; no new debugger or input stimulus.
Mapped the four aggregate queries to H+0x30..0x3c and exact embedded
cPlayerCommandLStick/RStick instances. Constructor, live vtables and DTI getters
agree; verified leaf processors calculate magnitude/normalized state, with a
mapped-command alternative for RStick. First-pair reconstruction produces
direction vectors copied into exact local player+0x1510/+0x7880. Timed
position-derived override clears H+0x9f3 = C+0x9e3, selecting the direct-vector
branch instead of camera-relative processing. Its timer/position producers and
action-selector meanings remain unknown.

The instance-preserving H+0x1410 helper retains the same controller, player
and verified uMhCamera. Consumer 0x1412ab4a0 reads BOTH scalar pairs into
its +0x1e60 accumulator; connected spatial calculations use that accumulator
and camera state. Therefore neither raw pair is established as movement-only.
Local direction-state writes do not yet identify a locomotion/attack/item
executor or safe hunter-only suppression point. Controller provenance, UI
scope, scheduler/SPL order and hook lifecycle remain unproved. Next recommended
evidence is a separately authorized bounded read observation of local P+0x1510
to identify its consequential downstream consumer; not performed here.

Only investigation documentation changed; preserved prior edits. Exact-instance,
constructor/DTI, instruction-boundary and chained-unwind checks; final assertions
and git diff --check. No production code/build/install, game launch/restart,
debugger attachment, hooks, memory writes, GUI automation, commit or push.

## 2026-10-10 — Exact local source writer captured with a hardware watchpoint

User authorized the next bounded runtime investigation and confirmed an offline,
loaded scene. Revalidated the pinned build and current local player/controller,
then captured one hardware-watchpoint change at exact C+0x18. Writer
0x1411a9cc7 stores through H+0x28; observed source changed 0 -> 0x20000000
on LWP 14178. Its loop copies 46 indexed results from 0x141b164c0 using
BTS/BTR; immediate master-update caller is supported by prologue/stack and
call-site evidence, not GDB's unreliable PE backtrace. Bounded follow-up links
native keyboard state +0x138 through mapping +0x26c8 to this exact local source.
Physical-keyboard reachability is instruction-supported; the captured true
result's device branch and physical-only provenance are not established.
Mouse/controller/callback alternatives, bit overrides, scalar outputs and a
timed position-derived branch prevent treating the whole routine as input-only.
No safe suppression boundary yet. Next: classify the local scalar outputs and
their immediate consumers/nonphysical override. Details in the investigation.

The initial GDB attachment refused an internal software breakpoint under the
no-memory-write permission; its residual group stop was cleared for this exact
PID. Hardware-only attachment then succeeded. Watchpoint deleted, debugger
detached; final TracerPid=0 and process running normally. Temporary debugger
stops/debug-register instrumentation occurred, but no game-memory writes,
software breakpoints, hooks, automated input, launches or installed changes.
Scratch evidence is outside the repo. Pinned-build verifier PASS;
git diff --check; documentation only, existing edits preserved, no commit/push.

## 2026-10-10 — Exact command-controller alias escape; writer still unresolved

Returned to the local embedded source instead of extending the light-resource
lead. Verified master setup passes human-controller+0x10 as the fifth argument
to 0x1412a8a90; that helper retains the same pointer at receiver+0x1ec0
and copies it into subordinate records' +0x40. Its bulk clears target separate
storage. This expands the exact-instance alias inventory but proves no write
to command-controller+0x18. Bounded pointer/leaf-accessor and chained-unwind
checks found no verified runtime source assignment. Generic indexed setter
capability still lacks an exact-instance invocation and physical provenance.
No suppression boundary. Recommended different evidence: a future scoped
debugger write watchpoint on a freshly validated source qword, after initialization,
capturing instruction/destination/registers/call stack and changed bits. Not
performed here. Documentation only; prior edits preserved; git diff --check;
no launch, instrumentation, hooks, process-memory writes, installed/executable
changes, commit or push.

## 2026-10-10 — Named-record loader and light factories resolved statically

Following the universal-modder reverse-engineering evidence workflow, traced
component+0x90 assignment to an rLch resource-loading caller. Its loader
constructs rLch::Chr groups and supports rLch::OverrideParamHolder named
records (with a separate dynamic-ID fallback). The bounded child factory's
twelve alternatives return uLlk light DTIs/factories, not virtual-pad DTIs.
Traced stream-backed selector construction and BoolUnitProperty's initial
value byte/property-string loading; factory discriminator and operation type
are distinct stream fields and must not be conflated. Concrete selected
record, literal property, successful selector match and runtime instance
remain unproved. No physical-input provenance or safe suppression boundary.
Details and addresses are in docs/exclusive-input-investigation.md. Only
documentation changed; existing edits preserved; git diff --check. No launch,
hooks, instrumentation, game-memory writes, installed/executable changes,
commit or push.

## 2026-10-10 — Target-array assignment and property record traced

Verified 0x1411fe55f stores a factory-produced pointer in the target array.
The array is container+0x18 within component+0x98; adjusted container helpers
explain allocation, pointer copying and zero initialization. Target production
follows data group/child selection, child virtual +0x128, then returned-object
virtual +8. Property text comes from a type-3 value entry in a separate named
record; execution matches the operation's selector to the child's string.
Selected factory identity, literal property text and concrete runtime pairing
remain unresolved. No local bitset alias, physical-input provenance or safe
suppression boundary claimed. Documentation only; unrelated edits preserved;
git diff --check; no launch, hooks, instrumentation, memory writes, installed
or executable changes, commit or push.

## 2026-10-10 — Property operation target and name narrowed

Static construction installs 0x1411fdfe0 as a small operation's virtual +8;
its property name is a caller-supplied, capped inline string, and its value
byte comes from the construction caller. Verified dispatch 0x141200fe9
passes an element of component+0xb0's object array as the target. The
operation lives in a three-slot component list. No array-element alias to
the local command bitset or actual "Bit" string binding is proved.
The narrowed next edge is the selected target-array element's assignment
and its corresponding source property string. No physical-input provenance
or safe suppression boundary claimed. Documentation only, unrelated edits
preserved; git diff --check; no executable/installed changes, launch, hooks,
instrumentation, memory writes, commit or push.

## 2026-10-10 — Indexed property setter dispatch resolved

Pinned static instructions now connect generic property enumeration/lookup
0x1421711f0 to dispatcher 0x14218f3f0: descriptor+0x18 supplies the
receiver, +0x40 supplies the index, +0x30 supplies the tail-call target,
and the caller's byte supplies set/clear. For the known bitset descriptor
this would reach 0x140268500. Caller 0x1411fe07b supplies index zero
and a float-state-selected byte, but its object/name have no proven alias
to the exact local command bitset. Generic property-copy code independently
corroborates dispatch semantics. No physical provenance or suppression
boundary claimed. Next: resolve that caller's incoming target object and
property name. Documentation only, existing edits preserved; git diff --check;
no executable/installed changes, launch, hooks, instrumentation, memory/input
writes, commit or push.

## 2026-10-10 — Embedded source bitset property setter identified

Read-only pinned instructions resolve bitset_prop<46> registration descriptor
receiver/getter/count/setter aliases. Generic setter 0x140268500 performs
indexed BTS/BTR dword writes at bitset+8, which would reach command source
+0x18 on the embedded receiver. No invocation on that exact instance or
device provenance is proved; descriptor copying is not a source-bit copy.
Targeted direct-reference search found no calls/jumps to the setter; indirect
descriptor dispatch remains the precise blocker. Next: resolve a bound
descriptor+0x30 invocation and its index/value producers. No suppression
boundary claimed. Documentation only; unrelated edits preserved; git diff
--check; no executable/installed behavior, hooks, instrumentation, memory
writes, game launch, commit or push.

## 2026-10-10 — Source bitset initialization and dependency narrowed

Read-only pinned code establishes command-controller+8 as cVirtualPad and its
embedded bitset_prop<46> storage at command-controller+0x18. Constructor
0x141371ce7 zeros that exact source qword. Source bits 0/1 feed records used
for outputs 2/3 and 4/5 respectively, with history/timer and player gates;
there is no identity mapping or independently established action meaning.
Static master/base update ordering is recorded. No non-initialization writer
or physical-device-exclusive provenance was verified; bounded search is not
exhaustive. Next: alias/xref trace of embedded virtual-pad/bitset pointers,
including helper/bulk writes. Documentation only, existing edits preserved;
git diff --check; no game behavior, installation, hook, input write, launch,
commit or push.

## 2026-10-10 — Command-bit owner and reconstruction resolved

Read-only pinned loaded-code analysis identifies controller+0x10 as
cPlayerCommandController through vtable/getter/DTI evidence. The transition
queries output bits 2..5 at +0x780; +0x860/+0x8a0 are query bookkeeping.
Verified controller caller 0x14118e350 -> 0x141230bb0 expands source +0x18
bits into timed/edge-maintained records, clears and reconstructs output bits,
then conditionally filters them. Exact source-bitfield producers and physical
device provenance remain unknown; no hunter-only suppression boundary claimed.
Next: bounded trace of this object's +0x18 assignments, not global query hooks.
Only investigation documentation changed; git diff --check; no installation,
executable behavior, hooks, input writes, launch, commit or push.

## 2026-10-10 — SPL DoAction detour and original path resolved

Installed Core decompilation/cache plus loaded delegate/JIT chain identify
0x140269ce0 as SPL ActionController's existing DoAction hook. It notifies
entity/local-player callbacks by reference, then calls Original on normal
paths; exceptions can interrupt that call. Actual OriginalFunction delegate
points to trampoline 0xfffc0060, which restores displaced instructions and
returns to 0x140269ce7. CrafterHunter implements neither action callback.
Native request can enter current state or queue it. A verified conditional
consumer 0x14026a2d0 -> 0x14026a1e0 dispatches current action +0x30; entry
495's target is 0x140eef2f0. No observed execution of destination (1,495),
action semantics or physical-input-only boundary is claimed. Next: command-bit
producer provenance upstream of the transition predicate. Detailed evidence
in the investigation report; git diff --check passed. Documentation only,
no game writes, instrumentation, installed changes, launch, commit or push.

## 2026-10-10 — transition result to action-state selection, read-only

Verified generic vtable+0x30 dispatch at 0x14026ba7c against actual
PopGimmickButton instances in the local controller's constructed +0xa00
table owner. Consumers match the returned integer to selection records;
-1 rejects a record, while 0 can select a destination pair. Optional
callbacks can alter results. One concrete record targets (1,495), whose
action entry metadata names nHmAction::cActEnvBashKick; actual action semantics
remain unproven. Downstream 0x140269ce0 is already detoured, limiting claims
about live state-update execution. No hunter-only suppression boundary.
Next: establish that existing detour's provenance and original-call behavior.
Full bounded-search coverage and evidence in the investigation report.
git diff --check passed. Documentation only; existing changes preserved.
No hooks, instrumentation, memory/input writes, installed/executable changes,
game launches, commits or pushes.

## 2026-10-10 — classify transition 0x2a early-out, investigation only

Read-only loaded-code inspection verifies 0x140ef5db0 in the constructed
nHmTransition::cPopGimmickButton vtable, with additional transition-family
table references. Active sPlayer state 0x2a returns -1 after controller
registration/cache maintenance, bypassing eligibility/command queries and
success flag writes. It does not establish movement/attack semantics or a
hunter-only suppression boundary. Next: trace consumption of its virtual
0/-1 result into transition/action selection. Details and exact evidence are
in docs/exclusive-input-investigation.md. git diff --check passed; documentation
only, no hooks, memory/input writes, executable/installed changes, launches,
commits or pushes. Existing unrelated edits preserved.

## 2026-10-09 — pinned-build input trace, no executable changes

At b421dc0, verified executable SHA/revision/Steam build and installed SPL bytes
against its 2.0.0 Linux archive. Read-only access to already-running PID 110141
allowed loaded-code inspection (on-disk text remains encrypted). Keyboard writer
0x1422E7AD0 is already SPL-detoured; its native caller produces edge state after
the writer. Installed SPL filters shared keyboard/mouse state for ImGui, not a
verified hunter-only consumer. Independently resolved the game's XInputGetState
thunk to installed Wine xinput9_1_0; decoded a pad routine with XInput and
DirectInput branches. No controller gameplay support is claimed. Installed hook
and unload code has cleanup/thread-order caveats; OnUpdate alone does not prove
post-poll/pre-hunter timing. The semantic hunter consumer remains unidentified.
Full evidence, candidate ranking and exact missing prerequisite appended to
docs/exclusive-input-investigation.md. Host verification passed; no observer
hook, input write, installation, game launch or production change. Extracted
excerpts stay outside the repository. Documentation left uncommitted/unpushed
as requested for review.

## 2026-10-09 — reference input ownership investigation, no suppression installed

Reference source/history at d9cac469 shows F8 window-focus handoff: guest pauses,
releases mouse/hides window; host requests foreground focus; host F8 counter
shows/focuses guest again. Cocoa prevents transparent-window click-through.
This is not a verified hunter gameplay input hook and does not establish
controller suppression. SPL queries and pre-main-update callback do not prove
the required polling/consumption order. Stopped per user's explicit safe-hook
gate rather than clearing global input or replacing the loader. Comparison,
source links, missing prerequisite and future manual plan are documented in
docs/exclusive-input-investigation.md. No code copied or binaries changed.
User manually accepted player-preview occlusion and movement/pose in the tested
setup, including seated boat pose; this is not exclusive input or terrain support.

## 2026-10-09 — player depth viewport conversion, live acceptance pending

User's fixed-camera real/source screenshots show foliage-shaped depth rejection
over Steve against visible sky. Runtime selected depth allocation is 1920x1080,
but observed writer viewport is (0,0,1440,810); player/pre-UI viewport is full-size.
With approval, map player pixels to the selected resource's observed writer
viewport before integer Load. Track first depth-enabled/write-enabled draw each
frame in normal mode, not only debug mode. Missing/stale/invalid viewport refuses
player draw; no guessed scale, lifecycle guard relaxation or depth comparison change.
Diagnostic shaders/probes share the conversion. Geometry/skin/pose/anchor and
stone/frame/paired paths unchanged. Regression checks cover the measured torso
sample (781.945,371.781)->(586,278), identity, nonzero origin and pixel centres.
First-writer viewport correspondence remains a live acceptance requirement, not
proven solely by metadata. No occlusion acceptance or gameplay claim.

## 2026-10-08 — player-model artifacts installed, runtime gate still open

After all builds/headless checks passed, closed only Minecraft game PID 322466
with SIGTERM (MHW was already absent); launchers were left untouched. Installed
a704151's Fabric 0.3.0 JAR and native renderer DLL in their existing mod/plugin
locations. No managed binary change was needed. Byte comparisons match builds.
Fabric SHA-256: 68d76bd7d5a396e2fd78b861ce1e2a7394d772894b4560f4d0a07766dbdc660a.
Native SHA-256: 27a25c38dcd611b9f52d2512281e5b8a71df836dc00b1e7639129732935610eb.
Scoped previous artifacts: ~/.local/share/crafterhunter-backups/player-model-install.5s1taW/;
native flag/license/DLL rollback: native/mhw-renderer/build/rollback.xhRcfa/.
Player flags remain opt-in, game saves/worlds and launch configuration unchanged.
Games not relaunched. Live skin/export/submission are NOT proven; the orbit test
must wait for fresh complete-player upload and draw-submission evidence.

## 2026-10-08 — baked complete player model, headless checkpoint

User observed that the paired preview only includes surfaces visible to the
Minecraft camera. Confirmed: the paired shader reconstructs guestDepth samples,
not hidden geometry. Enlarging those samples cannot supply a complete back/side.
User approved investigation then the smallest player-only geometry slice.

Inspected local 26.2 bytecode: AvatarRenderer (not PlayerRenderer) extracts
AvatarRenderState; LivingEntityRenderer.submit supplies body PoseStack; PlayerModel
setupAnim supplies real bones; ModelPart.Cube polygons supply every face and UV.
Implemented a non-drawing collector for only the actual main player model,
restoring shared poses/visibility afterward. Static baked triangles + resident
skin travel through bounded atomic player.asset; per-part matrices/feet through
player.pose at the existing 4 Hz budget. Existing feet anchor and lifecycle guards
remain authoritative. Added an opt-in pre-UI structured-buffer player draw using
the same host camera/depth. Frame/stone shaders and bridge are not replaced.

Actual normal/slim baked model tests prove 12 cubes, 72 faces including all six
directions, 432 triangle vertices, normalized UVs and native walk/head pose
changes. Actual baked geometry/matrices roundtrip Java/native. Texture fixture
is synthetic, NOT live skin proof. Passed native malformed/matching/freshness/
negative feet tests, isolated controller contracts, all eight shader entry-point
checks, Fabric/native/managed builds, existing reprojection/frame/capture/depth/
SPL regression suites and 26 Rust tests. No runtime draw/skin/orbit/FPS acceptance.

Scope/risks/commands recorded in docs/player-model-preview.md. Armor, items,
cape and other layers are excluded. No world export, collision or gameplay added.

## 2026-10-08 — source-sized paired surface footprints

User screenshots show the actual Minecraft skin in MHW in front/rear F5 views,
but dotted surfaces. MHW was already closed; terminated only the Minecraft game
PID with SIGTERM and confirmed both games absent. No saves/worlds were edited.

Fixed the existing paired shader's fixed four-host-pixel patches: each sample now
projects its four-source-pixel cell at that sample's depth through the existing
camera/anchor transforms. At 949 -> 1920 width, the old patches leave about 4.09
pixels between equal-depth samples. Sample/vertex counts, calibration, lifecycle
guards, scene-depth selection and stone rendering are unchanged. No gameplay added.

Passed reprojection/anchor numerical regressions (resolution, aspect, small
targets, shared edges and camera magnification), preview contracts, all shader
entry-point checks, depth selection, native build and 26 Rust tests. These are
headless checks, not live surface-quality or performance acceptance.

Installed only CrafterHunter.Render.dll via the existing installer; installed and
built SHA-256 match: 3d4831a9b287607e1d4db40dde7e984614192d10797a4829b464b866cee5aad1.
Previous renderer/flag/license are in native/mhw-renderer/build/rollback.wV9sRF.
Games remain closed. Coarse depth edges and disocclusion remain limitations;
next manual gate is surface continuity with the same first-person align then F5.

## 2026-10-08 — player-anchor artifacts installed

User requested automatic installation after verified changes. Confirmed no MHW
or Minecraft game process/window; launcher and Gradle daemon were left untouched.
Installed b041c2d's matching managed/native DLLs in the existing MHW CrafterHunter
plugin directory and Fabric 0.3.0 JAR in ~/.minecraft/mods. Existing scoped files
backed up under ~/.local/share/crafterhunter-backups/player-anchor-install.9ZLcv3/
(mhw/ and minecraft/); native installer also created build/rollback.HL92qZ.
All three installed artifacts compare byte-identical to their built sources;
managed install marker matches the installed SHA-256. Bridge remains active on
127.0.0.1:38470. No saves/worlds, other mods or launch settings changed; no games
launched. Player visibility and alignment still require manual acceptance.

## 2026-10-08 — player-feet calibration, existing compositor preserved

User supplied visible village point-cloud screenshots, then requested both games
closed before the next scoped fix. MHW was already closed; Minecraft received
SIGTERM (no SIGKILL), and both process absences were checked before changes.
No save/world files were edited. Games remain closed.

Camera-to-camera calibration puts the guest player's eye at the host viewpoint,
not at a visible player reference. Added snapshot v3 within the existing 320-byte
header: finite interpolated local-player feet at 280, provenance at 304. Level
render HEAD samples the same frame's partial tick; issue-time frozen copies travel
with both attachments. v2 archives remain readable but cannot player-calibrate/draw.
Native calibration now maps guest feet to existing SPL Player.MainPlayer.Position
in metres. Camera continuity guards, first-person calibration requirement, fixed
F5 anchor, shader/depth selection and renderer architecture are unchanged. Missing
host player invalidates; no guessed height offset, movement writes or input handoff.

Passed Fabric and managed/native builds; Java/native v2/v3 pairing, nonfinite and
provenance refusals, feet-to-feet/F5 anchor regression, existing reprojection,
capture-boundary, preview, shader, depth selection, SPL lifecycle and Rust tests.
Not deployed or runtime accepted. Sparse holes and front-camera source coverage
remain limitations; the hunter may occlude a coincident Steve. No claim that this
fix alone makes Steve visible in every F5 view or completes playable MVP.

## 2026-10-08 — visible-workspace preview reaches paired draw submission

With both user-launched games on inactive workspace 2 and the terminal on
workspace 1, new cadence logs measured consecutive producer identities about
1000 ms apart, matching consumer intervals. This is producer slowdown, not a
4 Hz stream sampled at 1 Hz. The user switched to workspace 2; without changing
thresholds or composition code, producer intervals settled near 255 ms.

Generation 1113423980880: calibration accepted at tick 1280720, epoch 1372;
scene depth 0 measured 56.56% coverage and selected at 1281847; actual paired
reprojection draw submitted at 1281907. Epoch 1372 remained armed through
1366306, over 85 seconds. This passes the immediate upload/calibration/eligibility/
submission gates in the visible-workspace setup, not visible or aligned rendering.
The exact guard trigger in the earlier session was not recorded and cannot be
reconstructed; off-workspace one-second observations demonstrably put both
freshness and host continuity at their existing thresholds. No guard bypass.

The session-scoped bridge remained active with both roles registered and UDP
38470 listening. Earlier listener loss remains unexplained: no historical exit
record proves shutdown versus crash. Updated preview instructions require both
windows visible on the active workspace. Exact compositor/driver throttling and
visual depth/alignment/F5 acceptance remain unproven. No gameplay changes.

## 2026-10-08 — failed preview investigation, live gate still blocked

Fresh native logs show accepted calibrations followed by the combined host
jump/gap/invalid-pose refusal, with one-second upload intervals and no paired
draw submission. Those logs do not record the triggering pose or observation
interval, so an exact root cause cannot be claimed retroactively. Both games
and the bridge were closed when this investigation reached runtime checks.

Added narrowly scoped refusal classification with old/new host positions,
distance and host-clock times; preserved all existing thresholds and reset
behavior. Added same-generation producer issue-time intervals, consumer
observation intervals and identity steps to distinguish producer throttling
from consumer sampling. Upload-driven anchor loss is now explicitly logged.
Regression checks retain exact 1 s/25 m boundaries and distinguish NaN,
rollback, gap and jump. No gameplay or composition behavior changed.

Local 26.2 bytecode shows AFK/minimized frame throttles of 10/30 FPS, not
1 FPS; pauseOnLostFocus is already false. This rules out that option alone,
not other scheduling stalls. PortUnreachable proves an unavailable listener,
not whether it exited or was never started. The paired frame channel is
shared memory, independent of bridge UDP. Runtime gates remain unaccepted;
these diagnostics are required before choosing a non-speculative fix.

## 2026-10-08 — opt-in real paired GPU composition slice, not playable acceptance

User narrowed work to Playable Steve MVP and required implementation rather than
further diagnostics. Audit confirms paired GPU uploads were unused by drawing,
old colour rendering remained sky-only, host player proxy runs in the opposite
direction, session/ownership negotiation and guest terrain collision are missing.

Added actual paired reprojection draw at the existing pinned pre-UI signature,
reusing measured scene-depth selection and saved context state. Real guest depth
unprojects through captured inverse projection/view/position and explicit anchor;
host metres become GPU centimetres once. Guest samples test against host reversed
depth and private depth orders overlapping guest samples. Preview samples every
fourth texel, not continuous geometry; terrain is included and holes expected.
New flag never silently falls back to sky-only on refusal. Stone drawing unchanged.

Fabric opt-in bounded streaming supplies at most four paired captures/second,
one in flight, skipping diagnostic artifact writes without weakening failure
retention. Preview control start/align/stop uses atomic writes and preserves frame
channels. Lifecycle cleanup added for all paired GPU resources on CH_Stop.
No input takeover, host camera writes, terrain import or gameplay claim.

Verified Fabric build; managed/native builds; six shader entry points compile
under glslang (not proof of D3D runtime compilation); paired transport/freshness,
reprojection/alignment, Fabric capture and Rust tests pass. Controller regression
tests cover temporary-root start/align/idempotent stop/channel preservation; shader
contract checks cover resources, host-depth comparison, origin refusal and bounds.
No deployment or live demonstration. Next: one scoped manual composition test,
with screenshots, advancing identities and before/after focused frame rate. See
docs/playable-steve-mvp.md for audit, hypothesis, acceptance and known limitations.

## 2026-10-08 — host-local calibration diagnostic, no guest drawing

Added opt-in align.request consumption by rename-to-claim, requiring a fresh paired
upload in first-person. CH_AlignmentHost observes SPL camera in metres; explicit
calibration fixes guest/host camera origins, and logs transformed camera position
with generation/identity/F5 mode/host-local epoch. F5 does not recalibrate. Host
jump >25 m, gap >1 s, clock rollback/invalid pose or lost upload clears the anchor.
These are continuity heuristics, not an authoritative area/title detector.
Control tool now publishes complete requests using atomic same-directory replace.

Verified: reprojection/alignment regression tests including jump/gap/rollback and
third-person calibration refusal, managed plugin build, native MinGW renderer build.
No deployment/game requests or live acceptance. Stone/depth selection and sky-only
composition untouched. Full calibration protocol/ownership acknowledgement and
visual alignment remain missing. Diagnostic needs advancing paired uploads within
the existing one-second watchdog, so bounded pacing/trace collection comes next
before asking for manual acceptance. No 60 fps or visible Steve claim.

## 2026-10-08 — explicit shared-world anchor reference, drawing unchanged

Reviewed three user F5 archives (215619Z/215700Z/215730Z): identities 3/6/9
in generation 700129956865, matched colour/depth hashes, all finite depth,
colour-aligned bottom-up rows and recorded zero-to-one reversed-Z projection.
Rear/front camera displacements measure 4.0000002 blocks, opposite about the
first-person position. This supports render-camera capture, not host alignment.

Added world_alignment.hpp: explicit same-XYZ guest/host origins and positive
metres-per-block scale; requires armed nonzero matching guest generation/host
epoch. Composes the actual captured view inverse and camera position without
per-F5 re-anchoring. Headless tests require an unchanged block to map identically
through all three camera modes, with translation/scale/inverse and stale/reset
refusal. Existing reprojection test entry point runs these too.

No native drawing, telemetry protocol, input, deployment or game requests changed.
This is the mathematical contract only: calibration exchange and host epoch
lifecycle still need implementation before a manual alignment diagnostic exists.
Agent/manual next objectives are recorded in AGENTS.md and world-reprojection.md.

## 2026-10-07 — exact render arguments and paired snapshot v2

Local Loom 26.2 bytecode establishes GameRenderer copies CameraRenderState's
projection, applies bob/hurt and nausea transforms, then uploads that modified
Matrix4f before invoking LevelRenderer.render with actual camera and view arguments.
The previous resampled CameraRenderState projection missed those effects. Added
observational ModifyArg on the final upload (returns the identical object), and
LevelRenderer HEAD capture of the actual view and double-precision camera position.
Per-world-frame reset prevents last-frame metadata surviving a missing hook.

Paired v2 header carries final projection, actual view rotation, f64 position, all
three F5 modes, queried GL clip-control mapping/origin and depth range, and pinned
main-pass clear 0 provenance. Missing exact arguments/backend metadata refuses
transport, not GPU-lifetime policy or diagnostic publication. Native rejects v1
and unknown provenance, prepares inverse projection/view and eye-to-guest transform
for upload-only staging. Existing stone and colour-only drawing unchanged.

Headless bytecode checks pin effect-before-upload-before-level ordering, exact hook
signature, reset and main-pass clear. Java/native round trip covers all F5 mode
values, f64 precision, view/mapping offsets, restart/resize and twelve malformed
refusals. Fabric/native builds, capture completion/readback-plan tests, 17 depth
validator regressions and affine view/position reprojection checks pass.
This does NOT establish real F5 camera alignment, native GPU contents or
visible Steve. No deployment or live requests. Next fresh v2 runtime camera evidence
and guest/host shared anchor conversion before depth-aware composition.

## 2026-10-07 — projection reference and real archived-block replay

Added column-major inverse/unprojection and explicit eye-to-host/current-host-VP
projection reference, with host-space reversed-Z occlusion. Refuses invalid/clear
depth, unusable matrices, behind-camera and out-of-frustum points. Window mapping
and clear value are explicit caller inputs, not guessed from matrix coefficients.
Upload-only staging now retains matching inverse projection and refuses singular
projections before allocation; neither existing drawing path binds new views.

Native read-only replay of the two hash-bound archived bedrock captures reconstructs
4.894803 and 12.121316 blocks versus independently known 4.895355 and 12.122026.
Ray/face intersection is checked, not just infinite Z-plane distance. Headless math
tests cover pixel centres, bottom-up orientation, off-axis rays, translation/rotation,
window mappings, clipping and invalid cases; native build and paired transport
regressions pass. No replacement Minecraft geometry or
assets generated, no deployment or runtime requests. This is not live GPU alignment.

Snapshot v1 lacks full issue-time view/effects and depth-mapping provenance. The
next gate is obtaining those and validating guest/host anchor conversion, not
guessing an exact camera from player Euler angles. docs/world-reprojection.md and
AGENTS.md record the boundary for the next agent.
User confirms the target includes all native F5 camera modes; added that requirement
to the agent contract. First/third-person gameplay remains unimplemented.

## 2026-10-07 — freshness-gated paired upload, not composition

Added host-local watchdog and bounded generation retirement for world snapshots.
Startup/restart requires identity advance; duplicates cannot refresh liveness,
regression and retired generations are refused, and generation exhaustion fails
closed. Native upload-only diagnostic is off by default (world-upload.enabled).
It polls at 250 ms intervals, atomically stages immutable RGBA8/R32_FLOAT textures
and views with matching issue-time metadata, releases CPU attachments, and clears
staging on expiry/refusal/failure/disable. It never binds these textures for drawing.
Stone shader, scene-depth selection and existing colour-only composition unchanged.

Headless transport tests include startup stale-file refusal, duplicate expiry,
recovery by advance, local clock rollback, restart warmup, retired-generation
refusal and bounded exhaustion. MinGW renderer build, existing shader compilation,
depth selection, frame-source, frame-clock and composite checks pass. No installation or
runtime request writes; real Proton file sharing and GPU upload contents remain
unverified. This is request-paced scaffolding, not 60 fps or visible Steve.
Next: projection-normalised reprojection, then scoped live upload/composition
acceptance with advancing identities, real blocks and current host-camera evidence.

## 2026-10-07 — manual block-depth gate and bounded paired transport

User screenshots plus archived captures establish two distances to existing
bedrock at (917,-60,348), F3 target confirmed in the second screenshot.
Archives 20261007T132237Z and 20261007T132722Z: identities 9 and 12,
generation 1391383374261, centre raw pixel (474,514), 949x1028, FOV 70.
Expected centre-ray distances to the known near-face Z plane are 4.895355 and
12.122026 blocks, from pose and block coordinates independently of depth;
reconstructed about 4.895 and 12.121. Strict validator passed hashes, projection,
finite scene depth and colour/depth orientation at both distances. Both used
pre-hand-clear fallback; this does not verify the optional always-on-top hook,
linked MHW camera, isolated Steve or guest composition. Prior 4.9-degree FOV
did not reproduce at these independent-camera poses, not a global FOV fix claim.

Implemented request-paced WorldFrameChannel publication of a complete paired
snapshot with issue-time generation/identity, pose and projection. Atomic
same-directory replacement isolates incomplete writes; bounded size and malformed
header checks precede native-reader allocation. Separate world.frame path never
modifies the existing colour channel. Transport refusal preserves diagnostic
publication. Added portable native reader, not connected to renderer.cpp, and
docs/world-frame-transport.md with byte offsets and explicit freshness limitations.

Verified headless Java/native binary round trip; pair consistency during concurrent
Java reads; restart/resize; wrong-size producer refusal retaining prior file; eight
native malformed-file refusals retaining prior output. Fabric build and existing
capture tests pass. No deployment, game launches or runtime requests in this turn;
Windows/Proton consumer sharing, linked pose fidelity, continuous throughput and
native depth composition remain unverified. Next: freshness-aware host selection
and upload, before projection-normalised depth composition.

## 2026-10-07 — strict block-distance acceptance, not copy-completion success

The next goal remains actual world-depth acceptance, not native composition.
Implemented an opt-in --acceptance mode in the existing depth validator:
requires BOTH, captureBoundary, positive generation/identity, matched file hashes,
finite in-range non-flat depth, a finite recorded projection, CONFIRMED colour/depth
orientation, and independently known camera-forward surface distance. Diagnostic
mode remains available for old artifacts without pretending its exit zero proves
the acceptance gate. Pixel bounds and positive finite ground truth are now checked;
negative Python indices previously sampled a different pixel silently.

WorldCapture now publishes SHA-256 hashes of the exact raw attachments with each
metadata identity. They detect stale/mixed or partially overwritten files, not
physical pixel validity. No asynchronous resource-lifetime policy, hook, live frame
channel or native compositor changed. The runtime script now explicitly calls a
publish completion rather than claiming valid paired rendering.

Added docs/world-depth-acceptance.md and an AGENTS handoff: two existing opaque
Minecraft block measurements, independent surface coordinates, raw bottom-up pixel
indexing, fresh screenshots/logs and archive, with the previous FOV observation
still unresolved. No new meshes, synthetic game scenes or gameplay writes.

Verified headless: 17 validator regressions pass (including all-zero depth,
undecidable orientation despite a plausible pixel, mismatched files, absent ground
truth, wrong distance, negative pixel and NaN); bytecode hook check passes;
frame/readback-plan tests pass; runtime script bash syntax passes; Fabric build
passes. No deployment, launcher tasks, runtime request writes or live acceptance
performed. Next: permission-scoped deployment/restart and the documented real-block
gate; do not mark Steve/depth-correct guest composition complete from these tests.

## 2026-10-07 — capture before the earlier always-on-top depth clear

Read-only inspection of the latest capture (14:50 local, identity 1) found
depthMin=depthMax=0 and depthDistinctApprox=1 despite both copies completing.
Loom's 26.2 LevelRenderer bytecode exposes a destructive clear in
lambda$addAlwaysOnTopPass$0 before executeAlwaysOnTop. This frame-graph pass
executes before LevelRenderer returns, so the existing GameRenderer pre-hand-clear
hook was already too late when this pass ran. This is a concrete earlier erase,
not evidence that the OpenGL readback recipe needs replacing again.

Added LevelWorldCaptureMixin immediately before that earlier clear. The pass is
conditional (hasAnyAlwaysOnTop); the GameRenderer hook remains as fallback when
it is absent. A per-renderLevel guard services capture exactly once, reset at
HEAD. Metadata records captureBoundary for the issued identity, not whichever
boundary a later frame reached. Both mixins require one injection match; the
synthetic method is explicitly version-pinned. No native renderer, transport,
GPU lifetime policy, installed game files or saves were changed. Corrected the
plan's overclaim about accepted full guest colour/depth composition.

Headless checks: python3 tools/test-world-capture-hook.py pins the early-clear
method, its bytecode ordering, optional-pass predicate, fallback wiring and
registered mixins (the old hand-only hook fails this check); bash
tools/test-fabric-frame.sh passed; python3 tools/test-validate-world-depth.py
passed all eight fixtures; Fabric ./gradlew build passed; git diff --check passed.

Not verified: runtime mixin application or nonzero scene depth after this change.
No deployment or game launch performed. Next agent: deploy only with permission,
restart the guest, archive a fresh paired capture with captureBoundary and
matching identity/projection/colour/depth, and test an existing Minecraft block
at independently known camera-space distances. Verify orientation against the
scene, not capacity or callbacks. The previous 4.9001036-degree FOV remains an
unexplained observation, not a corrected projection or a proven FOV bug. Do not
change native composition or begin gameplay integration until that gate passes.

## 2026-10-04 — scope and recon

User target: real Minecraft inside MHW Iceborne with world, monster, and story
interaction. Existing repository already chooses a separate-process bridge.
Created `MODDING_PLAN.md` with ordered acceptance milestones and explicit
ownership of simulation and progression.

Read universal-modder mod-any-game, game-recon, mashup-mods, and
share-field-notes instructions. Checked the plugin's local knowledge index;
no MHW-specific entry. The CLI is absent from PATH, but the plugin's source
and skills are locally available.

`tools/doctor-linux.sh` found the host installation, Proton prefix, SPL
bootstrap, CrafterHunter endpoint, and build prerequisites. Fingerprint matches
the previous recorded executable and Steam build. No game installation changed.
Reviewed public SPL callback docs and the credited macOS crossover README.
Full story compatibility is not established by these sources.

`cargo test --workspace`: seven tests passed. Direct execution of
`tools/test-fabric-camera.sh` failed because the file is not executable;
run it using Bash without changing file permissions.
`bash tools/test-fabric-camera.sh`: passed anchor, movement, yaw wrapping,
recovery, world change, toggle, decode, and freshness checks. These are
headless checks, not evidence of live game compatibility.

Next acceptance: live camera test, then a depth-composited cube on Linux/DXVK.
Ownership/offline test profile confirmation is pending. No gameplay mutation,
save editing, or game launch was performed during recon.

## 2026-10-04 — live camera and rendering probe

User confirmed that Minecraft follows the MHW camera while both games run.
Screenshots show the Fabric HUD reporting a live packet feed and bright green
probe geometry in an MHW expedition. The first probe used an assumed camera
forward axis; its edges appeared off-centre and sometimes over foreground
objects. This is evidence that `OnRender` and primitive drawing work, but not
proof of correct placement or depth occlusion.

Version 0.2.1 derives the probe's centre ray from the viewport matrices.
Headless tests pass for two view directions, reversed depth, and a singular
matrix; the plugin builds with no warnings. The user still needs to check its
placement and whether MHW terrain hides it. No game was launched or installed
by the agent during this change.

Further 0.2.1 screenshots from Astera's smithy show only a few green cube
edges. The installed DLL hash matches the published 0.2.1 artifact. Because
the smithy is full of close geometry and the camera is aimed steeply down or
up, these images cannot distinguish clipping/occlusion from a placement error.
The next acceptance scene is a broad clearing with a level camera, followed
by a controlled wall-occlusion shot.

User corrected the interpretation of those screenshots: the green geometry
stays in a player-relative direction, moves incorrectly with camera rotation,
and appears over foreground objects. This is a placement/depth failure of the
0.2.1 probe, not merely a crowded-scene artifact. Version 0.2.2 switches the
render probe to the loader's world-space camera target and logs up to twelve
camera/target/centre/projection samples to the loader log. It adds a magenta
centre marker to distinguish pose from line rendering. Headless target-ray
checks and the .NET plugin build passed; live placement is pending.

The user then shared expedition screenshots and a recording of 0.2.2 showing
a magenta wire sphere near screen centre and isolated green lines. The probe
log shows camera/target motion, but every recomputed centre projects to about
(960, 540). This exposes the placement error: the draw location was recreated
in front of the camera every sample. Version 0.2.3 anchors the cube once in
world space during ordinary camera movement, re-anchors after a camera jump
over 25 m, and removes the magenta diagnostic sphere. The screenshots still
suggest that debug primitives do not provide the depth behaviour needed for
Minecraft geometry; that remains an open milestone.

## 2026-10-04 — stone A/B/C first live result

The user's 23-second expedition recording shows the v0.3 comparison labels.
The visible red cube sits under **C**, the projected image-quad path, and draws
through MHW foreground. **A** has no visible mesh and **B** shows only a few
white points. The SPL log confirms Minecraft's stone asset arrived, four
color meshes were registered for A, and the 16×16 PNG loaded for C. The stone
PNG in Minecraft's local client cache is grayscale, so the red C result is
not the intended block color. A/B are staged against dense nearby terrain;
this footage alone does not prove whether they are occluded or the primitive
draws fail. These are observations, not proof of a depth-correct render.

Version 0.3.1 changes C to draw the actual transmitted RGBA pixels directly
as ImGui quads, eliminating the suspect GPU texture-handle path. It moves the
three 70 cm blocks to a tighter row, 40 cm above the original anchor, and adds
full-length pixel-colored edges to B so its line pass is legible. C remains
depth-unaware by design; a host depth-buffer integration is still required.

## 2026-10-04 — cursor overlap confirms overlay ordering

The v0.3.1 recording shows C with the actual gray stone pixels and cube
perspective, but it still covers the hunter and terrain. A's label is visible
against open sky while its mesh is absent; B still contributes only tiny
fragments. The user's observation that C covers the cursor is additional
evidence of the foreground ImGui draw-list ordering, not evidence that the
camera transform itself is wrong.

Version 0.3.2 puts C's pixels on ImGui's background draw list, leaving labels
foreground. This should place UI windows and ImGui's software cursor above the
block, but **does not** put MHW world geometry above it. SharpPluginLoader's
public 1.0 rendering API has no scene-depth texture accessor. The reference
MHW crossover uses a Direct3D Present composite with the game's depth buffer;
porting that behavior to Proton/DXVK requires a separate native graphics path
and validation of the actual D3D version and depth resource before injection.

## 2026-10-05 — explicit placement invalidation (0.3.3)

Baseline first: MHW in-world with the stone composing under the HUD, and the
Minecraft HUD reporting `MHW LINK: LIVE | 1 pkt/s | seq 8147 | age 269ms`,
later `WAITING | seq – | age –1ms` while Minecraft was backgrounded. The bridge
performs no rate limiting and the Minecraft log carried no errors, so the rate
reads as background throttling rather than a protocol defect. Recorded as an
open observation, not changed in this entry.

`PlacementLifecycle` replaces the process-lifetime anchor. It drops the anchor
on a single-frame camera jump over 25 m, or on over a second without a usable
camera sample, and never re-anchors: after a scene change the stone stays absent
until an explicit `place`. `control-mhw-renderer.py clear` drops the anchor on
demand. Headless checks cover the jump, camera-timeout, non-finite sample, clear
and reset paths (`Placement lifecycle checks passed`); Rust, Fabric and the
existing .NET checks stayed green. Version bump to 0.3.3.

Verified live in the running game with 0.3.3 hot-reloaded without an MHW
restart: `place` → `clear` → `place` produced anchored / cleared by request /
anchored log lines with the stone present, absent and present; camera rotation
kept the anchor and re-projected the cube; and a player-driven transition to
Astera logged `Native placement invalidated: camera jumped over 25 m in a
single frame`, after which no anchor event followed for the rest of the
observation — no automatic re-anchor — until a fresh `place` anchored at new
Astera coordinates and the stone drew there.

Injected input does not reach Proton in this session: uinput injection and
Hyprland's `hl.dsp.send_key_state` both left the game unaffected, so the scene
transition had to be player-driven.

No SharpPluginLoader lifecycle callback is relied upon, so none was verified;
placement invalidation uses camera telemetry alone. Depth-candidate selection in
`renderer.cpp` is untouched.

## 2026-10-05 — depth-candidate investigation (no selection change)

Measured the existing read-back sets instead of changing the selection rule.
`depth[0]` held scene geometry in all four capture sets across two sessions
(97.4–99.97 % coverage when terrain filled the frame, 54.2 % when sky filled
46 % of it), `depth[1]` was entirely zero at capture time in every set, and
`depth[2]` was never captured. The paired depth/colour PNGs put sky at the
reversed-Z far value, match foliage and hunter silhouette positions, and show
the stone absent from depth because the renderer reads depth and never writes it.

Candidates are appended on first discovery and pruned only on resize: `depth[1]`
appeared about two minutes after `depth[0]` in this session, so list index
records discovery order rather than scene-depth priority. `renderer.cpp` is
unchanged; any future rule must be validated in more areas and should prefer
per-frame freshness over list index.

## 2026-10-05 — live camera link acceptance: outage, rate, F7

Step 8 ran itself: the bridge process died at 05:02:07 after a clean
03:06:07-05:02:07 stretch, and Minecraft logged
`Bridge I/O failed; retrying: java.net.PortUnreachableException` at 60/min
while the HUD read `WAITING | 0 pkt/s | seq – | age –1ms`. Restarting the
bridge printed `registered Mhw` and `registered Minecraft` within seconds, a
second `Connected to crafterhunter-bridge/v1` appeared in the Minecraft log,
and the HUD returned to `LIVE` with neither game restarted. MHW noticed
nothing: its socket never errored, so no second HelloAck went out and the
ten-second camera grace was not re-applied. The bridge now runs detached from
any terminal via
`setsid nohup ./target/debug/crafterhunter-bridge > native/mhw-renderer/build/evidence/camera-link-acceptance/bridge.log 2>&1 &`.

The open `1 pkt/s` observation resolves as background throttling. Across a
68 s / 30-sample OCR series with MHW rendering, the HUD held
`LIVE | 16-18 pkt/s | age 3-55 ms` on every sample with an increasing
sequence; with MHW on an inactive workspace it fell back to `1 pkt/s` and
flapped between `LIVE` and `WAITING` as age crossed 500 ms. The producer caps
at 20 Hz, so the rate is MHW's frame rate: a window Hyprland is not rendering
blocks on Present and the camera payloads stop with it. Read the HUD with both
games on the active workspace.

F7 toggles the link cleanly: the HUD reads `OFF` while packets keep arriving,
and the second press returns to `LIVE`. Minecraft runs on XWayland, so `wtype`
never reached it; Hyprland's `hl.dsp.send_shortcut{mods="", key=..., window=...}`
did. That dispatcher targeted at the MHW window, `send_key_state`, and
`ydotool key` still leave Proton unchanged, while `ydotool mousemove` does move
the system cursor, so uinput reaches the compositor and only delivery into
Proton fails. Rotation, translation, inverted axes and the F8 fresh anchor stay
player-driven; steps 5, 6 and 9 of `docs/camera-link-test.md` remain open.

## 2026-10-05 — step 9 verified, and what a measured rotation proved

Quit and rejoined the Minecraft world with injected input only. Hyprland's
`send_shortcut{window=<window>}` drives Minecraft *when that window has focus*
(silently no-ops otherwise, which cost several attempts), and `ydotool` reaches
it through uinput, so mouse moves plus `ydotool click 0xC0` walked the pause
menu and the world list. The world unloaded at 06:04:11 and reloaded at
06:10:40; a minute later the HUD read `LIVE | 16-17 pkt/s | age < 68 ms` with
neither game nor the bridge restarted. The placement half comes from
`control-mhw-renderer.py capture`: the `enabled=1` branch was taken with
`parameters.centre` still holding the anchor, and the texels at the anchor's
projected screen position are exactly `127,127,127` and `143,143,143` — the
Minecraft stone palette, which Astera's frame never produces. The runtime log
has logged no invalidation since `anchored` at 04:46:10, so the stone survived
the bridge outage, the world unload and the reload.

Reading the renderer back also produced the measuring tool the eyeball tests
were missing. `parameters.bin` is `{inverse, projection, centre, screen}`,
`centre` is the anchor and `matrices.bin` is the same viewProjection, so the
anchor's screen position is `centre * viewProjection` under System.Numerics'
row-vector convention — the column-vector reading lands on screen centre and is
wrong (it does not match where the stone actually is). One -600 px
`ydotool mousemove` walked that projection from (483,406) to (1335,822): a real
camera rotation that did *not* invalidate the placement, exactly as the
lifecycle specifies, and the stone moved with it — the box at the new
projection is 4200/4200 neutral texels of the stone palette while the old
position holds only dark scene.

Input into Proton is still the blocker for steps 5 and 6. Pointer motion
rotated MHW twice, but after any focus switch to Minecraft the identical moves
leave MHW pixel-identical (mean 0.00-0.14 over 3 s) while the game keeps
rendering (6,790 pixels change elsewhere in those 3 s) and the link stays
`LIVE | 12 pkt/s`; a click to re-grab the pointer does not help and
`ydotool key` with W held a full second never moves the hunter. F7 and F3 do
work on Minecraft when it is focused, which is how the ON/OFF comparison was
taken: `OFF` visibly changes Minecraft's world view (mean 8.45 over 3,514
pixels), so the link's pose really is being applied. Whether it follows MHW in
the same direction still needs a person — Minecraft renders a featureless dark
grass field, and MHW ignores injected input.

## 2026-10-05 — steps 5 and 6 verified by the person

The person drove MHW directly and reported that camera, movement and position
track between the two games: Minecraft follows MHW's rotation in the same
direction with no inverted axis, and its view moves relative to the starting
Minecraft viewpoint instead of jumping to MHW's absolute coordinates. F8
re-anchors from the current vanilla Minecraft camera and re-orients both the
player and the camera, so the fresh-anchor half of step 6 passes too. The two
halves the agent could only prove separately — that Minecraft applies the pose,
and that the native placement tracks rotation without invalidating — are now
covered end to end by a person seeing the link behave.

The run was performed with Minecraft parked on a menu, because Minecraft
periodically takes input away from MHW. That is a workaround around window
focus, not a link defect, and it is recorded in `docs/camera-link-test.md` so
the next run does not mistake it for one.

Two operational notes from the session. The bridge died on `EINTR` with
`Error: Os { code: 4, kind: Interrupted, message: "Interrupted system call" }`
and exited instead of retrying the socket read; a restart brought both clients
back (`registered Mhw`, and `registered Minecraft` on a fresh port) with neither
game relaunched, so the recovery path in step 8 held a second time, but the
missing retry is a real defect to fix. And the layout cost of testing is now
measured: both games must share the active workspace or MHW stops rendering and
the feed falls to `1 pkt/s`, while Minecraft must stay visible above MHW's
window or its response cannot be observed — Hyprland renders only the active
workspace, and MHW re-raises itself over the pinned Minecraft overlay at
intervals.

Milestone 1 in `MODDING_PLAN.md` is recorded with the step 9 dimension change
and fifteen-minute expedition still open. Next milestone is 2, composition: one
Minecraft block inside an MHW expedition, hidden by MHW terrain at different
camera angles, after verifying the actual DXVK color/depth resources and frame
synchronization.

## 2026-10-05 — step 9 closed, camera acceptance passes

The person ran the expedition and the dimension change. `CrafterHunter.runtime.log`
anchors the placement at 11:04:54 and holds it for twenty-seven minutes with no
invalidation, then logs `Native placement invalidated: camera jumped over 25 m
in a single frame` at 11:32:25 — the camp teleport. That is the specified
explicit invalidation for an area transition rather than a loss: the stone goes
away and nothing re-anchors it on its own, so F8 is required to start again.
The Nether change was reported flawless.

The corroborating counts matter as much as the report. `latest.log` contains no
CrafterHunter exception anywhere in the run window; the only error lines are
the bridge outage at 60 per minute from 11:00 to the 11:04:32 restart, and none
after it, with no further `PortUnreachableException` through 11:36. So the link
ran the whole expedition without a single retry, and the outage recorded
earlier in this journal was confined to before the run.

Milestone 1 in `MODDING_PLAN.md` is now a pass: all nine steps of
`docs/camera-link-test.md` are recorded, with the live-game halves driven by the
person. Next is milestone 2, composition — the occlusion acceptance scene is
the gap, since the native depth-tested renderer, the identified scene depth
resource and the placement lifecycle already exist.

## 2026-10-05 — the bridge survives an interrupted read

The outage recorded earlier today was the bridge itself. A signal landed on
`recv_from`, which returned `Interrupted`, and the receive loop only tolerated
`WouldBlock` and `TimedOut`, so `main` returned `Err` and the process printed
`Error: Os { code: 4, kind: Interrupted, message: "Interrupted system call" }`
and exited. From the games' side that is just a dead peer: Minecraft retried at
60/min until the restart, MHW kept sending into the void.

`is_transient` now covers `Interrupted` alongside the timeout cases, so the
receive loop simply tries again. Every other error still returns, because a
bridge that fails loudly beats one that goes quiet during a live session. The
send path carried the same latent fault — `send_packet` and `send_bytes` used
`?`, so an interrupted datagram send would have killed the process in exactly
the same way. Sends now go through `send_with_retries`, extracted as a
closure-taking function so the retry is testable without a socket: it retries
an `Interrupted` send until it lands and returns any other failure at once.

Five tests cover the decision: interrupted, would-block and timed-out reads are
retried; broken-pipe and permission-denied are not; an interrupted send is
retried to completion; a real send failure returns on the first attempt; a
truncated datagram is still an error. `cargo test --workspace` reports twelve
passing, `cargo clippy --workspace --all-targets` is clean, and
`tools/test-fabric-camera.sh` passes. The bridge was rebuilt and restarted so
the running process carries the fix.

This was step 0 of the composition prep. Next: depth-resource selection by
this-frame freshness instead of discovery order, plus the frame-synchronization
trace, before the cutscene case is attempted.

## 2026-10-05 — freshness alone would have broken occlusion

Reading the two captures left in `render/` from today (`13166284` at 06:24 and
`14757842` at 06:51) rather than launching anything turned up the design
constraint for milestone 2's depth rule. Both hold three candidates now, not
the two the earlier table saw. In `14757842` candidates 0 and 1 were **both**
fresh (`age=1`), and `inspect-depth-capture.py` reports candidate 1 at `0.00 %`
covered — cleared every frame and never rendered into.

That matters because the naive rule the plan was heading towards, "pick whichever
depth was cleared this frame", would have selected an empty buffer. Under
reversed Z an empty depth is all zeros, `sceneDepth` then reads 0, and
`depth + epsilon < 0` is never true, so nothing is ever discarded and the stone
draws through every object in front of it — the exact failure the depth path
exists to prevent, reached by a "safer" rule rather than by the old one.
Candidate 2 shows the mirror image: `age=87757` and `age=134603` with
byte-identical percentiles across two different views, so it is stale and
frozen.

Selection needs two conditions — fresh this frame **and** known to contain
scene content — with content coming from an occasional read-back rather than
every frame, since a per-frame read-back stalls the GPU. Until that exists
`depths[0]` plus the staleness check stays, because it is the only candidate
verified to hold geometry. Recorded in `docs/depth-renderer.md` next to the
table it extends.

## 2026-10-05 — recorded cutscene, teleport, and second-area evidence

Two recordings from the person close the observable half of milestone 2, and
one of them changes what was claimable about cutscenes.

`recording_2026-10-05_12.21.00.mp4` (146.75 s) is a scripted sequence: a white
transition at 42 s, a handler cutscene at 54 s, a monster cutscene at 90 s. The
HUD reads `LIVE | 16–17 pkt/s` at `age 5–29 ms` with `seq 978 → 2458` over 84 s
— no gap, no `WAITING`, Minecraft's view following the cutscene camera
throughout. That is the camera half of the cutscene case, measured rather than
argued.

It is also the recording that shows why the *composition* half was never
observed: `CrafterHunter.runtime.log` has placement anchored at 12:19:01 and
invalidated seven seconds later by a >25 m jump, with no re-anchor before or
during the recording. With `enabled=0` the renderer does not compose — the
session's `renderer.log` has no `Composing before MHW UI` line after its init —
and indeed no cube appears in any of the twelve frames sampled. So that video
is evidence for the link and explicitly not evidence for composition during a
cutscene. Recorded as such instead of being counted as a pass.

`recording_2026-10-05_12.37.51.mp4` (13.16 s) supplies the other half in
normal play: placement anchored at 12:37:34, never invalidated during the
recording, and the stone visible as a world-anchored cube with two faces over
the camp while the HUD holds `16–17 pkt/s`, `age 1–25 ms`, `seq 12245 → 12436`.
Three frames saved to the gitignored evidence run directory. Separately, the
log shows three single-frame >25 m invalidations that day (12:19:08, 12:33:05,
12:37:23), each followed by a manual `place` and never by an automatic
re-anchor.

The capture taken at 12:35 in a second area is the "capture another area" step
the depth table asked for, and it reproduces the morning's hazard exactly:
`depth0` 99.75 % covered, `depth1` `age=1` and `0.00 %`, `depth2` `age=24924`
and `0.00 %`. Fresh-but-empty is a property of the resource, not of one area,
and `depth2` having no content here at all (4.47 % in the morning) means
"known content" must be re-checked periodically rather than fixed at discovery.

Milestone 2 is recorded as passing on these checks. What stays open is honest
and small: the cube was never seen *on screen during* a cutscene, the
depth-selection rule still needs its fresh-and-content design, and
frame-synchronization evidence has not been traced. Composition reads MHW's own
GPU camera constants each frame, so camera ownership is not the risk; the
depth candidate is.

## 2026-10-05 — the cube during a cutscene, finally observed

The gap the previous entry left open is closed by
`recording_2026-10-05_12.48.56.mp4`: 101.31 s of dialogue cutscene, subtitles
at 10 s, 66 s and 86 s, with the stone live the whole time. Placement anchored
at 12:48:52 and the only invalidation is a >25 m jump at 12:50:51 — fourteen
seconds *after* the recording stopped — so `enabled=1` for every frame of it.

Four sampled frames show the cube world-anchored inside the cutscene: behind a
close-up character at 10 s, two faces behind the Handler at 34 s with her
drawn in front of them, behind the white-fade shot at 50 s, and among three
talking characters at 86 s with a foreground character occluding it. At 66 s it
is simply out of frame, which is what a world-anchored block should do when the
cutscene camera looks elsewhere.

The occlusion is the part worth writing down. The cutscene's own characters
draw *in front of* the cube, so the depth resource bound during the cutscene
held the cutscene's geometry — the fresh-but-empty candidate that the morning's
captures made the leading selection hazard did not fire here. Selection still
needs its rule; this says the risk is real but not constant.

Link quality through the sequence: `seq 14706 → 15217 → 15557` measures 16.0
and 17.0 packets/s over the two intervals, agreeing with the `17–18 pkt/s` on
screen, `age` between 7 and 57 ms, `LIVE` in every frame that has a HUD at all.
A 101 s scripted sequence with no stall.

Eight `Native placement anchored` lines span the recording 4–21 s apart.
`render/place.request` has exactly one writer, `tools/control-mhw-renderer.py
place`, and no request file was left behind, so each anchor came through that
sanctioned path rather than from anything re-anchoring on its own.

Milestone 2 is now recorded as passing **with the cutscene case included**;
`MODDING_PLAN.md` and `docs/depth-renderer.md` updated. What remains on this
milestone is design work rather than observation: the depth-selection rule
(fresh *and* known content, re-checked periodically — `depth2` held content in
one area and none in another) and the frame-synchronization trace. Next phase:
terrain and player queries for milestone 3.

## 2026-10-05 — depth selection implemented, frame-sync trace added

The two follow-ups that were recorded as design work are now code.

**Selection** moved into `native/mhw-renderer/selection.hpp`, which carries no
D3D types on purpose so it can be tested with the host compiler. A candidate
binds only when it matches the backbuffer, was cleared to 0 (reversed Z), is at
most one frame old, **and** a read-back has shown it holds geometry. That last
condition is the one the morning's captures demanded: `14757842` and `2089775`
both held a candidate at `age=1` reading `0.00 %` covered, so freshness alone
would have bound an all-zero buffer and drawn the stone through everything.
An unmeasured candidate is not bindable at all, so on startup composition waits
for the first read-back instead of defaulting to index 0.

Content measurement is deliberately cheap and non-blocking: one copy in flight,
`Map` with `D3D11_MAP_FLAG_DO_NOT_WAIT` retried on later frames, a 90-frame
timeout, only for candidates that are fresh right now, and only on first sight
then every 300 frames. The re-check interval matters because content is not a
property of a resource for ever — `depth2` held 4.47 % in one area and 0.00 %
in another. Threshold is 2 % of a whole-frame sample grid, chosen to err
strict: too strict hides the stone, too lenient draws through.

`bash tools/test-depth-selection.sh` compiles the header with `-Werror` and
runs 13 checks — freshness boundaries, the reversed-Z clear, target mismatch,
the fresh-but-empty case, the threshold from both sides, discovery-order
fall-through, corner content still counting as content, and every failure mode
resolving to "skip this frame".

**Frame sync** is a bounded diagnostic rather than a permanent cost:
`python tools/control-mhw-renderer.py framesync` arms 60 composed frames, and
each one logs the bound depth with its coverage and age, an FNV-1a hash of the
GPU host view-projection of the very draw being composed (so a frozen camera
buffer shows up as `same` across frames that should differ), and the block
centre projected twice — once with those GPU constants and once with the
CPU-side view-projection handed in for the same frame. The pixel difference
between the two is the CPU/GPU sync error. `tools/inspect-framesync.py` judges
a trace and exits non-zero if a frame bound no depth, a bound depth was older
than one frame, the camera could not be read, or the projections disagree by
more than `--max-delta` (default 2 px); its failure paths were checked against
synthetic good and bad traces.

Renderer rebuilds clean under mingw with `-Wall -Wextra`. Gates run: 12 Rust
tests, the fabric camera checks, and the new depth-selection tests, all green.
Still outstanding is the live half — deploy the DLL, run the framesync trace
against the running games, and judge it.

## 2026-10-05 — depth selection and frame sync validated against the live game

The rebuilt DLL was deployed through the development reload path — previous
build backed up to `~/.local/share/crafterhunter-backups/renderer-20261005T131415/`,
pinned executable hash re-checked first — and the plugin picked it up at
13:14:16, one second after the reload request.

The new selection rule then showed its two behaviours in the running game,
which is what the whole change existed for.

**It fails closed.** The first composed frame logged
`no eligible depth candidate (1 observed; unmeasured, stale or empty) -
composition skipped`, then at 13:16:17, two seconds later, `depth[0] content
56.94% holds geometry (age=0)` and `depth select=0`. Nothing binds until a
read-back has proven the buffer holds geometry — no defaulting to index 0, and
no draw-through while the measurement is pending.

**It refuses a fresh empty buffer.** At 13:17:58 a second candidate appeared
(`depth[1] 1920x1080 format=39 bind=72 clear=0.000`) and was measured
`content 0.00% empty (age=0)`. That is the hazard from this morning's captures
— fresh, correctly cleared, holding nothing — occurring on its own in live
play, and the rule declined it. It never appears in a selection line.

The frame-sync trace was armed repeatedly across a minute of camera movement.
`python tools/inspect-framesync.py` reports PASS (exit 0) over 958 frames:

- every frame bound a depth, `depth=0` throughout, `age=0` throughout, no
  `depth=-1` anywhere;
- 37 distinct GPU camera hashes with 36 flagged `changed`, and the projected
  anchor moving from `(960.0, 540.0)` to `(1060.5, 504.4)` across 35 distinct
  positions — so the camera was demonstrably live rather than stuck;
- `delta` 0.000 px on all 958 frames including the moving ones: the CPU-side
  view-projection handed to `CH_Frame` for a frame is bit-identical to the GPU
  host matrix of the draw composed for that frame.

Rate context worth keeping: 28.9 fps averaged across the run, 37.7 fps with
MHW's window focused, and roughly 1 fps while MHW sat on an inactive
workspace. That last number is why the games must share the active workspace —
it is the same rule the packet-rate observation recorded in milestone 1, now
measured on the renderer side.

Evidence archived as `20261005T1321-framesync-trace.log` (958 frames,
gitignored). Both milestone 2 follow-ups are closed; `docs/depth-renderer.md`
carries the full table and `MODDING_PLAN.md` the updated status. The trace is
bounded — once disarmed, `frameSyncRemaining` hits zero and no further GPU
read-backs happen.

## 2026-10-05 — milestone 3 reconnaissance: what the host actually offers

Started milestone 3 by finding out what the pinned build will and will not do,
before writing any of the query path. Findings are recorded in
`docs/host-queries.md`.

**The player side needs no reverse engineering.** Reflecting the installed
`SharpPluginLoader.Core.dll` (2.0.0) in a scratch process — metadata only,
nothing executed inside the game — confirmed `IPlugin.OnUpdate(float)` as a
game-thread tick, `Entities.Player.MainPlayer` carrying `Model.Position`,
`Rotation`, `Forward`, and `CollisionPosition`, `SingletonManager.GetSingleton`,
and `Memory.PatternScanner.FindFirst(pattern, cache)` with `NativeFunction<..>`
for typed calls. Camera sampling already runs on that same tick.

**There is no loader API for a terrain raycast.** SPL's
`SharpPluginLoader.Core.Collision` namespace turned out to be attack and
hit-detection data (`AttackParam`, `CollGeomResource`, `HitZoneResource`), and
`MtGeometry` describes shapes attached to a model. Stage terrain has to be
queried through the game's own collision routine.

**The executable's code section is encrypted on disk.** `.text` is 48,304,640
bytes at entropy 8.00/8.00 with zero `48 89 5C 24` prologues — about eleven
would appear by chance in random data of that size — and `40 53`, `E8`, and `C3`
counts exactly at their chance expectation. The section has no instruction
structure at all.

This is worth recording honestly because it cost a wrong turn: the first tool I
wrote, `tools/inspect-mhw-signatures.py`, scanned the file for the seven
terrain-ray signatures and reported all seven MISSING in 0.8 seconds. I assumed
my matcher was broken and went to debug it. The matcher was fine; the bytes
really are not code. Offline signature verification is impossible on this build,
so the tool was replaced by `tools/verify-host-build.py`, which verifies what
can be verified and exits non-zero when any of it fails:

- the installed executable is the SHA-256 pinned in `MODDING_PLAN.md`;
- `.text` is opaque, which is the reason the scan happens in the loaded image;
- SharpPluginLoader's runtime address cache resolves `Player:FindMasterPlayer`
  to `0x141B42010`, identical to the address the MIT-licensed
  `justbustin/minecraft-crossover-bridge` published for build 421810, inside
  `.text` — the runtime layout of this executable matches the build those
  terrain signatures were written against;
- SharpPluginLoader's pattern cache already holds two plugin signatures it
  resolved in the loaded image on this build, both inside `.text`, so the scan
  path the adapter depends on demonstrably works here.

The adapter rules that follow are stricter than "call the same function":
signatures in one adapter type as byte patterns, fingerprint before resolving,
any miss disables terrain queries for the session, game-thread execution only,
a fixed query budget per tick with dropped-and-reported overflow, and "no
terrain" published instead of a stale answer while the singleton, player, or
stage is missing. The player proxy maps by relative displacement for the same
reason the camera link does: MHW world coordinates sit hundreds of metres from
Minecraft's origin.

No game file was modified and nothing was executed inside the game process.

## 2026-10-05 — milestone 3: the host player proxy, end to end

The player half of milestone 3 now runs from the game thread in MHW to the
player in Minecraft.

**Protocol.** `PlayerState` (kind 11) is seven `f32`: world position in metres
and the model rotation quaternion, mirroring `CameraState`. The Rust crate
encodes and decodes it and pins the layout with golden bytes, because C# writes
it and Java reads it and neither compiler checks the other's work.

**Plugin.** `SamplePlayer` runs inside `OnUpdate`, the same game-thread tick as
the camera, at the same 20 Hz. No player is a normal state rather than an
error: a loading screen or an area transition produces no sample, therefore no
packet, and the guest ages the stream out instead of holding a position that no
longer exists. Only a managed API failure stops player sampling, and it stops
the player alone - camera telemetry keeps running. The bridge needed no change,
since it already forwards every game-to-game packet.

**Guest.** `PlayerMixin` runs at the tail of `LocalPlayer.tick()`, so vanilla
finishes its own movement for the frame first; the proxy then places the player,
sets facing, zeroes velocity, and clears accumulated fall distance so physics
cannot apply a second move for one the host already made. F9 toggles the link.
The release path is the same code with no work to do: stale feed, disabled
toggle, or world change makes `PlayerLink.update` return null and the mixin
touches nothing, so movement returns to the keyboard on that tick.

Two decisions are worth stating rather than leaving implicit.

*Relative displacement, not absolute coordinates.* MHW's world sits hundreds of
metres from anything in a Minecraft world, so the first fresh sample anchors
both the host position and the player's current position, and later samples
apply their difference: one metre of hunter travel becomes one block.

*The 25 m single-frame jump rule, reused.* The placement anchor already treats a
single frame that moves more than 25 m as a scene change rather than motion. The
proxy does the same: such a jump re-anchors it where the player stands instead
of sweeping the player across the world to catch up.

**Refactors the change forced, both with their suites green afterwards.** The
camera's quaternion-to-yaw conversion moved to `Rotation`, shared with the
player, because two copies of that math would drift the first time one is
corrected. The freshness and arrival rules moved to `SampleFeed`, so camera and
player age samples by exactly one rule while remaining independent streams -
the player check asserts that a stale player sample stays stale while the camera
feed goes live, and vice versa.

**Checks.** Rust: 10 protocol tests including golden bytes and rejection of
wrong length, non-finite values, and an unassigned kind. Java: a new
`PlayerLinkTest` covering anchor, movement, yaw wrapping across +/-180, the jump
re-anchor, every release path, world change, payload decoding, and feed
independence; the camera suite still passes after both refactors. The two
headless scripts now read one shared source list so they cannot drift. The
mixins compile against the mapped 26.2 jar through `./gradlew --offline
compileClientJava`, which is the gate that proves `PlayerMixin` and its
injection target are real.

**Not done.** The terrain half of milestone 3 has not started. The proxy has not
yet been accepted in the running games: the yaw convention in particular is
derived from the camera's proven conversion and has to be watched against the
hunter's actual facing, because a mirrored proxy would still look plausible in
motion. My first run of the new suite failed on an expectation of mine, not the
code: identity wraps to -180, not +180.

## 2026-10-05 — player proxy accepted, and the bridge that was eating it

Minecraft was restarted with the rebuilt jar (40,805 bytes, previous jar saved
to `~/.local/share/crafterhunter-backups/minecraft-mod-20261005T1412/`), the
plugin hot-reloaded at 14:11:51, and both guarded reads armed at 14:12:03. The
guest connected on launch with no discarded-packet errors, so the deployment
looked complete.

**It was not, and neither end could tell.** The bridge binary had been started
at 12:20, before `Kind::PlayerState` existed, and the protocol rejects an
unassigned kind, so every player packet died in the bridge:

```
discarded invalid packet from 127.0.0.1:39442: UnknownKind(11)
```

5,138 times, at 20 Hz, while the plugin logged `First guarded player read
succeeded` and Minecraft logged nothing at all. The camera kept working — its
kind was already known — so the proxy simply never arrived and the HUD stayed
on `WAITING`. The only evidence was in the bridge's own log.

`cargo build --offline --workspace` recompiled the bridge and probe, and a
restart on the same log file re-registered both endpoints (`registered Mhw`,
`registered Minecraft`) with zero discards afterwards; the guest reconnected at
14:36:01 after one `PortUnreachableException` retry. This is the operational
half of "the bridge needs no change": the source is forward-compatible with new
kinds, a running binary is not. The line to add next time a kind is introduced
is in `docs/host-queries.md`, next to the status claim where it will be read.

**Verdict from the person:** camera and movement between the two characters are
precise and accurate, the same judgement that closed milestone 2, and they
asked to proceed to the next phase.

**What is still open, deliberately recorded rather than folded into the pass.**
The one reading of the proxy as *reversed* was taken with the camera link (F7)
off. That matters because F7 is what normally overwrites the view from the MHW
camera every frame: with it on, the model quaternion never reaches the screen in
first person, and with it off the vanilla camera follows `LocalPlayer.yRot`, so
the view *is* the model-quaternion path. The person attributed the reading to
the camera link being off rather than to the conversion, but the clean test —
F7 off, F9 on, turn the hunter 90 degrees and confirm the character turns the
same way — has not been run, nor has F9 mid-motion. Both stay in the
verification table in `docs/host-queries.md`.

**Why the agent could not take the screenshot itself.** MHW runs floating on
the same workspace, above Minecraft and re-grabbing focus, so `grim` captures
and an injected F2 both landed on MHW. Hyprland here dispatches through Lua:
`hyprctl dispatch 'hl.dsp.focus{workspace=hl.get_workspace(3)}'` switches
workspace, `hl.get_windows()` returns window objects, and `hyprctl eval` can
write files with `io.open`, which is how the window geometry was read. Raising
the game or lowering MHW is the person's layout, so the HUD reading stays with
them until they say otherwise.

## 2026-10-05 — terrain stage A: the game's own segment cast, behind a fingerprint

Milestone 3's terrain half was a design and nothing else. It now has an
adapter, `native/mhw-spl-plugin/TerrainAdapter.cs`, that resolves the game's
segment cast in the loaded image and proves it on the host.

**Getting the routine's contract back.** The seven terrain signatures were
known to exist only as bytecode: `tools/inspect-mhw-signatures.py` was never
committed, and its `__pycache__` entry was the only surviving copy. Marshalling
that `.pyc` back and disassembling the module gave the seven name-to-pattern
pairs and their published addresses. All seven match the MIT prior art's
constants exactly (`sCollision::CheckSegment` at `0x14231AC00`, the `Param`
block's three routines, `TriangleInfo`'s three), which is what makes the
pairing trustworthy: an off-by-one pair would have disagreed. The prior art's
`raycast()` supplied the rest — `Param::ctor(param, 0x7FFFFFFF, 0x3FFFFFFF, 0,
0, 0xA, 0, 1, 1, 0, 1)`, `param[0xF9] = 0`, two `0x140` caller-owned buffers
zeroed and 16-byte aligned, eight segment floats `start.xyz 0 end.xyz 0`, flag
`1`, hit position at `tri+0xC0`, normal at `tri+0xB0`, attribute through
`TriangleInfo::attr(tri, 0)`, and `reset`/`dtor` before the buffers go out of
scope. `docs/terrain-query.md` records it in full.

**The calling convention, answered from the running process.** SPL's
`NativeAction`/`NativeFunction` call through `delegate* unmanaged` handles, so
the first question was whether a managed call can reach game code at all under
Proton: the game's PE uses the Windows x64 convention, while a Linux CLR would
emit the System V one. `/proc/4959/maps` answers it — the game's process maps
`coreclr.dll` out of
`steamapps/compatdata/582010/pfx/drive_c/Program Files/dotnet/shared/Microsoft.NETCore.App/8.0.12`.
The runtime inside the prefix is the Windows build, so its function-pointer
calls already match the game, and the loader's own types are the right door.
Calling one is an unsafe call, so `AllowUnsafeBlocks` is now on in the plugin
project and the unsafe surface is three methods: `TerrainAdapter.Tick`,
`TerrainAdapter.CastDownRay`, and `Plugin.OnUpdate`, which exists only to reach
the ray. `Pattern.FromString` was round-tripped in the scratch reflection tool
against both the loader's 2.0.0 and the 1.0.0 package the plugin compiles
against, confirming `??` wildcards parse and garbage throws.

**The adapter.** `OnLoad` hashes `MonsterHunterWorld.exe` from the process
working directory (the game root, confirmed by `readlink /proc/4959/cwd`),
compares it with the pinned SHA-256, resolves all seven patterns with
`PatternScanner.FindFirst(pattern, cache: true)`, and allocates the aligned
buffers. The first failure latches `Disabled` with its reason and no tick can
move it back. `OnUpdate` ticks the adapter *before* the bridge gates anything,
because a terrain answer is a host fact and should not wait for a guest that is
still connecting; it publishes the state before it casts, so a loading screen
reads *unavailable* rather than the previous ray's answer. One ray per tick,
one-second burst of twelve self-checks then a five-minute heartbeat, each line
carrying the ray height, `CollisionPosition`, the model origin, the normal, and
the attribute. `CollisionPosition`'s meaning is not assumed: if it is the feet
rather than the capsule centre, the delta shows it instead of hiding it. Every
native call is wrapped, so a failure disables terrain for the session instead
of escaping into the game process.

**A gate that was missing.** The plugin's headless tests
(`native/mhw-spl-plugin/tests`) existed but nothing in the gate ran them.
`tools/test-spl-plugin.sh` does now, and the terrain policy went into them:
every signature parsed through the loader's own `Pattern.FromString`, the
down-segment geometry, the agreement tolerance including its boundary and its
refusal to judge non-finite numbers, and all seven branches of the state
machine. A typo in a byte pattern fails there instead of quietly disabling
terrain in the next session.

**Not done, and not hot-reloaded yet.** The protocol half (a request kind, a
result kind, and a bounded queue for requests arriving from the guest), the
guest half (mapping Minecraft coordinates through the proxy anchor), and the
live run are all still ahead. The adapter has deliberately not been loaded into
the running game: a native call into the game's collision routine is the first
thing in this project that can take MHW down if the recovered ABI is wrong, and
it should go in on purpose, with someone at the keyboard who can stop it.

## 2026-10-05 — the terrain self-check became a request, not a heartbeat

The adapter committed in `f572a79` cast a down-ray on a one-second burst as soon
as the world was ready. Reviewing it before deploying: that puts the first
native call into the game's collision routine about a second after a world
loads — during a cutscene, a quest transition, or with nobody watching — and it
would have fired unattended in a session that had not asked for terrain at all.
It was never hot-reloaded, so no game was ever exposed to it.

The rule is now explicit. `TerrainAdapter.Tick` publishes the state every
sample and casts **at most one ray, only when a request is pending**, and the
request is a file next to the plugin's log:
`nativePC/plugins/CSharp/CrafterHunter/terrain/check.request`, written by
`tools/control-mhw-terrain.py check` and removed by `clear`. The file is deleted
before the ray runs, so a request fires exactly once; a request that arrives
while the singleton or the hunter is missing is refused in the log with the
state that refused it, rather than silently discarded. This is the same
request-file pattern the native renderer already uses for `place` and `clear`,
and it has the property the acceptance run actually needs: the cast happens
when a person is standing on the ground they want measured, which is also the
only way to measure a slope and compare it with flat ground.

`TerrainRequest` holds the channel and the test project covers it: an absent
request produces nothing, a pending request is taken exactly once, a cleared
request never fires. The per-tick budget is unchanged in spirit and tighter in
fact — one ray per one-second sample, requested — and stage B's guest queue will
spend that same budget rather than a second, larger one.

## 2026-10-05 — the terrain ray answered, live, on demand

Plugin 0.3.4 was installed into the running game at 15:51 and the first native
call into MHW's own collision routine went through without taking the game
down. Everything below came from four requested casts and the log they wrote:

```
15:51:18.231  Terrain adapter ready: all 7 signatures resolved against the pinned executable.
15:51:18.783  Terrain state changed to Ready.
15:51:59.803  Terrain self-check 1: hit=1 agree=True rayY=-3.821m collisionY=-3.471m
              delta=0.350m positionY=-3.821m normal=(-0.33, 0.89, -0.32) attr=1048576
15:52:36.803  Terrain self-check 2: (identical)
15:52:42.805  Terrain self-check 3: (identical)
15:52:48.806  Terrain self-check 4: (identical)
```

**Resolution works as designed.** All seven patterns resolved in the loaded
image in about a second, alongside the executable hash, with no address ever
written to the log. `SingletonManager.GetSingleton("sMhCollision")` returned a
live pointer on the first sample, so the state machine went straight to `Ready`
— which matters more than it looks: had the singleton name been wrong, the
adapter would have stayed `Unavailable` and never cast anything at all.

**The ray hits what the hunter stands on.** `rayY` and `positionY` match to the
millimetre, so the segment lands exactly on the surface under the hunter's feet
and the model's origin is that surface. This is the question milestone 3 asked,
answered on the host with no protocol and no Minecraft restart.

**`CollisionPosition` is not ground height.** It sits 0.35 m above the hit —
consistent with a collision reference point above the contact rather than the
contact itself, and exactly the kind of assumption that would have quietly
skewed the guest's ground height by a third of a metre in stage C. The adapter
logged ray, collision point, and model origin as three separate numbers for
this reason; had it logged only an `agree` verdict, the offset would have looked
like a passing test.

**The sample is indoors.** The screenshot put the hunter inside the Astera quest
house, standing on slanted wooden architecture — the normal is a unit vector
about 27 degrees off vertical, which is a plank, not terrain. Stage geometry,
platforms, and terrain share one collision system, so the cast is valid
evidence about the routine and not yet about terrain. Two outdoor samples, one
flat and one on a slope, are still owed.

**A note on how it went in.** Nothing cast on its own: the four rays came from
`tools/control-mhw-terrain.py check`, one request at a time, and the game
survived all of them with the bridge and both endpoints still registered. The
periodic version that was committed first would have fired a native call the
moment the world became ready, which is why it was replaced before it was ever
loaded.

## 2026-10-05 — terrain on the wire: kinds 12 and 13

The protocol half of milestone 3's terrain stage is now defined, and both
payloads have golden bytes pinned by tests, because the plugin will decode one
in C# and the guest will write the other in Java and neither side is written
yet — this is the moment to fix the layout rather than after.

`TerrainRequest` (12, guest to host) is a `u32 id` and six `f32`: `start.x y z`
then `end.x y z`, 28 bytes. `TerrainResult` (13, host to guest) is a `u32 id`,
a `u8 status`, three reserved zero bytes, six `f32` for position and normal, and
the game's `u32` surface attribute: 36 bytes. Endpoints are metres in host
coordinates, because the guest is the only side that knows where it anchored —
the host cannot answer a question asked in Minecraft coordinates. A guest that
maps through its own proxy anchor keeps the question and the answer in one
space, the same reason the camera and player payloads carry raw host
coordinates instead of pre-mapped values.

Three statuses, and the difference between them is the whole point:
`0 no terrain`, `1 miss`, `2 hit`. *No terrain* has to be an answer, not an
absence, or the guest's freshness timeout would be the only way to learn about a
loading screen. *Miss* is a real answer and must be distinguishable from having
no answer, because a guest that conflates them holds its last hit straight
through a wall; it therefore carries a zeroed position so it also cannot be read
as a hit at the origin. Decoding refuses a wrong length, a non-finite float, an
unknown status, and non-zero reserved bytes, matching the header's own reserved
bits.

Two of the new tests caught my own mistakes rather than the code's: `-4.0` is
`0xC0800000`, not `0xC0000000`, and a non-finite `attribute` is just a large
`u32` — the attribute is not a float and cannot be probed that way. Both were
caught because the layout was pinned as bytes rather than described in prose.

Still nothing sends either packet: the host half (accept requests from the
guest into a bounded queue, cast within the per-tick budget, answer) and the
guest half (pace requests, map coordinates, feed collision) are next, and the
bridge has to be rebuilt and restarted before either can work — the lesson from
`UnknownKind(11)`.

## 2026-10-05 — the host answers terrain requests

The host half of stage B is in. `TerrainRequest` arrives on the endpoint thread,
where nothing native may happen, and is queued for the game thread: eight deep,
a ninth dropped and counted with a report every 64, so a guest that outruns the
host loses answers rather than making the game wait. The tick samples at twenty
hertz and casts at most two rays per sample, which drains a full queue in about
two frames of budget instead of stalling one. A request that lands while the
state is not `Ready` is answered `no terrain` straight away — the whole reason
that status exists, rather than letting the guest time out holding an old hit.
Answers queue separately at thirty-two, where the slow side is the endpoint
rather than the guest, and drop the newest.

`TerrainPacket` is the wire layout in C#, and the plugin's tests pin the same
bytes the Rust crate pins: the same constants read in one language and written in
the other, plus the refusal of a short request, a request with a NaN in it (never
handed to the game), and a miss that must carry its id and status and nothing
else.

`TerrainRay.DownSegment` and `Agrees` moved to metres, because the adapter now
takes and returns metres everywhere and only the cast itself works in MHW units.
That is the same class of mistake as reading `CollisionPosition` as ground
height: a scale factor that leaks into a boundary is a bug waiting for a
multiplier nobody expected.

**Why the wire path is not proven yet.** The obvious test is the probe, which
already speaks the protocol. It cannot be used: the bridge keeps one endpoint
per source and routes only Mhw↔Minecraft, so a probe would have to register as
`Minecraft` and take the guest's slot, and the real mod only registers on its
Hello at startup — the camera and player links would stay dead until Minecraft
was restarted. Probe-sourced packets are not routed at all, so it would not see
its own answers either. Proving the wire path with the real guest is stage C, and
the bridge has to be rebuilt and restarted first or kinds 12 and 13 will vanish
into `UnknownKind`.

## 2026-10-05 — three surfaces, and a normal that is not a normal

Two more requested casts, flat dirt in Astera's base camp and a dirt path in the
Wildspire Wetlands, completed the ground-truth comparison across three surface
classes. The hit height agreed with the hunter's own height every time — to the
millimetre indoors, within 6 mm outdoors — and `CollisionPosition` came back
0.350 m, 0.344 m and 0.350 m above the hit. Three surfaces, three offsets inside
6 mm of each other: that is a property of `CollisionPosition`, not of the ground,
and stage C has to use the ray's own hit for ground height.

**The finding worth the trip.** On the Wetlands path the hit normal came back
`(-0.00, 0.01, 0.00)` — magnitude 0.010 — while the two earlier surfaces
returned unit vectors, and its surface attribute was `0x00104000` against `0` for
dirt and `0x00100000` for the wooden platform. Three samples, same value, so it
is not noise from one cast: either that surface class leaves the triangle-info
normal unfilled, or the hit resolved against a collision volume instead of a
triangle. Nothing available from outside the process can tell those apart, and
it does not matter, because the consequence for the guest is the same either
way — **a normal cannot be assumed to be unit length.** Normalizing a vector
that short amplifies rounding noise into an arbitrary direction, and a slope
feature built on that would walk the player off a surface that is actually flat.
Stage C's rule is now written down: below a threshold, treat the normal as
absent and treat the surface as flat.

The self-check line prints `|n|` because of that, and this is the reason to
care about a log line's shape. `(0.00, 0.01, 0.00)` reads as a direction — "the
ground faces almost straight up" — to anyone reading it quickly, including me,
which is why it took a second look to notice that the vector has no length. The
hit count came back on the line too for the same reason: one hit means the game
found a surface, and without it a zeroed normal is ambiguous between "no hit" and
"hit with nothing to report".

## 2026-10-05 — the normal is a value, not a direction, so slope comes from heights

The person asked whether a cave in the Wetlands was good enough as a slope
sample. The right answer was not to look at the screenshot, so the self-check
stopped being a single ray.

**Why.** A fifth surface, a cave floor with attribute `5`, reported the
*identical* normal `(0.00, 0.01, 0.00)` that the Wetlands path had reported
minutes earlier — same value, different place, different attribute. An unfilled
buffer reads as zero; three surfaces do not agree to two decimals on a sentinel
that is not zero. Whatever the game is doing there, it is reporting a value, and
that value is not a direction. With two of five surfaces carrying a real normal
and three carrying this, any slope feature built on normals would work on flat
ground and fail on a hillside, which is the worst possible failure: correct until
the player walks uphill.

**So slope is measured, not read.** A check is now a sweep of three columns —
the hunter's own, and one metre either side along their facing — and the report
gives the rise per metre between the outer two. Three rays at two per sample, so
a sweep spans two ticks and stays inside the per-tick budget in both. The centre
column keeps its comparison against `CollisionPosition`, which has now come back
0.350, 0.344, 0.350, 0.236 m on four surfaces: not a constant, and not a
ground height either way.

On that cave floor, three times in a row: `right[1m=36.533m] left[-1m=36.551m]`,
so `slope=-0.01 rise/m`. Flat — on a floor with a visibly inclined wall metres
away. The measurement settled a question the screenshot could not, and it will
settle the next one the same way: the person will know they are on a slope
because the number moved off zero, not because either of us judged a picture.

`SlopePerMetre` returns NaN rather than a number when a column found no surface
or the spacing is zero. A slope averaged across a missing sample is an invented
number, and an invented number in a collision path is how a player ends up
standing on nothing.

## 2026-10-05 — a 22-degree hillside, and the ground-truth row closes

The person found the steepest slope they knew in the map and stood on it. Three
sweeps, identical:

```
centre[0m=27.351m] right[1m=27.722m] left[-1m=26.923m] | slope=0.40 rise/m
```

0.40 rise per metre is about 22 degrees, 0.8 m of height across the two-metre
span. All three columns carry the same attribute, so it is one smooth incline
and not steps wearing the same number — and all three report `|n| = 0.010`,
which is the whole reason the sweep exists. The centre column agreed with
`CollisionPosition` to 0.350 m, the same offset it shows on flat ground, so the
hunter's feet are on the surface the ray finds even at 22 degrees.

Milestone 3's ground-truth question is answered: the routine is callable, it
lands on the ground the hunter stands on, it does so at 22 degrees as cleanly as
on the flat, and the two things it needed to tell us — that `CollisionPosition`
is not ground height, and that the normal is not a direction on most surfaces —
were both found by measuring rather than by reading the documentation.

Five surfaces, three of them reporting the same non-normal, one measurement
method that does not need it. The next stage is the guest half: a heightmap of
column heights around the player, mapped into Minecraft coordinates through the
proxy anchor, with the bridge rebuilt and restarted first so kinds 12 and 13
survive it.

## 2026-10-05 — the plan was aimed at the wrong direction, and it was my fault

The person asked when Steve would be walking around in Monster Hunter. I
answered with milestone arithmetic, said no milestone delivered it, and then said
the honest thing: the plan's own line 41 had already said *"a Minecraft-controlled
playable mode is a later feature and must not have both games drive the player."*
I had read that line several times while building three milestones against it and
never once asked whether "later" was acceptable.

The goal was always **Minecraft playable inside MHW**: you are Steve, MHW's world
is the world around you, an iron sword damages a real monster, a monster's claw
damages Steve, a shield refuses it, TNT works. The plan instead built the inverse
— a player proxy mirroring the hunter, monsters proxied *into* Minecraft — and
milestone 4 as written would have moved an MHW monster into Minecraft, which is
the opposite direction to the one thing worth wanting.

**What it cost:** plan text, not code. The expensive 90% was already aimed
correctly. Camera link into MHW's camera, Minecraft pixels composited into MHW's
frame with validated occlusion, host terrain queries, the fingerprint and
signature discipline, the protocol with golden bytes, the evidence discipline —
every one of those is required by the real goal. The wrong 10% is the player
proxy (now retired to its honest role as the MHW-owned spectator mode) and
region-selected block placement (superseded by a full-frame composite).

**What changed.** `MODDING_PLAN.md` now opens with the contract as testable
properties rather than a sentence of intent, and the milestones run 3 Playable
Steve → 4 Terrain under Steve → 5 Combat exchange → 6 Building → 7 One quest →
8 Story → 9 Broader interaction. Ownership is stated as the keystone: exactly one
owner of input, movement and camera at every instant, negotiated by handshake,
visible in the HUD, with the MHW-owned mode kept intact because scripted
sequences need it.

**And the risky item is now first.** Milestone 3 opens with a spike —
compositing a *full* Minecraft frame at native resolution and 60 fps with the
player model visible, instead of one depth-selected block. Everything after it
depends on that working, it is the only item in the plan with a genuinely
unknown cost, and it is cheap to find out. `MODDING_PLAN.md` also gained a
"What could still kill this" section, naming the full-frame composite, writing
damage into MHW, elytra disagreeing with MHW's own physics, and quest state, so
none of them is discovered halfway through.

The lesson worth keeping: a plan that defers the keystone will be followed
faithfully and still arrive somewhere else. Ask what the keystone is before
milestone three, not after milestone six.

## 2026-10-05 — the spike: can Minecraft's frame be read at all?

Milestone 3 opens with the one item whose cost nobody could honestly estimate, so
it goes first and alone: read a rendered frame out of Minecraft, on demand, and
look at it.

Minecraft 26.2 changed the shape of this. There is no `Minecraft.mainRenderTarget`
any more and no `Framebuffer`; the game's own renderer moved to an abstract GPU
layer — `com.mojang.blaze3d.textures.GpuTexture`, `RenderTarget.getColorTexture()`
— behind `RenderSystem.getDevice()`, with a `GpuDeviceBackend` per backend. The
old `glReadPixels` plan would have been written against an API that no longer
exists.

The good news is that the new layer has the right primitive built in:
`CommandEncoder.copyTextureToBuffer(texture, buffer, offset, Runnable, mip, x, y,
w, h)`, with a mapped-buffer read via `GpuBuffer.map`. That is an async
texture-to-buffer copy with a completion callback, backend-agnostic, on the
render thread — no PBO plumbing, no `glFlush` guesswork, and it does not care
whether the backend turns out to be OpenGL or Vulkan. So the readback is queued
on one frame and read on a later one, and the render thread never blocks on a
full-frame download.

`FrameCapture` polls `<gameDir>/crafterhunter/frame.request` four times a second
and does nothing at all unless a request is there; the file is deleted *before*
the copy runs, so one request means one run even if the game is busy. Requests
are `capture [frames]` with the count capped at 600, parsed by `FrameRequest` —
the same parser the Python tool's output has to satisfy, so the two cannot drift
into a request that is silently ignored and looks like a failed capture.

`FrameLayout` holds the contract the native side will trust, and it is where the
one real subtlety lives: a GPU readback arrives with row zero at the **bottom**
on an OpenGL backend and at the **top** on Vulkan. Published frames are therefore
flipped to top-down once, in the guest, and the format is named in the meta line
(`rgba8-topdown`) rather than assumed by whoever reads it next. A consumer that
guessed would show MHW's sky at the bottom of Steve's world on one driver and
the right way up on the other, which is the kind of bug that gets blamed on the
renderer.

Each capture also writes a PNG next to the timings, and that PNG is the actual
acceptance: a frame that is upside down, from the wrong buffer, or missing its
geometry is obvious in a way no assertion would catch. The hook is the tail of
`GameRenderer.render` rather than the level renderer, deliberately — including
the HUD makes the readback self-checking before any native code assumes anything.

`tools/test-fabric-frame.sh` checks row order (including flip-twice-is-identity
and the one-pixel case), frame byte counts, the meta line's fields, that a zero
field does not read as absent, and every refusal in request parsing. Minecraft 26.2
needed no Fabric API at all: loader and Mixin only, and the mixin compiled
against the mapped jar.

Mod 0.3.0 rebuilt and installed (50,526 bytes, previous jar backed up to
`~/.local/share/crafterhunter-backups/minecraft-mod-20261005T172944/`). It needs
a Minecraft restart before anyone can ask it for a frame.

**Not done:** the native half. Nothing composites this yet — no shared-memory
transport, no per-pixel depth compare, no frame in MHW. The next step after a
human confirms the PNG is a correct frame is the transport, then the composite.

## 2026-10-05 — the frame capture shipped a bug that looked like a feature

The first live run of the spike accepted a request and then produced nothing.
No exception, no output file, and a summary that said `status=queued 1` — the
capture was neither working nor failing, which is the one state a request path
must never be in.

**The bug was an inverted flag, and it was mine.** One boolean stood for both
"the copy is in flight" and "the copy has landed". It was set true when the copy
was queued and cleared by the GPU callback when the copy completed, so the frame
loop published *while the copy was still being written* and re-queued the request
the instant the callback arrived. Every frame: publish nothing, queue again. The
request was genuinely accepted and genuinely never satisfied, and the only
visible symptom was a status line that had been written before the attempt.

Two flags now, because those are two different facts, and the transition between
them is a state machine: `IN_FLIGHT` waits, `READY` publishes exactly once, `IDLE`
may queue. `FrameCopyState` holds it as pure logic and `tools/test-fabric-frame.sh`
walks all three transitions plus the "callback fires twice" case, because a bug
this shape is invisible to integration testing — the request genuinely went in,
and nothing genuinely came out.

**The lesson is about the evidence channel, not the flag.** I designed the
summary file to make silent failures impossible, and then the first thing I did
with it was `writeSummary` only on request and only at the end, so the one
failure mode that mattered reported nothing. Every outcome now writes the
summary: on accept, on copy failure, on short readback, on completion. The HUD
line stopped saying "idle" for a failure too, because idle and broken are
indistinguishable from outside and the HUD is all most people will ever look at.

**Wasted time worth recording.** I burned several turns on Hyprland window-close
after being told to read the configs first. What actually closes Minecraft is
`SIGTERM`: `window.close()` reaches the game, which answers with a "Save and Quit
to Title?" dialog that waits for a click, so the window stays open and the
process never exits — which reads exactly like a broken close request. The
focus dispatch also takes an `HL.Window` object from `hl.get_windows()`, not an
address string, and the call has to go through `hl.dispatch` inside an `eval`
because the `hyprctl dispatch` shortcut only accepts a bare dispatcher
expression. `tools/mhw-control.py` now uses that form and
`tools/restart-minecraft.py` uses `SIGTERM`, both documented at the call site.

Mod 0.3.0 rebuilt (50,927 bytes) and installed. Full gate green: 19 protocol
tests, five headless suites including the new copy-state checks, gradle, plugin,
host verification.

**Not done:** still nothing composites this. No transport, no depth compare, no
frame in MHW. The next run answers one question — does a real frame come back,
and how long does it take.

## 2026-10-05 — the spike is answered: a Minecraft frame reads out, and it is cheap

Plugin and mod aside, the one number this project could not estimate: **a full
Minecraft frame can be read out of the GPU, on demand, correctly.**

30 frames requested, 30 captured, `status=done`, 1908x1028, 7,845,696 bytes —
exactly the size the contract predicts, checked by the headless suite rather than
by eye. The PNG is right way up: crosshair centred, HUD legible, hotbar along the
bottom, the ocean horizon where the horizon is, and our own `MHW LINK: LIVE` and
`PLAYER: LIVE` lines captured from inside the frame. Row order is therefore
correct on the first attempt, which is the part most likely to have needed
debugging.

**What it costs, measured rather than asserted.**

| Step | Cost |
| --- | --- |
| Read a 7.8 MB frame out of `/dev/shm` | 1.41 ms median, 3.70 ms worst of 20 |
| Write a 7.8 MB frame to `/dev/shm` | 4.7–5.1 ms |
| Frame size at 1920x1080 | 8.3 MB |
| Sustained if we ship every frame at 60 Hz | 0.50 GB/s |

So a frame crosses the boundary in single-digit milliseconds at this
resolution, and the honest reading is that **shared memory is enough** — no
zero-copy, no Vulkan external memory, no GPU handle sharing. That is the
expensive design I was going to have to write if this had gone badly, and it is
not needed.

The number that first came out was 1001 ms and it was a lie. `mapMillis` was
measuring from *queueing* a copy to *publishing* it, so it reported how long the
frame waited for me to ask for it — dominated by request pacing, not by the GPU.
It is now three numbers: `readMillis` and `flipMillis` are the work, `ageMillis`
is the waiting. Conflating them is how a cost measurement becomes a number
nobody can act on, which is the same mistake as the bridge that discarded 5,138
player packets while both ends looked healthy: a metric that measures something
adjacent to the thing you care about.

**What this settles.** Milestone 3 was restructured today around the fear that a
full frame might be unaffordable. It is affordable, so "Playable Steve" is an
ordinary engineering milestone and the order stands: transport (double-buffered,
one frame in flight, with an age), then the per-pixel depth compare that makes
occlusion correct rather than approximate, then the ownership handshake.

**The honest caveat.** 1908x1028 is not 1920x1080, and this measured a *file*
in `/dev/shm` rather than the double-buffered ring the transport will actually
use. The number will move; the order of magnitude will not, and the design
decision it drives — shared memory, not zero-copy — is safe either way.

## 2026-10-05 — the transport, and a cross-language test that earned its keep

The frame contract, stated on both sides and checked against each other.
`frame_transport.hpp` on the native side, `FrameChannel` on the guest, and
`tools/test-frame-transport-agreement.sh` compiles both and compares the
constants. Two slots, one frame in flight, a sequence number per slot, pixels
published before the header that claims them.

**The agreement test caught three real disagreements on its first run**, which is
the argument for writing it:

1. `sizeof(SlotHeader)` is 64 in C++ because of `alignas(64)`, not the 40 bytes
   its fields occupy. The guest had assumed the field count and written slot
   one's header inside slot zero's pixels.
2. `sizeof(BufferHeader)` is 128, not 64 — the `reserved[14]` array — so every
   offset downstream was wrong by half again.
3. Then, once the sizes matched: **the guest was writing big-endian.**
   `ByteBuffer.allocate` defaults to big-endian and every field was byte-swapped.
   This is the worst class of transport bug — every field present, at the right
   offset, holding the right value, and useless — and it is invisible to any test
   that only checks "did it write something".

The native struct also gained explicit field order with padding first. Leaving
layout to the compiler is fine until a second language writes the bytes by hand,
which is exactly what happened: the first C++ ordering and the first Java
ordering were mirror images of each other around the alignment boundary.

**A mutation test, because "the test passes" is not evidence.** The endianness
fix was reverted, the suite run, and confirmed to fail with the swapped values,
then restored. A suite that cannot fail on the bug it was written for is a suite
that will pass on the next one.

Field offsets are now pinned three ways: `static_assert` and `offsetof` checks in
the C++ test, byte-level reads in the Java test at the same offsets, and the
cross-language comparison. Fifteen constants agree: magic, version, format name,
bytes per pixel, slot count, both header sizes, both slot offsets, both pixel
offsets, and the buffer size for the live 1908x1028 capture.

**Also fixed while here:** `FrameLayoutTest` and `FrameChannelTest` were in the
default package while the classes they test are not, so `javac` put their
classes in two different directories and `java` could not find one of them. The
headless script runs them by their full names now, which is what stopped the
silent "Could not find or load main class" that was being misread as a test
failure.

Mod 0.3.0 rebuilt and installed. Gate green: 19 protocol tests, seven headless
suites (three new), gradle, plugin, host verification.

**Not done:** nothing reads the channel yet. The native side has the contract and
its rules, but no code maps or draws from it — that is the next piece, and it is
where the depth compare lives.

## 2026-10-05 — the compositing rule, pinned before it is written in a shader

The last piece of milestone 3's composite is the per-pixel question: given
Minecraft's pixel and MHW's depth at the same screen position, does Minecraft
draw? That is the whole difference between "Minecraft is inside MHW" and "a
Minecraft screenshot pasted over MHW", and it is written as
`frame_composite.hpp` with its tests *before* it goes into the pixel shader,
because a shader cannot be tested without a GPU and a backwards depth test still
produces a plausible-looking image.

**The comparison is reversed Z and that is the whole trap.** MHW clears scene
depth to 0 and nearer is the *larger* value, so "is Minecraft in front" is
`minecraftDepth > hostDepth`. Written the conventional way it inverts, and Steve
renders through walls — an image that looks fine in an open field and wrong in a
canyon. Both directions are asserted, because only one of them fails if the
comparison is flipped, and the mutation test confirms it: flipping the operator
fails 5 of the checks and restoring it passes them.

Two other rules came out of the same trace:

- A host pixel the pass never touched still holds the clear value, which under
  reversed Z is the *far* end. So depth 0 means sky, and Minecraft **must** be
  drawn there — otherwise a Minecraft building against the sky punches a hole in
  the world. This is the opposite instinct from "empty means discard", and it is
  why `hostHasDepth` is a separate flag rather than inferred from the value.
- Minecraft's own sky carries no information for MHW: it is Minecraft's sky, not
  the world's, and drawing it would replace MHW's sky with a flat gradient. So
  Minecraft pixels with no geometry behind them are `Unusable`, never drawn, and
  the frame's Minecraft coverage is reported so a frame that is 99% Minecraft sky
  can be refused before it costs any bandwidth.

Colour-without-depth is refused rather than drawn. Pasting it would put Minecraft
over MHW's characters; refusing it loses the frame. The failure is visible as
missing Minecraft instead of a Steve standing in front of a Rathalos.

Gate green, mutation test included. Next: the native reader that maps the shared
channel, uploads the texture, and runs this rule in the shader — which is the
first part of this that needs MHW running.

## 2026-10-05 — the native reader and the frame draw

`frame_source.hpp` maps the guest's channel once and hands out the newest
complete frame; `renderer.cpp` uploads it and draws it through the rule already
pinned in `frame_composite.hpp`. This is the piece that needs MHW running, and
everything that could be checked without it was:

- **Uploads are conditional on the sequence, not the frame rate.** The guest
  publishes at 20-60 Hz and MHW renders at its own rate; uploading 7.8 MB per MHW
  frame would cost more than the frame. A frame whose geometry changed means the
  guest resized, and the texture is rebuilt rather than stretched.
- **The two clocks are not assumed to agree.** The guest stamps frames with
  `System.nanoTime()` and this side reads `GetTickCount64`. Rather than convert
  and hope, the offset is measured once against the first frame seen and every
  later age is computed against that. Same machine, so drift is negligible;
  different machines would need a real handshake, which is stated in the code
  rather than left as a trap.
- **Minecraft's frame is centred, not stretched.** The two windows are almost
  never the same size and that is the normal case. Scaling to fit would shear
  Steve and, worse, shear the depth being compared against MHW's own, so a
  letterbox keeps the comparison honest.
- **The frame draws after the stone** and against the same depth candidate the
  stone used, so the two never disagree about what is in front within one frame.

`frame_source.hpp` is Windows-and-POSIX in one header, with the refusals stated
rather than at the call site: not mapped, shorter than its headers, no complete
frame, stale, or a frame claiming more space than the mapping holds. That last
one is the half-finished-resize case, and walking off the end of a mapping is the
sort of bug that reads as corruption somewhere else entirely.

Compiling it needed three fixes that are worth recording because two were mine:
`FrameView` and `SlotHeader` are in `crafterhunter::frame` and I wrote them
unqualified inside `crafterhunter::frames`; `munmap` takes a mutable pointer and
the view was declared `const void*`; and the member `newestFrame` shadowed the
free function `newest`, so an unqualified call inside the class would have
resolved to the member. The DLL builds at 20:11.

**Not verified:** nothing here has run inside MHW. The upload path, the shader,
and the letterbox mapping are all unexercised. Gate green across nine suites and
the mingw64 cross-compile, but the first run of this in the game is the real
test, and it is the step after this.

## 2026-10-05 — the gate that made the frame path unreachable

The first live attempt produced nothing, and the reason was mine rather than the
platform's. The frame composite was called from inside the stone's draw path,
which begins:

```cpp
if (ownDraw || ctx != context.Get() || (!traceDraws && (!drawEnabled || composed))) return;
```

`drawEnabled` only goes true once the plugin has a *placed anchor*. The MHW
restart cleared it, so every draw returned immediately. The frame composite could
not run without a stone placed — a debug aid silently deciding whether the player
was allowed to exist. The log showed depth candidates discovered but never
measured and no pipeline line at all, which is the signature of this and nothing
else.

Three changes, in the order they had to happen:

- **`selectSceneDepth()` is now shared.** The stone and the frame ask the same
  question and must get the same answer, or a block and a Steve in one frame
  could disagree about what is in front of them. It is cached per frame because
  the content measurement behind it can stall the GPU, and running it twice in
  one frame would pay twice for the same number. It is also, deliberately, not
  gated on the stone.
- **`ensureSharedState()` owns the fixed-function objects** that both paths bind —
  the context-state slot, the screen-size constants, the sampler, the rasterizer
  and the no-depth state. They were created inside `createPipeline()`, which only
  the stone calls, so the frame had nothing to bind even on the draw path that
  worked. All five have identical parameters for both draws, so two sets would
  only be two things to keep in step.
- **`createFramePipeline()` is called on first use by `ensureFramePipeline()`**,
  which fails once and reports once, instead of being chained off the stone's
  creation. A machine without shader model 5 should see one line, not one per
  frame.

The order inside `drawFrameComposite()` is deliberate and worth stating: pipeline,
then pixels, then depth. Uploading before the depth is known means a frame
arrives even in a frame where no depth is usable, so the next one has something to
show immediately. Minecraft still draws after the stone, so a block placed inside
Minecraft's geometry stays visible rather than being buried — the order decides
only who wins where both write.

`CH_Stop()` now releases the frame objects and unmaps the channel, which it did
not before; a reload would otherwise have left a stale mapping and a live texture.

**Not verified:** still not run inside MHW. This is the second attempt, and the
first failed for a reason that no host-side test could have caught.

## 2026-10-05 — two lost entry points, and the gate that should have caught them

The composite still did not draw, and this time the log said exactly why:

```
[32818087] FrameVS compile: CrafterHunter(70,10): error X3000: syntax error: unexpected token '('
[32818087] Minecraft frame pipeline unavailable; frame composite disabled
[32902979] VS compile: CrafterHunter(70,10): error X3000: syntax error: unexpected token '('
[32902980] Pipeline unavailable; drawing disabled
```

Line 70 was a verbatim duplicate of line 69, and there was no `VS` function in the
file at all. Both came from one careless edit: its `oldString` was the whole `VS`
function and its `newString` reused that body for `FrameVS` while also ending with
the `float4 PS(...)` line that already followed. So one edit renamed the stone's
vertex shader into `FrameVS` and duplicated `PS`. Nothing noticed for three
deployments because nothing on the host ever looked at the shader — the renderer
only logs its own compile failures, and only once someone is standing in an
expedition waiting for Steve to appear.

`tools/test-shader-source.sh` now compiles every entry point the renderer asks
`D3DCompile` for, with the names read out of `renderer.cpp` rather than listed in
the script, so a new entry point is validated without editing the test and cannot
be forgotten. Structural checks (brace balance, functions defined twice) run
whether or not a compiler is installed. glslang is not D3DCompile and will not
accept everything it does, so this is a floor rather than proof: it catches the
structural damage a text edit causes, which is what it is aimed at.

Mutation-tested both ways, since a gate that does not fail is worse than none:

- duplicating `PS` again → 5 checks fail, structural check catches it too;
- renaming `VS` away → `VS vs_5_0 FAILED / Entry point not found`, exit code 1.

Exit codes were checked directly rather than through a pipe, having first been
masked by `tail` in the mutation run — a gate whose failure is invisible in a log
is a gate nobody trusts.

## 2026-10-05 — X4500, and the depth the transport does not carry

Third deployment, third distinct failure, and the sharpest one yet:

```
FramePS compile: CrafterHunter(26,18-31): error X4500: overlapping register semantics not yet implemented 't1'
```

**The stone and the frame both bound `t0`/`t1` inside one source string.** `D3DCompile` parses the entire unit and rejects the file; glslang compiles each entry point in isolation and had happily accepted it. So the gate added an hour earlier did not catch this, and the honest conclusion is that it is a floor rather than proof — now stated in the script's own comment.

The two shaders are now separate string literals, `Shader` and `FrameShader`, each declaring `t0`/`t1` once.

**The deeper problem was hiding behind the compile error.** `FramePS` sampled
`minecraftDepth` at `t1` while `drawFrame` bound *MHW's* `sceneDepth` there. So
the shader wanted Minecraft's depth and would have been handed MHW's, comparing a
surface against itself and producing an image that looked plausible and was
meaningless. The pinned rule in `frame_composite.hpp` wants Minecraft's own
**linear eye depth** plus a flag for "Minecraft drew nothing here".

None of that exists. The transport is `rgba8-topdown`, `BytesPerPixel = 4`: colour
and nothing else. Per-pixel occlusion between the two games cannot be implemented
against this transport, and pretending otherwise is how a plausible-looking image
gets mistaken for a working one.

So the shipped rule is the interim one, `decideSkyOnly()`: draw Minecraft where
MHW holds no geometry, refuse everywhere else. That is the pinned rule with the
depth comparison unavailable, not a loosened version of it — `decide()` refused
colour-without-depth because *unconditional* pasting would cover the hunter, and
restricted to the host's own empty pixels nothing is covered.

**What this costs, stated plainly:** Steve appears against MHW's sky and is *not*
occluded by MHW's terrain. A tree between him and the camera will not hide him.
Minecraft's sky is indistinguishable from its geometry without depth, so the
letterbox carries Minecraft's sky with it. Full occlusion needs Minecraft's depth
attachment published and linearised — a transport change and a guest change, not a
shader tweak — and it is now written down as its own piece of work rather than
left as a comment promising it will arrive.

`decideSkyOnly()` is unit-tested with four assertions, including one that states
the gap as a test: a nearer Minecraft pixel over host geometry is `Draw` under the
full rule and `Skip` under the interim one. If that ever starts passing, depth has
arrived. Deleting the rule's refusal fails all four.

Two of my own test expectations were wrong on the way in, both caught by running
them: `DepthEpsilon` makes *equal* depths draw rather than skip, and I had the
reversed-Z direction backwards in the "nearer Minecraft" case. The rule was right
both times.

The 1x1 `minecraftDepthProbe` texture is gone. It existed only to stand in for a
binding the shader no longer reads, and leaving it would have suggested depth was
on its way without saying what it would cost.

## 2026-10-05 — the Windows mapping length, which made Steve impossible

Diagnosing the frame path before shipping it turned up three faults, each fatal on
its own. This is the first, and it is the reason nothing could ever have appeared
in MHW regardless of how many frames were published.

`frame_source.hpp` read the channel's length with `GetFileSizeEx` on Windows and
then **threw the value away**. `length_` was only ever assigned in the POSIX
branch, so on Windows it stayed `0`, and `newestFrame()` opens with:

```cpp
if (length() < headerBytes() + SlotCount * slotHeaderBytes()) { ... refuse ... }
```

`0 < 256`, so **every frame was rejected as "channel shorter than its headers",
forever.** No log line, no crash, no way to tell from the game that the reader was
alive and refusing. The comment directly above the discarded read even said the
length "is the only size that can be trusted before a header has been read".

No host test could have caught it, and that is the part worth keeping: Linux
compiles the POSIX branch, so the branch that was wrong is the one the test suite
never built. A test of the *arithmetic* would still have passed, because the
arithmetic was correct — the value it was given was not.

So the fix removes the failure instead of repairing it. On Windows the length is
now obtained from the mapping itself via `VirtualQuery`, so there is no cached
length in that branch to forget:

- `channelLongEnough()` is extracted as a pure rule and tested, including that a
  zero length is never sufficient and that one byte short of the headers is
  refused;
- the refusal now distinguishes `mapping length unknown` from
  `channel shorter than its headers`, because a zero means the mapping's size
  could not be established, which is a different failure from a truncated file;
- an empty file is refused at `open()` on both platforms. The guest creates and
  fills the channel in one constructor, so an empty one means we caught it
  mid-creation, and mapping it yields a zero-length section whose every read
  fails — a far more confusing symptom than "not there yet".

One test asserts the *shape* of the Windows branch: that it exists, that it asks
the mapping for its length, and that it contains no assignment to the stored
length. Mutation-tested: reinstating the cached `length_` fails both of those
assertions by name, and weakening the length rule fails the boundary check.

**Build passes; nothing here has been seen in-game yet.** That distinction is the
subject of the next entries.

## 2026-10-05 — the timestamp conversion, and why the clocks cannot be compared

The second fault, and the one that had been hiding behind the first.

The conversion was wrong by a factor of ten thousand:

```cpp
GetTickCount64() * 1'000'000ull / 10'000ull   // milliseconds -> ?
```

GetTickCount64 counts **milliseconds**, so milliseconds to nanoseconds is a
multiplication by one million. That expression divides as well and yields
milliseconds times one hundred — a number far too small to compare against
anything. `fresh()` opens with `if (capturedNanos == 0 || nowNanos < capturedNanos)
return false;`, so every frame looked like it had been captured in the future and
every frame was refused.

Worse, the offset that was supposed to rescue this could not be learned: it was
measured only *after* a frame passed the freshness gate. With clocks that genuinely
disagree, no frame passes the gate, so the offset is never learned and no frame is
ever accepted. The two requirements are contradictory.

**Measured rather than assumed.** The guest stamps frames with `System.nanoTime()`
and this process reads `GetTickCount64`. Reading a live channel:

```
guest stamp is 3422486.7 ms behind CLOCK_MONOTONIC
```

Fifty-seven minutes, which is what a JVM that counts suspend time and a kernel
whose monotonic clock stops during suspend will disagree by after a laptop has
been closed once. Not a constant, and not calibratable.

That killed the plan of correcting for the difference, and not only because of the
deadlock. Fitting an offset to a single sample makes `local - guest` equal that
offset *by construction*, so the age it reports is identically zero — for every
frame, forever. Both approaches were implemented, both were wrong, and the tests
that caught them are in `frame_clock.hpp`'s header comment so the next person does
not try them again.

**What runs instead.** Age is measured on the reader's own clock, from whether the
guest's sequence number advanced recently. That needs no agreement between the
processes and answers the question that matters — whether the world in the frame
has moved on.

The bound is deliberately loose, and the reason is measured too. The transport's
`MaxAgeNanos` is 50 ms, but publication is gated on a GPU read-back returning, and
the measured interval is **140 ms when Minecraft is busy and 1000 ms when it is
not** (`ageMillis=1000.5`, one sequence per second). A 50 ms bound would refuse
every frame ever published. `StallBoundNanos` is 3 s: it clears the slowest
observed rate with room to spare and still notices a stopped guest quickly. A test
asserts the bound exceeds both measured rates, so it cannot be quietly tightened
back to a number that refuses everything.

`chooseAgeSource()` keeps the transport's own rule for the case where clocks *do*
agree, so this is a decision made from a measurement rather than a replacement of
the contract. The chosen source is logged once with the divergence, so the log
states which rule is in force instead of leaving it to be inferred. One test
asserts the branches do not rescue each other: a fresh-looking guest stamp must not
rescue a frame that has stopped publishing.

`lastUploadedSequence` and `lastAdvanceSequence` were separated, because the upload
dedup and the liveness check are different questions and sharing one counter made
the second unaskable.

**Still unverified in-game.** This is the state of the code, not the state of MHW.

## 2026-10-05 — safe frame publication, and a test that could not fail

The third fault. `FrameChannel.publish()` wrote pixels, then a header, and never
marked the slot incomplete in between:

```java
channel.write(pixels, ...);            // slot still says published = 1
channel.write(header, ...);            // from the frame before this one
```

With two slots the *newest complete* header always names the slot that is not
being written, so a reader choosing by sequence was safe. The exposure is the
other end: `newestFrame()` hands back a pointer into shared memory and the caller
then spends milliseconds uploading 7.8 MB out of it, while the guest recycles
that same slot two frames later. Nothing re-checked, so the upload could composite
half of one frame and half of another — an artefact indistinguishable from a
rendering fault.

Publication is now three writes: mark incomplete (keeping the sequence, so a
reader can still tell what the slot used to hold), write pixels, mark complete.
The header write also loops until it is out; `FileChannel.write` may write fewer
bytes than asked and the header write was the one call that never checked. No
`force` is issued: both processes share this host's page cache, so a flush would
cost a 7.8 MB round trip per frame to achieve nothing, and every write goes
through `write(2)` so the ordering is the kernel's to keep.

On the native side `Source::stillHolds()` is asked *after* the upload whether
that is still the frame just uploaded, and a false answer drops the frame. One
frame of latency is invisible; a torn frame is not.

**The test for this took four attempts and the first three were theatre.**

- It watched a 16 KB frame and never caught the window. Too small to observe.
- It read the whole 8 MB buffer per sample, so one sample took longer than forty
  publishes and the run reported "the reader never ran".
- It waited for that reader with `Thread.onSpinWait()`, which starved the thread
  it was waiting for. On twelve cores.
- With the reader fixed it still **passed with the fix deleted**. 5430 samples,
  22 caught mid-refill, and every one of those was from the *first* publish,
  where `published` is 0 simply because the file was zeroed — not the state under
  test.

A test that passes when the bug is present is worse than no test, because it is
taken as evidence. What made it real:

- the reader now settles both slots *before* starting, so the zeroed-file state
  cannot be mistaken for the incomplete state;
- it samples the header, the first and last pixel bytes, then the header again.
  Tearing is not "pixels changed" but "looked finished, changed underneath, still
  looks finished" — which is also exactly what `stillHolds()` checks in the
  renderer, so the test and the fix assert the same condition;
- the frame is 16 MB. The window is only as wide as the pixel write, and at
  949×1028 that is microseconds — rare enough that the bug survived. A write
  lasting milliseconds makes it impossible to miss.

Deleting the incomplete write now fails with the tearing assertion. Restored, five
consecutive runs pass with 124–169 mid-refill samples, **all** of them marked
incomplete.

Two incidental finds: `FileInputStream.seek` **no longer exists in JDK 27** (the
legacy channel methods were removed), so the sampler uses positional
`FileChannel.read`. And an array element incremented by one thread and polled by
another is not guaranteed visible — the spin loop read a hoisted value forever.
Atomics, and a yielding wait.

**Build passes. Not yet seen in-game** — that is the next entry, and it is the
only one that can close this.

## 2026-10-05 — first run of the fixed reader, and a mistake of mine

The new DLL loaded and the frame path ran for the first time:

```
Shared draw state ready (context state, constants, sampler, rasterizer, no-depth)
Minecraft frame pipeline ready (colour only: sky-against composite, no depth channel)
Minecraft frame refused: no complete frame
```

Faults 1 and 2 are fixed — the length is derived, the conversion is right, and the
reader got as far as *deciding* about a frame instead of refusing on arithmetic.

**Then I broke it myself.** `verify-frame-composite.sh` deleted the channel before
requesting a capture, on the reasonable assumption that a new capture means a new
file. It does not: the guest holds the channel open for the life of the capture,
so deleting the path left Minecraft writing to an unlinked inode. The guest kept
reporting healthy publishing — `seq=30 published=1` — while no reader could ever
see it. The worst combination to debug, and self-inflicted.

The script no longer deletes it. The channel comes back when the guest reopens it,
which `reopenChannel()` only does on a **size** change, so a nudge of the
Minecraft window is enough and no restart is needed.

**The second thing the run exposed is a real gap.** Refusals were logged only when
the reason *changed*. The reader refused every frame with "no complete frame", said
so once at startup, and was then silent for the rest of the session — a log
identical to a reader that had stopped being called at all. Nobody should have to
guess that distinction from a log, so refusals now report on change and then once a
second while they persist.

A third, smaller defect in my own script: it created the evidence *parent*
directory but not the per-run one, so every `tee` failed and the run reported "no
evidence" while the capture itself had worked.

Not yet a composite on screen. Next: recreate the channel, re-run, and read the
upload line.

## 2026-10-05 — the transport rejected the host's resolution, not the frame's

The refusal the live run reported was not the clock or the mapping. It was this,
in `usable()`:

```cpp
if (slot.width != width || slot.height != height) return false;   // width/height = the HOST's
```

`width` and `height` were the *host's* 1920×1080. The guest publishes 1908×1028.
The two are never going to be equal — that is precisely why there is a letterbox —
so the check refused every frame the transport has ever been asked to carry. The
"no complete frame" line was accurate and completely misleading.

A frame's dimensions are the guest's to declare. They are now validated two ways
instead:

- **`usable()`** keeps only self-consistency: magic, both versions, a non-zero
  sequence, `published == 1`, and non-zero dimensions. A zero dimension is refused
  before any offset is computed from it.
- **`within()`** is new, and checks a slot's own geometry against the size of the
  mapping. The *sum* is tested, not either term: a slot can claim a size whose
  offset is comfortably inside the buffer while its pixels run past the end, which
  is exactly what a half-finished resize leaves behind.

Two more refusals came out of writing the tests:

- **Slots disagreeing about geometry are refused rather than resolved.** A slot's
  pixel offset depends on the buffer's declared geometry, so using one slot's
  dimensions to locate the other's pixels reads the right number of bytes from the
  wrong place. The next frame settles it.
- **Refusals carry a reason.** `newest()` now says which of the five ways it
  declined. A reader that refuses silently is indistinguishable from one that
  stopped being called, which is a distinction that has now cost this project two
  debugging rounds.

Regression-tested against the real numbers, and the mutation is behavioural rather
than cosmetic: putting the host-dimension rule back *inside* the new signature, so
the API is unchanged and only the behaviour differs, fails eight checks including
`a 1908x1028 frame is chosen by a transport that has never heard of 1920x1080`. An
earlier attempt at that mutation only changed a function signature and produced a
compile error, which proves the API moved and proves nothing about behaviour.

`newestFrame()` lost its host-resolution parameters entirely. The renderer change
is one line; the stone renderer and depth selection are untouched.

## 2026-10-05 — bounded recovery, so a stale mapping cannot outlive the channel

`uploadNewestFrame()` returned on "no complete frame" without re-opening anything.
So once its mapping described something the guest was no longer writing, it kept
describing it for the rest of the session — and nothing observable *from inside that
mapping* would ever have said otherwise. Three ways in, all with the same symptom and
the same remedy:

- the file is unlinked while the mapping is held. Not hypothetical: a verification
  script of mine deleted the live channel, and the guest went on publishing to an
  unlinked inode while reporting healthy sequence numbers.
- the guest recreates the file, at a new size after a window change.
- the guest restarts, so sequence numbers begin again at one.

`recoverChannel()` now handles all three, rate-limited to once per second by
`shouldRemap()` in `frame_source.hpp`. Bounded rather than eager because the reader
runs every frame and the channel is legitimately absent for as long as Minecraft is
closed — an unbounded retry would be a syscall storm in the most common case.

On a successful re-open it **forgets what it was holding** before the mapping goes:
`lastUploadedSequence`, `lastAdvanceSequence`, `lastAdvanceNanos` and the age-source
decision. That is what makes a guest restart safe — a sequence of 1 must not be
mistaken for the frame already on the texture — and a resized channel safe, since the
new geometry must not be compared against the old one.

The liveness path had its own remap under a different interval
(`StallBoundNanos / 3`). Two bounds for one remedy is one bound too many, so it now
calls the same routine. The recursion that introduces is bounded: recovery sets
`lastRemapNanos`, so the nested call cannot remap again, capping the depth at one.

`mayRemap()` treats a clock that has gone backwards as due, and a reader that has
never tried as due — otherwise the subtraction could never reach the interval and
recovery would deadlock permanently.

One test expectation of mine was wrong on the way in and the suite caught it: I
asserted that re-opening *is* allowed at the instant of the last attempt. It is not —
that attempt set the clock. Getting this wrong in the permissive direction is one
remap per frame, so the boundary is now asserted from both sides.

## 2026-10-05 — the letterbox was never letterboxing

Extracted from `renderer.cpp` into `frame_composite.hpp` so it could be checked
without a GPU, and immediately found to be **inverted**.

```hlsl
float2 uv = hostUv * uvRect.xy + uvRect.zw;   // what it did
```

That scales the *screen* coordinate. To draw a rectangle of relative size `scale`
starting at `offset`, the screen coordinate has to be brought *into* the rectangle:

```
guest = (host - offset) / scale
```

Scaling instead inverts the relationship. The region drawn becomes
`host ∈ [-offset/scale, (1-offset)/scale]`, and for 1908×1028 into 1920×1080 that is
**[-0.022, 1.022] vertically** — larger than the screen at both ends. So Minecraft was
not letterboxed at all. It was **cropped, about 2% lost off the top and the bottom,
and stretched to fill all 1920×1080** — the single thing the letterbox exists to
prevent, and the thing that would have silently invalidated the depth comparison the
composite depends on.

Extracting it also surfaced two smaller defects:

- **`scaleX` came out as 1.0000003** on the tight axis, which made `offsetX` slightly
  negative and mapped the host's own edge column to a uv below zero, discarding it.
  The scale is now clamped to the host.
- **A degenerate rectangle mapped everything to uv (0,0)** — which is *inside*
  `[0,1]` — so a zero-sized guest would have drawn one stray texel at the centre of
  the screen instead of nothing, contradicting the comment that claimed otherwise.
  `insideGuest()` now rejects it, and the shader checks a `frameFlags.x` validity flag
  because a NaN uv would sail past the discard (every comparison against NaN is false).

Two of my own assertions were wrong before the code was:

- I asserted the host's **left and right edges were outside** the rectangle. For these
  sizes there is no side bar — the guest is wider relative to the host than it is tall —
  so the frame spans the full width and those columns are correctly *inside*.
- I passed `offsetY` as the **horizontal** argument in three places.

Both were caught by the suite rather than by reading, which is the argument for
extracting the arithmetic rather than trusting it.

Mutation-tested: putting the scaling form back fails **eight** letterbox assertions,
including `the host's top-left corner is outside the rectangle` — which is the
assertion the bug itself made false, and which the old test asserted the other way.

The shader and the host now both derive from the one tested function, so the mapping
cannot drift between them again.

## 2026-10-06 — recovery that stopped resurrecting frozen frames

`recoverChannel()` zeroed `lastAdvanceNanos` on **every** successful re-open. But
re-opening a channel whose file still exists always succeeds — including when that file
is the *same frozen one*. So once per second the reader forgot the sequence had stopped
advancing, saw the same unmoving sequence as if it were fresh progress, and drew a
frame from a guest that had gone away. Recovery was not merely failing to help; it was
actively defeating the stall detection meant to stop exactly that, and it would have
done so indefinitely while looking healthy in the log.

The distinction needed is between a channel that is *the same writer, stuck* and one
that is *a different writer*. `classifyGeneration()` returns exactly that, from
evidence rather than from the file merely existing:

| Evidence | Verdict | Consequence |
|---|---|---|
| sequence unchanged, geometry unchanged | `Same` | keep the memory, keep refusing |
| sequence went **backwards** | `Restarted` | clear |
| geometry changed | `Resized` | clear |
| magic or version unrecognisable | `Replaced` | clear |

A *higher* sequence is deliberately `Same`: that is progress by the writer we already
know, not a new one. The regression test simulates the loop — memory, a stalled
sequence, five re-opens — and asserts the frame is still refused after each, so a fix
consisting of "never forget" would not pass it. The three genuine-new-generation cases
are asserted too, for exactly that reason.

Mutation-tested: making `shouldForget` return true unconditionally fails the test
repeatedly (`so the reader must not forget what it knows about it`), exit 1.

The reader's memory of the writer is now one struct, `FrameMemory`, holding the
sequence, the geometry and the liveness clock together. They are one fact — "what the
writer was last seen doing" — and splitting them across three globals is what let the
clock be cleared while the sequence was not.

`Source::headerValid()` was added because I first passed a *guess* for it: inferring
the header's validity from whether a frame happened to come back. That guess produces
the wrong verdict exactly when it matters, since a valid header holding nothing
publishable is the normal state before the guest's first frame.

## 2026-10-06 — the Windows length was page-rounded, and the path was a guess

Two claims about the Windows reader had never been checked, and both were wrong in a
way a host test could not see.

**The length was not the file length.** The previous fix replaced a missing assignment
with `VirtualQuery`, reporting the mapping's `RegionSize`. That is the size of the
*region*, rounded up to the allocation granularity — for a 15,691,648-byte channel it is
thousands of bytes more than the file contains. `within()` uses that number to decide
whether a frame's pixels fit, so a rounded-up length would accept a frame extending past
the real end of the file: no crash, since the mapping is larger, but pixels of nothing.

Now both platforms produce a byte count and a **single** assignment sits after the
platform split, so neither branch can bypass or omit it. `length()` has no
platform-specific variant at all, which is what removes the possibility rather than
patching the instance.

The test asserts the *shape* of the code, because a host test cannot run the Windows
branch and so cannot observe a zero length there. What it can insist on: exactly one
`length_ =` inside `open()`, positioned after the last `#endif`; no `#if` in `length()`;
no `VirtualQuery` anywhere in it. Mutation-tested both ways — reintroducing
`VirtualQuery` and dropping a path spelling each fail, exit 1.

**The path was a single unverified spelling.** The reader is a Windows process under
Proton handing `CreateFileA` a Unix-style path. Wine accepts those and maps them onto
`Z:`, but if that ever stopped being true the symptom would be "no frame arrives" —
identical to a guest that has not started. Both spellings are now tried, `Z:\dev\shm\...`
second, and **which one worked is logged** rather than left to be inferred. The test
pins that the Unix spelling is byte-for-byte the path the guest writes to (otherwise the
two processes are using different files), that the drive spelling has no forward slashes,
and that an out-of-range index falls back instead of reading past the list.

What this cannot tell us: whether Wine actually accepts the Unix spelling on this host,
or whether it needs the drive form. That is now a single log line rather than a
hypothesis — `frame channel opened via the ... spelling` — and it will say so the first
time the reader runs.

## 2026-10-06 — the guest and the reader, tested together

Fabric recreated the channel only when the window's dimensions changed. A channel that
was deleted — or replaced by another file of the same size — therefore left the guest
holding an open handle to an **unlinked inode**. It kept publishing, reported healthy
and rising sequence numbers, and every byte went to a file no reader could open. From
the guest's side everything was perfect, which is the worst combination to debug, and it
happened here because a verification script deleted the live channel.

`FrameChannel.channelLost()` now answers whether the file at the path is still the one we
are writing, and `writeShared()` recreates on a lost channel as well as a size change.
The check costs two stat calls, so it is rate-limited to once a second rather than made
per frame — a second is far faster than the failure it prevents.

Presence alone is not what is checked, and cannot be: a replaced file of the same size
reads as present and correctly sized. What reveals that is the sequence numbers going
backwards, which the native reader treats as a restarted writer. The test asserts the
recreated channel's sequence is *lower* than the one the reader last saw, because that
inequality is the reader's only evidence.

**The test that was missing is the integration one.** Each half had been tested alone,
and the fault above is invisible from either side: a predicate that correctly reports
"the file is gone" can coexist with a reader still serving a frame from an inode nothing
writes to, and a guest can report a healthy channel while the reader sees nothing. So
`tools/test-frame-recovery.sh` compiles a probe from the real headers
(`frame_source.hpp`, `frame_transport.hpp`, `frame_clock.hpp`) and has the Java guest
drive it across five stages — publish, unlink, recreate, resize, restart — asserting on
what the **reader** reports at a path the guest really writes to.

Two things came out of writing it:

- **A page-rounded length is caught by the integration test and not by the unit test**,
  because it makes the probe die rather than return a wrong number — `munmap` is then
  called with a length that was never mapped. That the failure is a crash rather than a
  clean assertion is itself worth knowing.
- **`Source::open()` gained an optional path**, defaulting to the fixed channel path. It
  is a parameter only so a test can point a reader at its own file; a runtime-configured
  path in production would be a second source of truth about where the two processes meet,
  which is the one thing this header exists to avoid.

Two of my own mutations were worthless, and the second only after checking. Removing the
`Files.exists` guard did **not** fail the test, because `Files.size` on a missing file
throws `NoSuchFileException` and the catch already returns true — so that guard is
redundant for correctness and kept only because throwing is expensive in the render
path. Mutating just its final `return` also did nothing, since the guard short-circuits
first. Replacing the whole method does fail, with the assertion naming the consequence. A
mutation that cannot fail is worth as little as a test that cannot.

## 2026-10-06 — Steve renders inside MHW. Screenshot, not inference.

First live composite. Measured, in order:

```
[2534188] re-opened the Minecraft frame channel (was absent, size 15691648): restarted, memory cleared
[2534189] frame channel opened via the Unix-style spelling (15691648 bytes)
[2534192] First Minecraft frame uploaded: seq=42 1908x1028
[2535203] Compositing Minecraft frame with MHW scene depth
```

- **Guest sequences advancing**: sampled the channel twice four seconds apart; the
  sequence moved by 58, about 14 fps, newest frame 126 ms old.
- **Upload**: `First Minecraft frame uploaded: seq=42 1908x1028`.
- **Draw issued**: `Compositing Minecraft frame with MHW scene depth`.
- **Visible**: `screenshot-live.png` and `live-a.png` / `live-b.png` in
  `build/evidence/frame-composite/`. Minecraft's ocean, its blocky cliff, the hotbar,
  the "Potion" stack and our own HUD are composited into the middle of MHW's screen,
  **and the hunter, the Palico, the minimap and MHW's control prompts are drawn over
  it** — the sky-only rule behaving exactly as specified: Minecraft fills the pixels MHW
  left empty, MHW's geometry wins everywhere else.
- **Liveness, visually**: `live-a.png` and `live-b.png` four seconds apart differ in
  99.95% of the pixels of the HUD band, so the composited content is moving. That is a
  stronger statement than any log line, because a log line only says a draw was issued.

**Two more things the run settled.**

*The Windows path question is answered.* `frame channel opened via the Unix-style
spelling` — Wine accepted `/dev/shm/crafterhunter/frame.channel` outright and never
needed the `Z:` fallback. The fallback stays, because a silent fallback from "the path
spelling Wine stopped accepting" to "the guest has not published" is precisely the
failure this file has produced before.

*The publisher stall is fixed in-game, which is the point of fix 1.* When a capture ran
out, the reader logged `same writer, memory kept` with `frame is not being published`
once a second, 168 times, and uploaded **zero** further frames. Before that fix it
zeroed the liveness clock on every re-open, so a frozen Steve would have been redrawn
every second indefinitely while the log looked healthy. A guest restart was exercised
live too — Minecraft was relaunched before this run — and the reader classified it
`restarted, memory cleared` before uploading.

**Two defects in my own verification script, both found by it disagreeing with the
screen.**

1. It reported `pipeline: NOT created` because it searched only the log since its own
   mark, while the pipeline is created once per session long before any capture.
2. It reported `upload: none` while uploads were demonstrably happening, because the
   upload line is guarded to fire once. Its absence means nothing at all.

Both now read from the session start, and the verdict states that liveness is judged by
whether the stall lines have stopped and by the screenshot.

**Still not verified: a window resize.** The channel is still 1908×1028 and no resize has
happened this session, so the `Resized` branch of the generation classifier has live
evidence only from the integration test, never from the game. Dragging the Minecraft
window is all that is needed, and it is the last thing between this milestone and
"verified".

**Unrelated and also broken: the bridge is down.** Minecraft's HUD reads `MHW LINK:
AWAITING | 0 pkt/s` and the log shows `PortUnreachableException`, so the camera and
player-proxy links are not running. That is the UDP bridge, not the frame path, and
nothing in this milestone depends on it — but the HUD is reporting two dead links and
that should not be left looking healthy.

## 2026-10-06 — measuring the link instead of reading it off a screenshot

The bridge was down: no process, nothing on UDP 38470, and Minecraft's log full of
`PortUnreachableException`. The binary was newer than its source, so no rebuild was
needed; it is started detached with `setsid` so it survives the shell that launched it.
Both endpoints registered within five seconds and the plugin reported:

```
Connected to crafterhunter-bridge/v1
Bridge HelloAck received; camera reads are armed after the startup grace.
First guarded player read succeeded.
First guarded camera read succeeded.
Received actual minecraft:stone pixels and PNG from Minecraft.
```

**Reading that off a screenshot would have been the wrong instrument**, twice over. The
HUD sits exactly where Minecraft's own F3 debug screen does, so the first attempt to
photograph the telemetry got the debug screen instead — which was itself useful, since it
reports the guest build as 1.27.8.1 / 27.8.1. And a composited image cannot say which
frame it came from, which is the whole question.

So `LinkStatus` writes the same numbers the HUD draws, plus the camera pose, to
`crafterhunter/out/link.txt` at 4 Hz, and `tools/sample-link-status.py` samples it. The
pose is recorded because it is what alignment is measured *against*: yaw span, pitch span
and position travel over a window say whether synchronized movement is even possible
during that window, and a static camera would make any such demonstration meaningless.
Failures are swallowed deliberately — instrumentation that can interrupt the frame it
measures is worse than none.

**A test of mine was silently environment-dependent.** `checkUnmappedRefuses` opened the
*fixed* channel path to prove that opening a missing channel fails. With Minecraft
publishing, the "unmapped" source mapped a live channel and read a real frame, and two
assertions failed for reasons that had nothing to do with the rule under test. It now
opens a path that cannot exist. This is the second time a test of mine has depended on
the machine's state rather than the code's, and it is worth stating as a rule: a test
whose result changes because a game is running is not a test.

## 2026-10-06 — what depth capture would actually take, checked rather than assumed

Before promising item 3, the three things that could have made it impossible were
checked against the deobfuscated 26.2 client jar rather than guessed:

1. **Is the depth attachment reachable?** Yes. `RenderTarget` exposes
   `protected GpuTexture depthTexture` with public `getDepthTexture()` and
   `getDepthTextureView()`. `MainTarget` inherits it. So depth is the same kind of
   object the colour read already uses, and `CommandEncoder.copyTextureToBuffer` is
   generic over `GpuTexture` — the call proven for colour takes depth unchanged.
2. **Is the current injection point wrong for this?** Yes, and now known rather than
   suspected. `FrameCaptureMixin` injects at `GameRenderer.render(...)` TAIL, which is
   *after* GUI compositing. That is why our own HUD text and Minecraft's F3 debug screen
   both appear in the composited frame: they are in the texture being copied. A
   world-only capture wants `GameRenderer.renderLevel(DeltaTracker)`, which is public and
   ends before the GUI pass.
3. **What about hands?** `renderItemInHand(CameraRenderState, float, Matrix4fc)` is
   private and called from inside the level path, so the first-person hand is drawn
   *within* `renderLevel` and no injection point in that method excludes it. That is a
   real constraint rather than a scheduling detail: "world colour and depth before
   hands and HUD" needs either a hook inside the hand call itself or acceptance that the
   hand is part of the world pass. Which of those is right depends on whether item 5's
   isolated scene is played first- or third-person, and that has not been decided.

So depth capture is feasible, not blocked. What is *not* established is the depth
format `allocateDepthAttachment` picks, the linearisation from that format plus
Minecraft's projection, and the normalisation against MHW's reversed-Z — items 4 and 6,
which are the parts where a wrong answer produces a plausible image rather than an
error.
# 2026-10-06 — bounded capture-lifecycle repair (headless)

Single-attachment completion now uses a capture-owned completion object; format
metadata no longer dereferences unrequested textures. Partial completion times
out too. Timeout or setup/copy failure disables new capture until process restart,
retaining the existing buffer pair rather than freeing/reusing uncertain GPU
resources. This is a conservative diagnostic policy, not a depth-backend fix.

The world verification script now emits `colour 1`, `depth 1`, or `capture 1`
rather than the invalid `capture colour` command. Commands are atomically renamed
into place and the consumer claims them before reading/deleting. AGENTS.md records
the goal, actual Git directory, working checkpoint, and next acceptance gate.

Verified: tools/test-fabric-frame.sh (including new completion checks), Fabric
`./gradlew build`, and Git diff whitespace check. No games launched, no deployment,
no runtime tests. GPU completion still does not establish valid copied pixels;
D32_FLOAT readback, row layout and paired depth acceptance remain unresolved.

# 2026-10-06 — the D32 readback refusal, measured instead of argued

The live log line that stopped every depth attempt was `GL_INVALID_FRAMEBUFFER_OPERATION
in glReadPixels(incomplete framebuffer)` at 04:18:57. Which recipe produced it was
settled against the deobfuscated 26.2 jar, not guessed: `GlCommandEncoder.
copyTextureToBuffer` binds the source with `bindFrameBufferTextures(readFbo, texture, 0,
mipLevel, GL_READ_FRAMEBUFFER)`, which puts the image on `GL_COLOR_ATTACHMENT0` and 0
in the depth slot. A D32_FLOAT image on a colour attachment makes the FBO incomplete,
`glReadPixels` is refused with 1286, and the backend then throws
`IllegalStateException("Couldn't perform copyTobuffer ...: GL error 1286")` *after*
`queueFencedTask` — so an earlier comment claim that a refused copy "raises nothing in
Java" was wrong and the comment is corrected where it survived.

`tools/gl-depth-readback-probe.c` (run by `tools/test-depth-readback.sh`) answers with
a real driver instead of a build. On both vendors available here — the default NVIDIA
RTX 2050 and, forced through glvnd, the AMD Radeon 660M / Mesa 26.2.4 that Minecraft
itself logs — the game's recipe gives `GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT` plus
`GL_INVALID_FRAMEBUFFER_OPERATION`, while the corrected recipe (depth on
`GL_DEPTH_ATTACHMENT`, `glReadBuffer(GL_NONE)`, completeness checked before reading)
returns the written values exactly. The same probe pins the row contract the capture
metadata had wrong: rows are tight `width*4` bytes, row 0 is the framebuffer's *bottom*
row, and a 4x larger destination changes neither — so capacity is not stride, and the
old `depthRowsPadded = capacity > width*height*4` could only ever be false because the
buffers are allocated exactly `width*height*4`. `glReadBuffer(GL_DEPTH_ATTACHMENT)` is
`GL_INVALID_ENUM`; `glGetTexImage` works as a fallback.

Consequently the depth half leaves the shared copy path: new `DepthReadback` (GL state
saved and restored around `GlStateManager`'s cached framebuffer bindings, completion
queued behind the fence, fail-closed on refusal) driven by the pure contract in
`DepthReadbackPlan`, with `DepthReadbackPlanTest` pinning row stride, written bytes,
row order (`bottom-up`), and depth convention (`gl-window-depth`, never compared
directly to MHW reversed-Z). `WorldCapture` metadata now reports pack row length,
pack alignment, row stride, written bytes, capacity *as capacity*, rows padded, row
order, convention, and the read status/error name.

Verified headless: `bash tools/test-depth-readback.sh` (both drivers),
`bash tools/test-fabric-frame.sh`, Fabric `./gradlew build`. Not verified: any live
capture — that needs both games running. The fail-closed policy (permanent disable
after timeout or setup failure, buffers retained) is unchanged; what changed is that
the refusal now arrives as an exception on a path that checks its own framebuffer
first, so the cause is named instead of inferred.

## 2026-10-06 — the pack state the read inherits, enumerated instead of assumed

The depth read set two pixel-pack parameters (row length, alignment) and left the
rest to whatever the context already held. Two of the untouched ones are silent:
a leftover `GL_PACK_SKIP_ROWS`/`GL_PACK_SKIP_PIXELS` moves the read deeper into the
destination — past the end of a buffer sized exactly to the frame — and a leftover
`GL_PACK_SWAP_BYTES` byte-reverses every float on the way in. Neither raises a GL
error; both destroy every value. The incoming state is now read first, reported as
`packNonDefault=` in the capture metadata, set to explicit values (skip 0, no swap,
no lsb-first, alignment 4, row length = width), and put back in the `finally` block
alongside the framebuffer and pack-buffer bindings. `DepthReadbackPlan.nonDefaultPack`
is the pure, tested half: it names each deviation from the GL defaults, in order,
with its value.

`tools/gl-depth-readback-probe.c` grew a section that starts from a deliberately
hostile pack state (`rowLength=17 alignment=1 skip=5,7 swap=on lsb=on image=3/2`),
then measures both contracts from it: the old row-length-and-alignment-only read
comes back **corrupted** — values wrong and data written past the end of the frame,
which is the regression this test exists to expose — while the guarded read returns
the written values exactly, writes nothing outside the frame, and restores all eight
pack parameters plus both bindings (verified against recorded hostile values and a
dummy FBO/PBO, since restoring zero to zero proves nothing). The two image parameters
are hostile too but deliberately not guarded: if they affected a 2D `glReadPixels`,
the guarded read would fail — so their surviving untouched is the measurement that
they are ignored.

`WorldCapture` metadata additionally records `colourRowOrder=bottom-up` (same GL read
origin as depth, so the two raw buffers are comparable) and `handInFrame=` derived
from the recorded perspective — the first-person hand is drawn *inside* `renderLevel`,
so excluding the HUD does not exclude the hand. The evidence PNG is now flipped to
display order (the probe proved row 0 is the framebuffer's bottom row); the raw
`.rgba` keeps GL order to stay in agreement with the depth buffer.

`tools/verify-world-capture.sh` now archives every run under
`out/archive/<UTC stamp>/`: the previous run's leftovers are copied before the first
request deletes the meta, each publish is copied the moment it lands (only the files
that mode actually wrote), and the run's GL/mod log lines are captured on exit — so
a later failure cannot overwrite an earlier success.

Verified headless: `bash tools/test-depth-readback.sh` (both drivers: NVIDIA RTX 2050
and Mesa AMD Radeon 660M, section D passing on each), `bash tools/test-fabric-frame.sh`
(pack-state assertions included), Fabric `./gradlew build`, `bash -n` on the gate
script. Deployed: `~/.minecraft/mods/crafterhunter-fabric-0.3.0.jar` is byte-identical
to the build (`sha256 9a0c2d53…d4cd9d159`, `cmp` clean); the previous jar is preserved
in `~/.minecraft/crafterhunter/old-mods/`.

Not verified — the live gate itself. One launch attempt (reconstructed from the
SKLauncher logs) brought the game up in-world but *without Fabric*: no
"Loading … with Fabric Loader" line, no CrafterHunter output, frame hook never fired;
the instance exited cleanly and its log is kept at
`/tmp/opencode/my-launch-logs/latest.log`. The launcher's injection evidently only
works under the launcher, so the live capture (colour-only, depth-only, paired, one
block at known distances, camera sync LIVE, screenshots) is handed to the user to run
with both games. Unchanged: the fail-closed retention policy, the shared-memory
transport, and the MHW compositor.

## 2026-10-07 — projection recorded from the camera; the gate blocks on scene depth

The projection metadata was NaN because the old chain read
`GameRenderer.levelProjectionMatrixBuffer.lastUploadedProjection`, a field the level
upload path nulls by calling `getBuffer(Matrix4f)` — structurally dead, so the numbers
were never there to record. `CameraProjectionAccessor` (a new `@Accessor("projection")`
on `Camera`, validated by the mixin AP at build) reaches the matrix the camera actually
holds; `WorldCapture.recordCamera` now writes `fov`, `zNear`, `zFar`, the 16 matrix
floats and `projectionSource=camera-projection`, and both dead accessors plus their
mixins.json entries are deleted. Measured live: `fov=54.37698`, `zNear=0.05`,
`zFar=1024.0`, and a matrix that is exactly the zero-to-one reversed family —
`m22 = n/(f-n) = 4.883051E-5`, `m32 = nf/(f-n) = 0.05000244`, `m23 = -1` (both computed
from the recorded planes to every printed digit). Committed `affc2b6`, pushed.

`tools/validate-world-depth.py` now *detects* the depth convention by trying both
closed forms against the recorded matrix instead of assuming one, and blocks reporting
`worst relative` for each when neither fits; `tools/test-validate-world-depth.py`
(4 fixtures) is the regression that fails on the pre-fix validator at the reversed
case. The live gate then exposed the next gap honestly: on this matrix the validator
BLOCKED with both conventions near `1.0` worst-relative, because the referee
`distance_from_matrix` hardcodes the classic `zw = (ndc+1)/2` window remap while the
measured buffer is `zw = ndc` (ZERO_TO_ONE). For this family the `reversed` closed form
`d = nf/(zw*(f-n) + n)` is algebraically exact — it reconstructs the recorded corner
values correctly — so the referee, not the closed form, is wrong. That fix needs a
fixture carrying the measured matrix, which is the regression test this bug deserves.

The gate run itself (`out/archive/20261006T172918Z/`, both games LIVE, camera ~17
pkt/s) passed everything that does not depend on scene depth: COLOUR, DEPTH and PAIRED
all published with both attachments complete, no GL errors, `packNonDefault=rowLength=949`,
exit 0, per-run archive so nothing overwrote run 1. The HUD was deliberately *on*
during capture — F3 debug, the CrafterHunter overlay and the hotbar are all visible on
screen and all absent from `world-colour.png` (HUD excluded, verified by comparison);
the crosshair and the held item are present in the capture and are recorded as such,
not treated as exclusions. `perspective=FIRST_PERSON`, `handInFrame=included` as
before.

BLOCKED stage, with fresh evidence: **the terrain is not in the depth attachment.**
95.0% of the depth buffer (928,016 / 976,572) is exact clear `0.0`. The only non-zero
content is (a) a hand-shaped rectangle in file rows 0–~320, cols 757–948 — screen
bottom-right under the recorded bottom-up order, `zw` 0.069–0.082 → 0.61–0.72 m, and
(b) a ~10x12 px blob at rows 512–523, cols 466–475, `zw ≈ 0.0498` → 1.00 m — the
crosshair, which writes depth inside the captured pass. The crosshair pixel therefore
reconstructs 1.00 m although the camera sits 3.5666 m above the ground plane and its
view axis must meet ground near 4.6 m, and `depthMax=0.08206147` is identical to the
previous run's — camera-attached geometry, not scene. Colour, meanwhile, is complete
(99.99% non-zero, full terrain). So the colour image is being obtained through a path
that carries the world, and the depth read is against an attachment that only the held
item and the crosshair ever draw into; where the terrain's own depth lives (the
intermediate target its colour is composited from) is the investigation for the next
round, before any transport or compositor change.

Known-distance ground truth was not forced: F3 is bound (`key.debug.overlay:
key.keyboard.f3`) and readable, but the player's ray has no Targeted Block within
reach, and the feet height (`y=128.00000`) is not confirmed at the camera's crosshair
hit point — a plane intersection from `anchor`+`anchorRot`+matrix would be computed
against the same projection under test. The distance claim waits for scene depth and
an independent ground height. Orientation likewise stays open: the validator reports
`depthRowOrder` as the writer's claim because gradient statistics are identical under
both orientations; the colour-based decision is not implemented.

Verified headless: `./gradlew build`, all `tools/test-*` (frame, depth,
frame-selection, world-depth 4/4), `bash -n tools/verify-world-capture.sh`;
`~/.minecraft/mods/crafterhunter-fabric-0.3.0.jar` byte-identical to the build
(`sha256 89743ae88c7fe3820cff8da499e20490df9d900dd92114323ab308513cdf5f0b`), previous
jar preserved in `~/.minecraft/crafterhunter/old-mods/`. Verified live: the archive
above — three modes published, projection recorded, HUD excluded from colour, camera
LIVE throughout. Not verified: scene depth (blocked, evidence above), distances
(validator BLOCKED, referee gap), the convention-vs-matrix match on the real matrix
(needs the measured-matrix fixture), colour/depth orientation as a *measurement*, and
the known-distance check itself.
# 2026-10-08 — bounded native skin upload/binding investigation

The live serialized 64x64 RGBA skin matches the cached user skin exactly after
vanilla SkinTextureDownloader alpha normalization: CRC32 ab329927, 29 distinct
RGBA values. This establishes Java readback/serialization, not GPU sampling.
No production texture bug is established. Fabric, channels, geometry, pose,
anchor and depth behavior are unchanged.

Added opt-in render/player-skin-check.enabled: once per asset, checksum the exact
CPU upload bytes and a staging CopyResource readback (skip RowPitch padding), log
dimensions/DXGI format/pitches/UV bounds, then query actual PS t0 and PlayerPS
identity immediately before drawing. No test texture or image dump. This small
GPU readback may block once; disable the flag after diagnosis. It does not alter
acceptance guards or shader output. GPU equality/binding and live appearance are
NOT yet proven: games were closed and were not launched by the agent.

Passed player model real normal/slim baked tests, Java/native roundtrip, CRC32
known-vector/padded-row regression, player controller/source contracts, all eight
shader entry points, native Release build, world reprojection/alignment,
depth selection, world-frame transport/freshness and cargo workspace (26 tests).
Installed only native DLL, build/install SHA256
9c22b7615c9fd8d6239bff55467a272b979ad7d997b07f825d75a6a495df2360;
previous DLL retained in
/home/yllaris/.local/share/crafterhunter-backups/native-skin-check.GJDQOW.
Diagnostic flag enabled before next launch; no saves/worlds modified.
# 2026-10-08 — diagnostic-only player UV milestone

Inspected actual local 26.2 ModelPart.Cube/Polygon/Cube.compile bytecode: UVs are
normalized by texture dimensions; transforms apply only to XYZ/normals and pass
the baked U/V unchanged. Mirroring/face corner pairing is already baked. Existing
row-zero texture upload/readback and sampling preserve V; framebuffer-origin
rules do not imply a skin flip. No production UV change justified.

Added opt-in UV gradient PlayerUVPS (retains production depth/UI/alpha rejection)
and separate deterministic 64x64 debug skin net, selectable with
tools/control-player-uv.py uv|net|real. Real asset bytes/cache remain untouched.
Each part/face carries independent color channels, blue vertical gradient and
white corner marker. Debug selection does not alter geometry, pose, anchor,
transport, calibration or depth policies. Default installed mode is real.

Passed normal/slim actual baked 12-part/72-face UV rectangle and V-direction
tests, player native/Java roundtrip and debug pattern tests, controller toggles,
all nine shader entry points, native Release build, world reprojection/alignment
and depth selection. Live gradient/net orbit and final skin appearance NOT yet
accepted; games were closed, not agent-launched.
Installed only native DLL SHA256
d4986afc486ac713c6eaa527f0a59d7416d0c72e676f69945ea81fd5d42ee67d;
backup /home/yllaris/.local/share/crafterhunter-backups/player-uv-debug.ZBWs0E.
# 2026-10-08 — host-depth pixel-coordinate investigation, not acceptance

Suspected displaced occlusion after UV/skin gate. Production uses direct integer
SV_Position.xy mip-0 depth Load; player viewport starts at zero with target size,
selection requires matching depth size, direct R32_FLOAT SRV, no copy/resolve or
UV normalization. Current source establishes identity texel mapping, not matching
MHW depth-writer viewport/image. No measured offset, root cause or production fix.

Added opt-in depth-position/depth-source debug shaders and mode controller. They
expose exact normalized sampled texel centres/raw-depth visualization on player
surfaces; host rejection bypass is debug-only, never production acceptance.
Metadata captures target/depth/pre-UI/selected depth-writer viewport plus four
posed-mesh projected bounding-box probes using actual GPU camera constants.
Initial depth-writer measurement can be absent: switch modes after several frames.
No geometry, pose, anchor, skin, transport or production depth changes.

Passed native build, all 11 shader entries, actual normal/slim player tests and
Java/native roundtrip, debug controllers, pixel-centre math across resolutions,
depth selection and reprojection/alignment regression. Runtime measurements and
controlled occlusion acceptance still pending. MHW closed but Minecraft remains
open; scoped installation deferred until requested safe shutdown confirmation.
