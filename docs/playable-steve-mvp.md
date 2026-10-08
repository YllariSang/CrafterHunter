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
explicitly maps a fresh first-person guest camera to the current MHW camera origin;
F5 does not change this anchor. Shader unprojects real depth with its issue-time
inverse projection, transforms through exact captured view/position and the fixed
anchor, converts host metres to GPU centimetres, and projects with the same draw's
observed host GPU camera buffer. Clear/invalid depth and unsupported upper-left
clip origin refuse. MHW scene-depth selection remains unchanged. A private D32
target handles guest self-occlusion; MHW's own depth is never overwritten.

This preview samples one source point per 4x4 texels and draws a 4x4 host-pixel
splat (six vertices). The sampling is an explicit bounded spike budget, NOT a
claim of complete geometry or correct reconstruction between samples. At 949x1028
it submits 366,996 vertices per composed frame. Holes/disocclusion and size changes
are expected. Guest terrain is included, not removed; no replacement model/assets.
First-person hands are excluded by the existing capture boundary. No lighting
integration, alpha-correct transparency or 60 fps claim.

Fabric `world-stream.enabled` supplies same-frame pairs at a requested maximum
4 Hz, one copy pair in flight, matching native 250ms polling. Streaming skips
diagnostic PNG/raw artifact writes; prior diagnostic artifacts/meta remain intact.
It retains the permanent fail-closed GPU lifetime policy. Remove the flag to stop;
no channel deletion. Native upload watchdog still expires after one second and
drops calibration, requiring explicit recalibration after recovery. Host epoch
uses observed jump/gap continuity, not an authoritative area ID/title detector.

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
