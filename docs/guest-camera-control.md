# Minecraft-owned camera slice

2026-10-11: implemented, headless-verified and installed; live acceptance pending.

Matching Fabric, managed and native artifacts installed after the user closed
both games and fresh host process checks confirmed exit. All three match their
tested builds by cmp. Scoped previous artifacts, marker and control requests are
preserved in native/mhw-renderer/build/rollback.guest-camera.mrqb_l57/ with a
manifest. The managed marker was updated; camera control and the input request
start OFF. Existing world/player channels, cadence flags and alignment were
preserved. Reopening and a fresh-session calibration are required for testing.

Keep the existing Fabric/bridge/SPL/native architecture. The existing paired
world anchor maps guest feet to host feet; it is never changed by F5. A small
atomic camera.pose channel carries the actual LevelRenderer camera position,
view and final uploaded projection, independently of GPU readback completion.
World capture and its permanent fail-closed policy are unchanged.

The pinned SPL 1.0.0 assembly exposes camera virtual slots 34/35 and ref fields;
local IL agrees with its source. Read-only loaded-code inspection of the pinned
421810 build establishes uMhCamera vtable 0x143497340, view 0x141fa0cc0 and
projection 0x1404d9810. Both take (camera, output matrix) and return the output.
Hook these getters, forward all other camera instances, and replace only the
main viewport's returned matrices when control is acknowledged and fresh.
Do not patch shader constants after scene rendering or persist camera fields.
Native culling/camera-consumer compatibility is a live acceptance question.
The original view getter also updates native cached bases. We preserve those
side effects, then replace its returned matrix; consumers of camera fields or
cached bases may still use the native pose. Getter application counters include
telemetry/helper calls and do not independently prove the scene used the override.

The shared world anchor converts camera displacement and view, including native
F5 rear/front offsets, collision shortening and view effects. MHW retains its
near/far and depth convention. Replace projection XY terms using Minecraft's
final projection and the actual host aspect; never copy Minecraft depth terms.
Walking bob and hurt motion are present in that final projection, not just in
the camera-relative view. Native factors the final matrix as perspective times
a rigid affine effect, including off-centre perspective, verifies the complete
round trip, and moves the effect into the mapped view. Effect translations are
converted from blocks to centimetres. The host projection then uses the recovered
perspective XY while retaining its native Z/W and near/far. Nonfinite, singular
or unsupported non-rigid/projective shapes still refuse control.
In Minecraft first person suppress the separate full-body preview, as vanilla
does; first-person hands are outside this slice's existing capture boundary.

See CONTRACT.md for the bounded local ownership handshake. Controller start
resolves the current local movement objects, prepares suppression, and publishes
a fresh control token. Fabric releases incoming F7/F9 links while requested;
F6 releases the mode. Native must have a matching advancing camera feed, armed
input filter, current main player/camera, fresh paired upload and valid anchor
before acknowledging. Stop, timeout, generation/anchor/player/camera replacement
release control and disable the owned suppression gate. Recovery requires a new
explicit request; do not revive an expired request or re-anchor on F5.

First runtime gate: one calibration in first person, then walking/look/F5 with
Minecraft focused and both windows visible. Save original screenshots and fresh
request/generation/sequence/epoch/applied-matrix evidence. Verify camera release.
No collision, combat, inventory, scene/menu acceptance expansion or 60fps claim.

After installing all three matching artifacts and loading both offline scenes,
keep Minecraft focused, F7/F9 OFF and first person. Enable the existing previews,
wait for advancing paired uploads, and calibrate once using
`python3 tools/control-player-preview.py align`. Then:

```bash
python3 tools/control-guest-camera.py start --pid <current-MHW-PID>
python3 tools/control-guest-camera.py status
```

Wait for the Minecraft HUD to show `CONTROL: Minecraft ACTIVE`, then walk/look
and switch F5 without re-anchoring. F6 releases the mode; complete explicit cleanup
with `python3 tools/control-guest-camera.py stop`. No game launching or bridge
restart is needed by the controller; UDP kinds are unchanged. Expired control
requires a fresh start, including fresh local input-object resolution.

Passed Fabric build, native Release build and SPL build; guest-camera Java/native
192-byte roundtrip, F5 fixed-anchor/rolled-view/shortened-camera/centimetre/native-Z
and lease/acknowledgement expiry tests; fake-install controller preservation tests.
Omitting the centimetre conversion in a temporary mutant fails the mapping check.
Existing camera/player links, actual normal/slim baked models, world capture hook,
paired transport/reprojection/upload, composition cache, shader compilation,
exclusive-input policy, SPL logic, player/paired controllers and Rust workspace
tests pass. None establishes visible host camera following or live release.

Runtime update: the user confirms camera following and all F5 angles work.
Fresh screenshots/sequence logs cover all three modes at fixed epoch 2422 and
generation 15247686952110. Walking released control: the first sampled rejected
bob projection is sequence 8437 (p[11] = -0.99998944998); the host latched release
after sequence 8436 with matched=0, camera/filter/paired still present, and no
alignment invalidation. Local evidence is in ignored
captures/guest-camera/runtime-20261011-220826/reactivation/.
The old final-projection check required p[11] == -1 within 1e-5 and p[15] == 0;
valid walking effects violate it. The new native-only fix passes the measured
packet and exact clip-XY/W equivalence with unchanged native projection depth
rows. Restoring the old guard fails that regression. Walking continuity after
the fix remains pending safe installation and fresh live testing; a headless
replay accepts all 1699 archived packets. User-observed
camera/F5 following does not accept culling or native depth compatibility.

Primary loader source: [Viewport](https://github.com/Fexty12573/SharpPluginLoader/blob/master/SharpPluginLoader.Core/View/Viewport.cs)
and [hook lifecycle](https://github.com/Fexty12573/SharpPluginLoader/blob/master/docs/Development/FunctionHooking.md).
