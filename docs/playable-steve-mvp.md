# Playable Steve: audit and first composition slice

## Scope / evidence (2026-10-08)

The MVP is real Minecraft-owned Steve in one supported offline MHW area, native
F5 cameras, depth-correct rendering, movement on tested MHW terrain, recoverable
transitions/restarts and measured performance. Combat, inventory exchange,
crafting, TNT, quests and story are excluded. This document is NOT acceptance.

Architecture remains Fabric + Rust localhost UDP router + SPL managed adapter +
native DX11 renderer above DXVK. UDP carries camera/player/terrain/assets; paired
world pixels use an atomic local file. Rust routes packets, but does not negotiate
input ownership: protocol session is still zero. Existing host player proxy is
MHW -> Minecraft, not Minecraft -> MHW. Disabling F7/F9 releases guest control;
it does not suppress host input or drive MHW's camera from the guest.

Code and recorded evidence establish camera/asset transport, native stone drawing
with scene-depth selection and pre-UI ordering, real paired Minecraft captures,
F5 render-camera metadata, and host terrain ray queries. The ray adapter checks
the executable hash/seven signatures and bounds request queues. Fabric has no
terrain-result collision consumer. No player mesh export exists. Sky-only colour
composition is not Steve isolation or depth-correct composition.

Tests establish protocol shapes/finite inputs, routing/error handling, bounded
freshness/restart selection, paired identity/bytes, capture completion, projection
math and source hook ordering. They do not establish live movement/collision or
frame presentation. Older measurements: 958 stone frames had same-frame selected
depth and 0.000 px CPU/GPU projection difference; 28.9 average fps, 37.7 focused.
Terrain ray heights agreed with hunter feet to at most 11 cm on five tested
surfaces. Those are NOT paired rendering performance or end-to-end latency.

Biggest risks: real paired rendering/latency at usable quality and cadence; guest
terrain visibility versus isolating Steve; guest-authoritative host camera/input
handoff; feeding host collision without uncertain native writes. No current
measurement proves the requested playable MVP is feasible at acceptable frame time.
Architecture documentation has historical stale milestone labels; treat measured
MODLOG entries and implementation as the evidence, not those labels.

## Smallest implemented rendering slice

Hypothesis: the existing paired textures and validated pre-UI pass suffice to
render guest surfaces at fixed mapped world positions and test them against real
MHW depth, without replacing working rendering infrastructure.

`paired-compose.enabled` selects the new GPU reprojection draw instead of the old
sky-only colour draw. `world-upload.enabled` remains required. `align.request`
explicitly maps fresh capture-matched Minecraft player feet to MHW player feet;
F5 does not change this anchor. Shader unprojects real depth with its issue-time
inverse projection, transforms through exact captured view/position and the fixed
anchor, converts host metres to GPU centimetres, and projects with the same draw's
observed host GPU camera buffer. Clear/invalid depth and unsupported upper-left
clip origin refuse. MHW scene-depth selection remains unchanged. A private D32
target handles guest self-occlusion; MHW's own depth is never overwritten.

This preview samples one source point per 4x4 texels and reprojects each sample's
four-source-pixel footprint at its own depth (six vertices). Equal-depth cells
share edges across resolution and view-scale changes; different samples are not
connected into a mesh. The sampling is an explicit bounded spike budget, NOT a
claim of complete geometry or correct reconstruction between samples. At 949x1028
it submits 366,996 vertices per composed frame. Holes/disocclusion and size changes
are expected at depth discontinuities and missing views. Corner projection adds
shader work; runtime performance of this change is not yet measured.
Guest terrain is included, not removed; no replacement model/assets.
First-person hands are excluded by the existing capture boundary. No lighting
integration, alpha-correct transparency or 60 fps claim.

Player anchoring requires snapshot v3 and the matching managed/native builds.
The actual render camera remains separate: its eye height and F5 displacement
survive the existing transform. Camera continuity still owns lifecycle guards;
missing host player data invalidates calibration. No guessed eye-height offset.
This correction is built/headless-tested and installed, not visually accepted.
The hunter remains rendered and can legitimately occlude a coincident Steve;
after calibration, move Steve slightly sideways in Minecraft to distinguish them.
F5 front captures the opposite direction: uncaptured terrain cannot be reconstructed
by this single-view preview. Player anchoring does not resolve that limitation.

Fabric `world-stream.enabled` supplies same-frame pairs at a requested maximum
4 Hz, one copy pair in flight, matching native 250ms polling. Streaming skips
diagnostic PNG/raw artifact writes; prior diagnostic artifacts/meta remain intact.
It retains the permanent fail-closed GPU lifetime policy. Remove the flag to stop;
no channel deletion. Native upload watchdog still expires after one second and
drops calibration, requiring explicit recalibration after recovery. Host epoch
uses observed jump/gap continuity, not an authoritative area ID/title detector.

## The full-resolution path (2026-10-11)

The spike asks for a full Minecraft frame at native resolution and 60 fps with
the player visible. Two of those three were already true in different places and
neither had both:

- `drawFrame` composites the full captured frame at native resolution, but as a
  letterboxed overlay: it replaces sky, it does not place guest geometry in the
  world.
- `drawPaired` places guest geometry correctly - real depth, host-camera
  reprojection, host occlusion - but sampled one point per 4x4 cell, which the
  previous checkpoint recorded as an explicit bounded budget.

`drawPairedFull` keeps the same gates, alignment, constants, private D32 target
and host-depth fragment test. The original full path submitted one point per
captured texel. That path was accepted for native-resolution colour/player/F5
appearance, but left regular coverage gaps when a texel projected larger than
one host pixel.

The stationary 2026-10-11 baseline measured 949x509 guest -> 1920x1080 host:
a texel spans 2.1218076 host pixels per axis. GPU camera X/Y/W matches the CPU
controlled camera exactly in three traced pre-UI draws. Selected depth 0 has
full-size continuous geometry in the inspected patch. The artifacts span frames;
this is not same-frame occlusion or all-F5 acceptance. See the latest MODLOG.

The new full path submits a **source-pixel footprint per texel**, using six
vertices and the texel's own depth for all four corners. It never connects
neighbouring depth samples into a mesh. At 949x509 this is 2,898,246 vertices,
six times the original 483,041-point budget. Equal-view magnification now has
continuous coverage in the headless perspective regression; real disocclusion,
uncaptured surfaces, different-view depth edges and native camera-fade dither
remain limitations. More shader work may reduce host frame rate; runtime cost
and visual coverage of this change remain unaccepted.

2026-10-12 correction: centre-depth quads introduce artificial horizontal bands
when a magnified sloped guest plane nearly coincides with native ground. The
archived 128x96 ground patch's constant-depth replay reproduces every observed
guest/host pixel; plane-corrected depths remove the bands while leaving native
fade dithering. This is a stationary replay, not new GPU acceptance.

The full shader now estimates a source-window-depth gradient only when all eight
neighbours in a valid 3x3 neighbourhood agree with a plane within roughly eight
float ULPs (tolerance max(1e-9, abs(depth)*1e-6)). It evaluates that plane at each
source-pixel corner before applying the existing inverse projection and host
mapping. Borders, invalid neighbours, depth discontinuities and nonplanar patches
retain the centre-depth footprint. No neighbour vertices/colours are joined into
a mesh. The host-depth fragment test and private depth target remain unchanged;
native camera-fade silhouettes are still respected. More VS texture reads add
unmeasured GPU work, with the same six-vertex budget. The production HLSL helper
is executed through test-only C++ vector/texture shims by
`python3 tools/test-paired-plane.py`; its flat-depth mutant fails the numerical
occlusion regression. Runtime ground/F5 appearance remains pending installation.

Stream/upload cadence is unchanged and controlled by the existing interval flags.
The periodic log reports footprint vertex count, host-local upload age and draw
count; these do not measure end-to-end latency or prove 60 fps.

Opt in with `paired-full.enabled` (`python3 tools/control-paired-preview.py
start-full`), which takes precedence over the preview flag. The stride-4 preview,
frame, stone and player paths remain unchanged. Full-footprint acceptance needs
fresh native-resolution screenshots/log identities, MHW foreground occlusion
and all three F5 modes with a fixed anchor after installation.

## Next [MANUAL]: only this slice's acceptance

Update 2026-10-08: the composition binaries are installed. Live logs now establish
paired uploads, stable calibration and an actual paired draw submission, NOT
visible pixels or visual acceptance. Generation 1113423980880, epoch 1372:
calibration accepted at host tick 1280720; depth 0 selected with 56.56% coverage;
paired draw submitted at 1281907. Calibration remained armed through tick
1366306 (over 85 seconds). Producer intervals settled near 255 ms.

**Keep both game windows visible on the active workspace during this preview.**
In the local Hyprland/Xwayland environment, leaving both games on workspace 2
while the terminal was active on workspace 1 produced consecutive captures about
1000 ms apart (not merely skipped consumer samples). Returning to workspace 2
restored approximately 4 Hz capture and allowed depth measurement/draw submission.
F3+P disables focus pausing; it does not guarantee rendering of an off-workspace
window. The exact compositor/driver throttling mechanism is not established.
Do not increase watchdogs to hide this setup limitation. Moving away may invalidate
calibration; return, wait for advancing pairs, then explicitly align again.

Run the bridge and enter loaded worlds, offline/private MHW. Keep both windows
rendering; guest focus pausing must be disabled. Disable F7 camera and F9 player
proxy so Minecraft input is not overwritten. Start in Minecraft first-person:

```bash
python3 tools/control-paired-preview.py start
# Wait for advancing paired upload identities in MHW render/renderer.log.
python3 tools/control-paired-preview.py align
# After calibration is accepted, use F5 rear/front mode to see the guest player.
# Escape hatch (does not delete any shared frame channel):
python3 tools/control-paired-preview.py stop
```

Acceptance for this spike: real guest surfaces/player visible after F5, fixed
world placement under host camera motion, foreground MHW geometry hides samples,
HUD/cursor above them, no draw on stale/missing upload. Capture screenshots with
fresh upload/calibration identities and record focused FPS before/after. Moving
in Minecraft should change captured geometry; that remains unproven until seen.
Do not call it playable if alignment/composition fails. Full MVP additionally
needs exclusive input handoff, guest-driven host camera, terrain collision and
transition/reconnect acceptance. Next smallest implementation depends on this
result, not on more speculative diagnostics.
