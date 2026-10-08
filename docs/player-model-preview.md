# Complete player geometry preview (26.2)

Status: implemented and headless-verified, **not live accepted**. This is a
player-body/skin-layer slice, not a complete playable MVP. Existing frame and
stone compositors are preserved; no world mesh, collision, combat or input
ownership is added.

## Investigation and design

Source of truth: local Loom `minecraft-clientonly-deobf-26.2.jar` and
`minecraft-common-deobf-26.2.jar`, inspected with javap. No decompiled code or
game assets are checked in.

- `EntityRenderDispatcher.getPlayerRenderer` selects normal/slim
  `entity.player.AvatarRenderer` (not the older PlayerRenderer name).
- `AvatarRenderer.extractRenderState` fills `AvatarRenderState`, including the
  current skin, orientation, walk/attack/crouch/flight state and interpolation.
- `LivingEntityRenderer.submit` supplies body rotation, scale, axis conversion,
  sleeping transforms and the actual model submission PoseStack. We invoke it
  with a non-drawing collector that accepts only the main `PlayerModel`.
- `ModelFeatureRenderer` normally calls `Model.setupAnim` before rendering.
  The export does the same, then reads the resulting part transforms. The shared
  model's poses, visibility and skipDraw are restored in finally.
- `PlayerModel` extends `HumanoidModel`: head/hat, body/jacket, arms/sleeves and
  legs/pants. Local normal and slim baked models each have 12 cubes / 72 faces.
- `ModelPart.Cube.polygons` supplies all face vertices and UVs; each quad becomes
  two triangles. `Vertex.worldX/Y/Z` performs Minecraft's model-unit conversion.
  `ModelPart.translateAndRotate` supplies the real hierarchical transforms.
  Visibility propagates through parents; skipDraw suppresses only that part.
- There is reusable **baked local geometry**, but not a single posed player GPU
  mesh exposed by this API. We reuse baked cubes plus their current matrices,
  not a framebuffer and not a reimplementation of Minecraft animation.
- `AvatarRenderState.skin.body().texturePath()` resolves through TextureManager.
  The complete resident OpenGL texture is read with DSA glGetTextureImage once
  per texture-object replacement; pack state/PBO binding are restored. This is
  synchronous tiny-skin readback, not the asynchronous world-depth copy path.

Export is opt-in at LevelRenderer.render HEAD, independent of F5 and frustum
visibility. Only the local player while it owns the render camera is supported.
Invisible/spectator states do not publish; old pose expires. Armor, held items,
cape, extra ears, shoulder animals and arbitrary modded layers are excluded.

## Data and transport

Use the existing local atomic-file data-plane pattern, not UDP video or a bridge
rewrite. `/dev/shm/crafterhunter/player.asset` is replaced only when baked geometry,
part count or the skin texture object changes. `player.pose` is atomic-replaced
at a bounded 4 Hz with no mesh/skin bytes. Both use WorldCapture's generation.
Two files may briefly refer to different assets during replacement; native draw
refuses that mismatch rather than combining them.

All fields little-endian:

| Channel | Header | Payload |
| --- | --- | --- |
| asset | 40 bytes: magic CHPM, version 1, generation u64, asset ID u64, part count u32, vertex count u32, width/height u32 | vertices: XYZ/UV f32 + bone u32 (24 bytes); tightly packed RGBA8 skin |
| pose | 64 bytes: magic CHPP, version 1, generation/asset/sequence u64, feet XYZ f64, part count u32, reserved zero u32 | per part: visible u32, column-major affine 4x4 f32 (68 bytes) |

Bounds: 64 parts, 4096 triangle vertices, skin at most 256x256. Native refuses
malformed lengths, nonfinite data, nonaffine matrices, invalid bone references,
wrong generation/asset/part count, sequence rollback and expired pose. Reuses
the existing 1000 ms receipt Freshness policy, including advancing-producer
startup warmup. Receipt age is NOT end-to-end latency.

Typical vanilla static payload is 432 vertices (10,368 bytes) + 64x64 skin
(16,384 bytes) + header, transferred once. Dynamic payload is 64 + 68*parts
bytes per update. FPS, export time and live cadence are not yet measured.

## Placement and rendering

Matrices already include Minecraft's body orientation and pose in feet-relative
coordinates. Native computes anchor(hostFeetOrigin,guestFeetOrigin) + guest feet
using existing world_alignment.hpp in double precision, then applies scale and
converts metres to MHW GPU centimetres exactly once. F5 is never an anchor change.

The separate opt-in draw uses resident structured vertex/bone buffers, point
filtered RGBA8 skin with alpha cutoff 0.5, current observed MHW GPU VP, the same
validated scene depth and pre-UI signatures. Private reversed-Z depth orders the
player's own triangles. Host depth is read only. No culling of hidden player
faces by Minecraft's capture camera; the MHW camera determines visibility.
Skin is unlit: no host shadows, material lighting, translucent skin blending or
overlay tint claim. Existing renderer/build identity gates remain intact.

## Verification and unresolved risks

`bash tools/test-player-model.sh` checks the actual installed 26.2 normal/slim
baked models: all six face normals per cube, normalized UVs, walking/head pose
changes, and Java/native roundtrip of actual baked triangles/animated matrices.
Its texture fixture is synthetic; it does not prove GL skin readback. Native
tests also cover malformed files, negative-coordinate anchoring, generation
mismatch, stale/rollback refusal and preservation of accepted data on parse error.

`python3 tools/test-player-preview.py` checks isolated controller lifecycle and
source contracts. Shader checks compile all entry points with glslang, not the
in-game D3DCompile/device. Existing frame/reprojection/depth tests remain required.

Unproven: actual mixin invocation/collector at runtime, live skin colors, D3D
player draw submission, back/side visibility under independent orbit, skin-layer
edge quality, host occlusion, walking/crouch/flight in MHW, live performance and
transitions. Texture contents changing in place without replacing the texture
object (e.g. an animated resource pack) are not supported by the skin cache.
Existing alignment has no authoritative guest-world/host-area ID. No new claim
about area transitions, disconnect or collision follows from these tests.

## Runtime submission gate, then manual orbit acceptance

Launch normally with the bridge running and both game windows visible on the
active workspace. Use the existing world; no fixture/world edits required.
Minecraft F7/F9 OFF. F3+P can disable pause-on-focus-loss, but does not fix hidden
workspace throttling. To test the player without the village surface overlay:

```bash
python3 tools/control-paired-preview.py stop
python3 tools/control-player-preview.py start
```

Start Minecraft in first-person and, once paired uploads advance, request once:

```bash
python3 tools/control-player-preview.py align
```

**Do not start orbit acceptance until** fresh logs show player-feet calibration
accepted, `complete player asset uploaded` and `complete player draw submitted`.
The latter is submission only, not proof of visible correct pixels. Export errors
appear in Minecraft latest.log as `player export disabled until restart`.
Renderer log: installed plugin's `render/renderer.log`.

Once submission is proven, switch Minecraft F5 to front view and keep that camera
fixed. If the hunter hides the coincident body, move Minecraft a few steps aside
without recalibrating. Orbit MHW's camera: inspect front, both sides and back;
the back must remain visible despite never entering Minecraft's capture view.
Also check walking/head motion and nearer MHW rocks/hunter occluding the body.
Capture front/back screenshots and fresh log sequences. No F5 re-anchor.

Stop with `python3 tools/control-player-preview.py stop`; if no other preview
needs the shared capture, also run `python3 tools/control-paired-preview.py stop`.
Neither stop command deletes the live asset/frame channels or alters saves.
