# Ground contact: normal correction and flat-patch prototype

The user authorized this slice after eight live guest terrain queries on
2026-10-12. Scope is a corrected host measurement and a headless prototype using
the actual installed Minecraft collision solver. The initial slice had no runtime
movement hook; the opt-in wiring below now implements it, pending live acceptance.
No invisible blocks or changes to the user's world/save.

TerrainAdapter previously divided the normal by 100 along with the hit position.
`TerrainRay.TryConvertHit` now converts only position, preserves the raw finite
normal (including zero), and refuses nonfinite geometry. It does not normalize
an unknown engine vector into a fabricated walkable direction. The host tests
check flat/sloped/zero normals, metres and nonfinite refusal. Restoring the old
normal divisor violates the unit-normal assertions.

Local 26.2 Entity bytecode confirms `collideBoundingBox` collects collider shapes
and resolves movement using them. The prototype constructs a small flat patch
from five matched, fresh, upward unit-normal hits: centre and four corners of a
0.5m square. Heights must agree within 1cm. It maps with the existing one-metre
per block XYZ anchor and refuses mismatched generation/epoch/token, missing or
500ms-old samples, future receipts, nonfinite data and sloped/nonunit normals.
This is a sampled approximation: five rays cannot prove continuous geometry
between them. Do not expand it to an infinite floor or infer walls from it.

`./gradlew testGroundContact` exercises actual Minecraft `Shapes.collide` with
the mapped finite shape: downward movement lands on its top, upward movement
remains free, movement outside its extent falls normally. Empty-shape control
falls through, exposing a missing-collider implementation. Identity/expiry,
missing hits, old 0.01-scaled normals, steps and slopes are refused. It proves
solver compatibility, not actual player movement or mixin application.

Next after restart: repeat known ground/horizontal queries, verifying unchanged
hit positions and unit-length normals. Before activating ground contact, connect
an immutable short-lived patch to both local-player and integrated-server
collision collection, tied to the renderer's actual anchor and active ownership.
Invalidate on F6, stale queries, anchor/session changes or missing host terrain.
Do not snap position each frame, override flying/jump input, or replace Minecraft
block collision. Test flat contact first in a suitable empty test area; existing
Minecraft terrain can mask whether a host collider works. Walls, steps, slopes,
fall damage and collision with guest builds remain later acceptance gates.

Verified managed correction and guest prototype installed after release and
PID-verified closure of MHW 503751 / MC 503565. Scoped backup:
`native/mhw-renderer/build/rollback.ground-normal.7d2x5xzl/`. All three artifacts
cmp-clean; native renderer and all seven enabled flags preserved. Control OFF,
games closed, no launches. Next manual restart and ground/horizontal normal
comparison; runtime ground-contact wiring is still pending.

## Corrected-normal runtime check (2026-10-12)

Fresh loaded session evidence: captures/guest-terrain/runtime-20261012-125913Z/.
All three installed artifacts and loaded native renderer cmp-match. The same
8 segments as runtime-20261012-124613Z return identical statuses (6 hits, 2 misses),
hit positions and attributes. All 6 normal lengths are within 0.000001 of one;
the 3 floor normals are exactly (0,1,0). Host frames advance during every query.
Original before/after screenshots and matching-nonce replies are preserved.
This accepts the unit conversion in the live guest/host query path. No slopes,
stairs, scene transitions, physical Steve contact or latency acceptance. Games
remain open, control OFF. Next connect the prototype to both client/server
collision using matched fresh queries and the actual renderer anchor.

## Opt-in live flat-patch wiring

The subsequent authorized slice adds `GroundCollisionMixin` to Entity's exact
`collectCollidersIgnoringWorldBorder(Entity,Level,List,AABB)` helper, verified in
local 26.2 bytecode. Both local and integrated-server Entity movement collect
through this helper. Vanilla block/entity/world-border shapes remain present;
one ephemeral patch is appended for the same player UUID and exact client/server
Level instances. The worker never changes position, velocity, onGround, flight,
blocks or saves. Creative single-player only for this initial test (mayfly must
be true, flying must be false to collide); survival/remote multiplayer excluded.

`tools/control-ground-contact.py --pid PID --seconds 60 --output CAPTURE_DIR`
is an explicit bounded sampler, not an autostart service. It requires advancing
ACTIVE acknowledgements and reads only our own cmp-matching loaded renderer's
anchor symbol. No addresses persist. A coherent fixed anchor is checked before
and after every ray and while waiting. Centre and four corners stay fixed at the
anchor's X/Z; the patch never chases the player. It uses the existing guest query
endpoint with one outstanding request and the cooperating writer lock. Requests
expire; a missing/miss/steep/nonunit/off-segment/nonflat result stops sampling.
Five-ray ages must remain below 500ms; slow sampling refuses instead of relaxing
freshness. After the first complete sweep, each new ray refreshes the rolling
five-hit set. The script has a 120-second maximum and removes its patch on exit.

`ground.patch` has 47 whitespace fields: version 1, increasing sequence, Linux
publication Unix milliseconds, token/generation/epoch, guest XYZ and host XYZ
origins, then five (hit XYZ, normal XYZ, query-age-ms) records. Query age includes
the whole local request round trip, conservatively preceding native execution.
The guest adds local publication age and converts the remaining lifetime to its
own monotonic clock. Duplicate files cannot extend expiry. No Windows/Java clock
subtraction. The guest's explicit-request poll is now 20Hz and its UDP receive
wait is bounded to 20ms; host two-ray/50ms and render/capture scheduling stay as-is.

GroundRuntime reads files only on the client tick and publishes an immutable
binding for the two collision threads. It clamps lifetime to patch freshness,
advancing camera acknowledgement freshness and a 100ms client-tick heartbeat.
F6/token change clears it immediately. A client-world replacement revokes that
token until explicit new control. Missing files, pause, invalid data or unavailable
integrated-server world removes the binding. No collision-path I/O or client
singleton access on the server. HUD shows GROUND: test patch/OFF; bounded log
reports include sequence, anchor epoch and separate client/server collider counts.

Build and existing terrain/camera/controller tests pass. Channel tests reject
duplicate renewal, stale initial data, future publication, wrong ownership and
extra fields, in addition to actual vanilla landing/jump/edge tests. Compilation
and helper inspection do not prove live mixin application: after restart, collect
both collider counters and actual landing/F6 evidence before acceptance.

First manual test: use Creative single-player, fly vertically about three blocks
above clear Minecraft ground, then remain still in first person with F7/F9 OFF.
Calibrate once there so MHW floor maps above Minecraft ground. Arm control and
start the sampler; fly up about one more block without moving horizontally, stop
flying and fall onto the fixed patch. Jump, then F6: patch disappears and normal
fall reaches the lower Minecraft floor. Do not remove blocks or recalibrate on
F5. This distinguishes host contact from the original ground; test side edges
only after centred landing succeeds. A 0.5m sampled square is an approximation,
not general terrain collision or continuous mesh extraction.

Verified guest runtime build installed after control release and PID-verified
closure of MHW 572057 and MC 571596. Scoped backup/manifest:
`native/mhw-renderer/build/rollback.ground-runtime.2_fyymao/`. All three artifacts
cmp-clean; native/managed and seven enabled flags preserved. Games closed,
control OFF, no launches; ground sampler has not run. Next manual restart and
centred Creative landing/jump/F6 with client/server counters and screenshots.

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

## 2026-10-12 — Reduce repeated guest request-pickup delay

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

Verified guest-only installation complete after PID-verified SIGTERM of MHW720643
and Minecraft720188, both exited. Backup/manifest:
`native/mhw-renderer/build/rollback.ground-query-polling.h2co2q0j/`. All three
artifacts cmp-clean; native/managed and seven flags preserved. Games closed,
control OFF, no launches. Next manual reopen into loaded offline scenes, Creative
hover about3 blocks above clear ground, first person/F7/F9 OFF. Then compare
pickup timing, calibrate once for the new session and collect centred landing,
jump, both collision counters and F6 with a fresh sampler. No runtime speed or
landing acceptance claim yet.

## 2026-10-12 — Fixed flat-ground contact accepted live

User reports the landing/jump/F6 test worked flawlessly. Fresh original screenshots,
879 pose/control samples and client/server counters independently support this
bounded pass. Evidence: captures/ground-contact/runtime-20261012-142354Z/acceptance.json.
Generation12014104535856, fixed epoch1831, token9116850822019451573. Sampler
published1586 patches and stopped when F6 removed the guest control flag; no
five-ray-age failure. X/Z stayed fixed. Falling feet reached patch top
Y=-56.759861197421145 (41 matching samples in the first landing window), jumped
to sampled peak -55.50838, then landed again. After matching release sequence4513,
feet fell to Minecraft ground Y=-60, 3.24014 blocks lower. Final logged collider
counts client138/server139; original ground-080/085 show ACTIVE/test patch,
ground-090 shows preview/GROUND OFF. Explicit cleanup confirms armed=0;
control flag/patch absent, games remain open.

This accepts centred Creative landing, jump/relanding and F6 removal on the fixed
0.5m square sampled flat patch. It does not accept walking coverage, edges, slopes,
stairs, walls, survival or general MHW collision. Preserve this checkpoint when
extending coverage; next bounded development is adjacent flat-floor support.

The preceding60-query OFF probe observed median pickup20.31ms vs38.47ms before,
round trip65.69ms vs78.15ms, all hits; largest five-query sum457.28ms. Evidence:
captures/ground-contact/query-timing-20261012-142356Z/. Cross-session file timing,
not controlled causal speed attribution or exact host stage timing.

## Two-cell flat strip (implementation; live acceptance pending)

The next bounded extension uses two adjacent 0.5m squares along fixed +X. The
first is the accepted anchor-centred cell; the second is centred 0.5m east.
Bounds relative to guest anchor: X[-0.25,+0.75], Z[-0.25,+0.25]. They do not follow
Steve or recalibrate on F5. Each cell retains centre + four-corner validation,
sharing two seam corners (eight distinct rays). All eight must be fresh, upward,
unit-normal, on-segment hits agreeing in height within1cm. Both cells use their
common minimum height and an exact shared seam after validating float-coordinate
rounding. Missing/nonflat/expired data refuses the whole strip; no extrapolation.
This is a sampled finite approximation, not continuous terrain extraction.

`tools/control-ground-contact.py --pid PID --seconds 120 --output DIR --strip`
opts into eight-ray batches. Without --strip the accepted single-cell sampler
is unchanged. One complete batch is in flight at a time; every sample age starts
at local batch issue, conservatively before host execution. Query response may
take up to1s, but publication/guest admission still require every sample <500ms.
Previous geometry also expires during a slow subsequent batch; expiry is never
renewed by rereading it. Reply loss, a miss, nonflatness, anchor/lease change or
F6 ends sampling and removes the patch. A fresh batch is not claimed to be a
single simultaneous host frame. No host ray budget or native rendering change.

Local ground.patch version2 has the same12-field header as version1 followed by
eight7-field hit/age records (68 fields total). Hit order: original centre, four
original corners, adjacent centre, two outer corners. Version1 remains readable.
Frame stores an immutable one/two-cell list; GroundRuntime builds their union for
the same exact UUID and client/server levels. Shape expiry, tick/ack heartbeat,
Creative/not-flying gating, first-person/F5 behavior and F6 clearing are unchanged.
Logs include cells=1/2. The fixed strip is intentionally small: tests should cross
the shared seam to X=anchor+0.65, jump/reland, then release. A0.6m-wide player at
that centre has no overlap with the original cell, proving adjacent support.
Do not walk beyond X=anchor+0.75 during the first check or move sideways in Z.

Headless tests exercise the actual Minecraft solver across the seam, upward jump,
fall beyond the outer edge, original-cell-only refusal at the far test point,
missing/stale/nonflat corner rejection and version2/legacy channel bounds. Batch
reply admission tests reject partial, duplicate, out-of-order-index, stale,
wrong-nonce, nonunit and off-segment geometry. Guest endpoint tests cover bounded
batch admission, out-of-order network replies reconstructed in input order,
duplicate/foreign refusal, no partial publication, timeout/disconnect and complete
validation before sending. Build success does not establish live strip support.

Verified guest-only strip build installed after both games exited. Backup:
`native/mhw-renderer/build/rollback.ground-strip.uvpa2ya5/`. All3 cmp-clean;
native/managed/seven flags preserved. Games closed/control OFF. Next manual
restart and batch timing/centred landing/+X seam crossing/jump/F6 evidence;
no live strip acceptance yet.
