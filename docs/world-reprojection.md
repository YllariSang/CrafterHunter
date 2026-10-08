# World-depth reconstruction and reprojection

Implemented CPU reference math, headless-tested against archived real captures;
not a GPU compositor or a live alignment acceptance claim.

`native/mhw-renderer/world_reprojection.hpp` provides column-major inversion,
raw bottom-up pixel-centre unprojection into guest eye space (forward is -Z),
explicit guest-eye-to-host-world transformation and projection through a current
host view-projection, and conservative reversed-Z host-space occlusion.
Clear, nonfinite, invalid, singular, behind-camera and out-of-frustum inputs refuse
without replacing outputs. Coplanar depths are skipped, not forced in front.

The caller MUST supply the measured window-depth mapping (zero-to-one or classic
minus-one-to-one), clear value, verified eye-to-host matrix and same-draw host VP.
The matrix alone does not identify GL clip-control state. Recorded bedrock captures
used reversed-Z zero-to-one with clear 0; do not infer that for every future backend.
No Minecraft linear distance or raw window depth is compared directly with MHW
depth. Only the point projected into HOST clip space yields a comparable depth.
Host UV is top-down; guest input Y remains its raw bottom-up row. Pixel centres
use x+0.5 and y+0.5, not integer pixel edges.

The upload-only native path now prepares and retains the inverse projection for
the same staged pair, rejecting singular projections before allocating textures.
It still binds no guest textures. These CPU helpers are a reference for a later
GPU implementation, not a per-pixel CPU rendering loop.

Checks:

```bash
bash tools/test-world-reprojection.sh
# Optional read-only replay of local, already archived acceptance captures:
bash tools/test-world-reprojection.sh \
  /home/yllaris/.minecraft/crafterhunter/out/archive/20261007T132237Z/PAIRED \
  /home/yllaris/.minecraft/crafterhunter/out/archive/20261007T132722Z/PAIRED
```

Replay checks raw attachment hashes and the independently known bedrock face
(917,-60,348), including that the camera ray intersects that face, rather than
using depth to invent its expected distance. Native reconstruction yields
4.894803 and 12.121316 blocks versus 4.895355 and 12.122026 from pose/face geometry.
Unit checks additionally expose row flips, pixel edges, off-axis reconstruction,
explicit window mapping, translation/rotation, clipping and invalid depth.
Archives are not bundled, modified or regenerated.

## Remaining prerequisite before drawing

### [AGENT] Explicit shared-world alignment reference (2026-10-08)

`world_alignment.hpp` now defines the CPU anchor contract:
`hostMetres = hostOrigin + metresPerBlock * (guestBlocks - guestOrigin)`.
XYZ directions match the existing CameraLink/PlayerLink relative mapping; the
default is one block per metre. Raw MHW centimetres must be converted by the
caller before supplying hostOrigin. This is a reference contract, not a measured
live calibration. No rotation is fitted from independent screenshots.

An anchor must be explicitly armed with nonzero guest generation and host scene
epoch. A generation/epoch mismatch or reset refuses conversion without replacing
the output. The caller must invalidate on disconnect/loading/scene change; this
helper does not discover those events. Snapshot v2 has guest generation but does
NOT yet carry a shared host epoch/calibration acknowledgement. The later opt-in
paired preview now uses this anchor at runtime; camera ownership remains missing.
Neither live shared alignment nor paired drawing is accepted yet.

The eye transform composes that fixed anchor with inverse(actual level view)
and exact render-camera position. F5 never recalibrates the anchor. Tests map
one unchanged block through first-person, displaced rear and reversed/displaced
front cameras and require identical host coordinates. Translation, scale,
round-trip inversion and stale/reset refusal are also checked by the existing
`test-world-reprojection.sh` entry point.

User archives 20261007T215619Z/215700Z/215730Z provide fresh capture evidence:
identities 3/6/9, generation 700129956865, three correct F5 modes, hash-matched
attachments, finite depth and colour-aligned bottom-up rows. Rear/front cameras
are 4.0000002 blocks from first-person, in opposite directions; front view flips
orientation. All used pre-hand-clear fallback, zero-to-one reversed-Z, LOWER_LEFT.
This does not prove wall collision, new known-block distances or host alignment.

Update: native `align.request` now explicitly calibrates the fresh uploaded
first-person camera to the current host camera (metres), and logs subsequent
mapped camera positions with generation, identity, mode and local epoch. Managed
`CH_AlignmentHost` supplies host camera coordinates after centimetre conversion.
No guest control acknowledgement or full shared protocol epoch is implemented.
This is a host-local, log-only calibration diagnostic, not visual acceptance.
Its one-second upload watchdog requires advancing paired captures; the existing
manual three-mode capture script is not a continuous diagnostic feed.

Calibration requests use atomic publication (`control-mhw-renderer.py align`)
and native rename-to-claim consumption. They refuse without a fresh upload or
in third-person. A host jump over 25 m, gap over one second, clock rollback,
invalid pose, upload expiry/refusal/disable or guest generation mismatch drops
calibration. F5 changes alone preserve it. A crash-left `align.claimed` blocks
requests rather than replaying a calibration; inspect/remove that specific file
only with the game closed. Title detection is NOT proven: calibrate only in a
loaded world. These are continuity heuristics, not an authoritative scene ID.

Update: bounded 4 Hz streaming and actual paired GPU surface preview are now
implemented. Stop adding diagnostics; see docs/playable-steve-mvp.md.
Next **[MANUAL]** after scoped installation: verify shared block position
under movement/F5 and a collision-shortened third-person camera, plus scene-change
invalidation. Current CPU tests are not a reason to ask for another identical
capture run or mark alignment accepted.

The user explicitly requires Minecraft's native F5 first-person, rear third-person
and front-facing third-person cameras. All three must drive the matching host
view in Minecraft-owned mode. Third-person includes camera displacement/collision
and Steve's normal guest model visibility; first-person hands need their own later
layer. These are acceptance requirements, NOT implemented gameplay features.

Historical snapshot v1 contained pose angles, NOT the complete issue-time view transform,
view effects, clip-control provenance or shared-world anchor/session mapping.
Do not derive an exact view matrix from player yaw/pitch and assume it includes
camera bobbing, third-person displacement and scripted camera effects. Snapshot v2
now observes the actual level camera/view arguments and final effect-modified
projection upload, with queried GL clip mapping/origin and depth range. It is
validated for three F5 capture modes by the user archives above, not for camera
collision or shared-world alignment. The opt-in preview now exercises GPU
reprojection through the validated pre-UI/depth pass; runtime acceptance remains.
Retain freshness gating and guest clear-pixel exclusion; reprojection alone
does not solve disocclusion holes, transparent pixels, or isolate Steve from terrain.
