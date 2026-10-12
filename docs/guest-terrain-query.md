# Guest terrain-query slice

User authorized terrain collision work after accepting the F5 guard on 2026-10-12.
The first slice connects the existing host segment adapter to the real guest UDP
endpoint. It does not yet constrain movement, insert barrier blocks, or export
collision meshes. MHW's adapter already has live ground/sloped-architecture
query evidence; see terrain-query.md. Its normals are not reliably directional,
so they must not be treated as validated walkable-slope normals.

The existing two-process route and installed games remain required. Minecraft
keeps movement/building authority; host segment answers are observations only.
Before driving collision, measure floor/stair/wall segments at coordinates mapped
through the renderer's fixed XYZ anchor, then choose a representation that also
works on Minecraft's integrated-server simulation. A single down-ray cannot
represent walls, overhangs or the player's swept volume. No save edits are needed
for this query slice.

## Channel and lifecycle

`tools/query-mhw-terrain.py --start X Y Z --end X Y Z` takes **MHW metres**,
not raw centimetres or Minecraft coordinates. It writes an atomic
`/dev/shm/crafterhunter/terrain.request`:

`positive-nonce expiry-unix-ms start.x start.y start.z end.x end.y end.z`

The guest UDP worker claims by same-directory atomic rename before reading. It
polls at most 20 Hz (ground-contact follow-up), allows one outstanding request, limits input to 512 bytes,
finite endpoints within 100000 metres and a segment length in (0,64] metres.
Expiry is at most five seconds ahead on the same Linux wall clock; request
round-trip timeout uses the guest's monotonic clock (one second). No cross-game
clock subtraction. The tool serializes cooperating writers with flock and waits
for its matching nonce; a stale reply cannot answer a new request.

Wire kinds 12/13 and their existing 28/36-byte layouts are unchanged. A random
initial 32-bit wire ID advances for each query, independent of the local nonce.
The request goes through Minecraft's existing connection, preserving its bridge
registration. The host retains its two-ray/50ms budget and bounded queues.
Answers must have MHW source, zero packet session, matching request ID, exact
length, valid status, zero reserved bytes and finite geometry. Non-hit geometry
must be zero. Late and unrelated results cannot replace the current result.

Atomic `terrain.reply` starts with the request nonce, followed by `hit` and
position/normal/attribute, or `miss`, `unavailable`, `timeout`, `disconnected`,
`invalid`. These results are diagnostic records, never cached colliders. Lost
answers time out without a retry or guessed floor. A reconnect retires the
pending request; commands expire rather than unexpectedly firing after restart.
F6 and render/capture channels are untouched; an explicitly requested diagnostic
ray is allowed with camera ownership either on or off.

## Checks and remaining evidence

`python3 tools/test-terrain-query.py` checks Rust/C# golden bytes, one pending
request, atomic claim, unrelated/reserved/late refusal, explicit miss/unavailable,
hit geometry, disconnect and expired command. Production endpoint send/receive
wiring is checked: the original guest had neither path. `cargo test --workspace`
and the Fabric build pass. Real guest-to-host query results still require a fresh
Minecraft restart and a loaded offline MHW scene. No floor/wall collision or
loading-screen acceptance is claimed.


Verified guest jar installed after control/filter release and PID-verified SIGTERM
of only MHW 376733 and Minecraft 376648; both exited. Scoped backup:
`native/mhw-renderer/build/rollback.guest-terrain.nncspc4o/`. All three installed
artifacts cmp-match their builds; native/managed and seven enabled flags preserved.
Control OFF; no launches. Next manual reopen into loaded offline scenes, then
explicit host-coordinate floor/wall queries. No live guest-query acceptance yet.

## 2026-10-12 live guest query evidence

Fresh MC 503565 / MHW 503751, all installed artifacts cmp-clean and loaded native
cmp-clean. Eight explicit queries through the real guest endpoint produced six
hits and two misses, each with its own matching nonce; host frame counters
advanced throughout. Three floor columns (hunter X and +/-1m) returned
Y=-6.3402185m, matching hunter feet within float serialization precision.
Horizontal west/south/north rays hit at 2.5335/4.6673/3.8022m; east and the short
above-head segment missed. All returned hit positions lie within the requested
segments (off-segment error <0.000001m). Original before/after screenshots and
logs: captures/guest-terrain/runtime-20261012-124613Z/.

This accepts stationary live guest query transport and hit/miss decoding, not
Steve terrain collision, exact wall identity, scene-change handling or latency.
Control stayed off; no calibration, movement, save writes or live channel deletion.

New source finding: TerrainAdapter.TryCast divides both hit positions AND normals
by UnitsPerMetre=100. All six observed normal lengths are approximately 0.01;
the corresponding pre-conversion lengths are approximately one. Earlier wording
that these normals are inherently unusable was premature: the adapter itself
scales them. Correct this dimensionless-vector conversion with a regression
before relying on normals for slope decisions; raw engine validity still needs
guards. Next physics integration must also map through the fixed renderer anchor
and apply collision consistently to Minecraft client and integrated server.
