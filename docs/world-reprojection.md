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

The user explicitly requires Minecraft's native F5 first-person, rear third-person
and front-facing third-person cameras. All three must drive the matching host
view in Minecraft-owned mode. Third-person includes camera displacement/collision
and Steve's normal guest model visibility; first-person hands need their own later
layer. These are acceptance requirements, NOT implemented gameplay features.

Snapshot v1 contains pose angles, NOT the complete issue-time view transform,
view effects, clip-control provenance or shared-world anchor/session mapping.
Do not derive an exact view matrix from player yaw/pitch and assume it includes
camera bobbing, third-person displacement and scripted camera effects. Next obtain
the actual issue-time guest view transform and explicit depth convention, version
the paired transport, and validate anchor conversion against current host camera
evidence. Only then wire GPU reprojection to the existing validated pre-UI/depth
pass. Retain freshness gating and guest clear-pixel exclusion; reprojection alone
does not solve disocclusion holes, transparent pixels, or isolate Steve from terrain.
