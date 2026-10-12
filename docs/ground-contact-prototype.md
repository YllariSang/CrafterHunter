# Ground contact: normal correction and flat-patch prototype

The user authorized this slice after eight live guest terrain queries on
2026-10-12. Scope is a corrected host measurement and a headless prototype using
the actual installed Minecraft collision solver. **There is no runtime movement
hook yet.** No invisible blocks or changes to the user's world/save.

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
