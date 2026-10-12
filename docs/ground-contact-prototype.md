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
