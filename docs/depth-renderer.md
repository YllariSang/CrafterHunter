# Depth-aware rendering path

## Verified native stone milestone — 2026-10-04

The sandbox blocker described below is resolved. Host access permitted actual
offline MHW testing on revision 421810, DX11/Proton/DXVK, at 1920×1080.
The new native path is opt-in and replaces A/B/C while selected. Initialization
failure does **not** silently fall back to the old overlay.

Evidence from the running game:

- Read-back `12293246-depth0.bin` contains the terrain; depth1 was empty.
  The full-resolution resource is R32_TYPELESS with shader-resource/depth
  bindings and is cleared to 0 (reversed Z).
- SPL's CPU viewport matrix uses forward Z. Treating it as reversed inverted
  the initial ray. The final implementation avoids conversion and frame lag:
  it binds the actual current draw's GPU camera constant buffer directly.
- A one-frame draw trace found the scene **before** native HUD drawing on an
  R11G11B10_FLOAT target. The observed UI signature is depth disabled,
  SRC_ALPHA/INV_SRC_ALPHA, vertex stride32, VS b0=1072 bytes and b3=400 bytes.
  The pixel shader additionally validates b3's pixel-to-clip scale. The shader
  uses b0's view-projection and inverse view-projection at byte offsets 0/320.
  These are observed constant-buffer layouts, not guessed retail pointers.
- The block remains world-anchored while orbiting. Sword/helmet and foreground
  vegetation occlude it. The Palico interaction prompt appears above the block;
  the game cursor stays visible, and the opened map covers the scene normally.
- The same renderer was exercised in the Research Base and Ancient Forest.
  Dithered camera-fade geometry has the game's existing dithered depth silhouette;
  this is not a solution for arbitrary transparent particles/glass.

Local screenshots (ignored build artifacts, not redistributed game assets):
`native/mhw-renderer/build/evidence/`, including `ch-native-preui2.png`,
`ch-cursor-over-stone.png`, `ch-map-over-stone.png`, `ch-forest-block.png`,
`ch-tree-hidden-active.png`, and `ch-tree-reappear-active.png`.

The test used the **existing installed Minecraft stone PNG** replayed through
the normal bridge/pixel receiver. No replacement texture or debug geometry
comparison was generated. This proves the receiver/native rendering path;
it is **not** evidence of a live Fabric client run in this session. The native
renderer draws a stone cube from those texels; it does not yet transfer a
Minecraft-rendered world framebuffer, collision, entities, or combat.

### Build and install

With MHW closed:

```bash
bash tools/build-mhw-spl.sh
bash tools/build-mhw-renderer.sh
bash tools/install-mhw-spl-plugin.sh install
bash tools/install-mhw-renderer.sh
```

Start the existing bridge and both games normally. Enter their worlds, then:

```bash
python tools/control-mhw-renderer.py place
```

This explicitly anchors the stone three metres down the current MHW camera ray.
It prevents title/loading cameras from automatically creating a bad placement.
It is only a visual placement: no ground snapping or collision is claimed.
After a scene change the anchor is dropped automatically, never rebuilt
(placement lifecycle below); run `place` again to restore the stone. Assets
expire after six seconds without the Minecraft feed. DX12, unsupported
executable hashes, unknown/stale depth, and missing UI signatures fail closed
instead of drawing through everything.

For development, `capture` reads back color/depth; `trace` captures one frame's
candidate passes; these deliberately stall the GPU and are not normal gameplay
operations. `reload` reloads a newly copied native DLL only, not managed changes.
Retired native modules remain mapped until game exit for callback safety.
The MinHook redistribution notice accompanies the native DLL.

### Placement lifecycle (v0.3.3, 2026-10-05)

Placement is explicit in both directions. `PlacementLifecycle` holds at most one
anchor and drops it when the camera reports a scene transition:

- a single-frame camera displacement over 25 m — a scene gate, fast travel or a
  cutscene camera cut; ordinary walking never covers that between two rendered
  frames, and camera rotation never displaces the camera at all; or
- no usable camera sample for over one second, which covers loading and title
  sequences where the anchor's world cannot be confirmed.

Nothing re-anchors afterwards. After a transition the stone stays absent until
the operator runs `place`, so a transition can never leave geometry anchored in
the previous world. `control-mhw-renderer.py clear` drops the anchor on demand.

Evidence from the running game (0.3.3 hot-reloaded into MHW 421810; screenshots
are local ignored artifacts under `native/mhw-renderer/build/evidence/`):

- `place` → `clear` → `place` in Ancient Forest logged `anchored`,
  `cleared by request`, `anchored`; `10-place-A.png`, `11-clear-B.png` and
  `12-place-C.png` show the stone present, absent, present.
- Camera rotation after `place` kept the anchor: the cube re-projected from the
  new angle with no invalidation logged.
- A player-driven transition to Astera logged `Native placement invalidated:
  camera jumped over 25 m in a single frame`. `31-after-transition.png` and
  `32-after-wait.png` show no stone, and a further 14 s produced no anchor
  event — no automatic re-anchor.
- A fresh `place` in Astera anchored at `<-378.678, -462.901, -164.472>`,
  a different world position from the pre-transition
  `<-24922.912, 4006.544, 40705.695>`; `34-astera-cube.png` shows the stone
  drawn there.

Observation: the first shot taken right after that Astera `place` showed no
stone although the anchor had already been logged, and it appeared with the
same anchor two minutes later. Anchor state and drawing are independent — the
Minecraft asset feed gates drawing separately (six-second expiry) — but this
instance was not isolated, so it is recorded as an observation, not a diagnosis.

No SharpPluginLoader lifecycle callback is used for placement: the invalidation
is driven purely by camera telemetry, which is observable and testable without
the game. The jump and camera-timeout paths are additionally covered by the
headless checks (`Placement lifecycle checks passed`).

### Depth-candidate investigation (2026-10-05, read-only)

`renderer.cpp` still selects `depths[0]`; the numbers below are evidence for
that question later, not a change to it. Four read-back sets from two sessions,
all 1920×1080 and format 39 (R32_TYPELESS, cleared to 0, reversed Z), measured
with `tools/inspect-depth-capture.py`:

| capture | `depth[0]` | `depth[1]` | `depth[2]` |
| --- | --- | --- | --- |
| `12293246` (previous session) | 97.4 % covered, 0.0026–0.018 | all zero | not present |
| `13897364` (previous session) | 99.96 % covered, 0.0025–0.080 | all zero | not captured |
| `13961495` (previous session) | 99.97 % covered, 0.0025–0.080 | all zero | not captured |
| `6774190` (sky filling ~46 % of frame) | 54.2 % covered, 0.0045–0.136 | all zero | not present |

`6774190-depth0.png` beside `6774190-color.png` is the visual form of the same
frame: sky black because reversed-Z far clears to 0, foliage and hunter
silhouette in the same positions as colour, and the stone absent from depth
because the renderer reads depth and never writes it. The percentiles are
consistent with a small near plane (0.005 ≈ 10 m of terrain, 0.14 ≈ 0.4 m).

Two observations for whoever revisits selection:

- Candidates are appended on first discovery and pruned only on resize. In the
  0.3.3 session `depth[1]` appeared about two minutes *after* `depth[0]`, so
  list index records discovery order, not scene-depth priority.
- All captures come from one build, one resolution and few areas. Before
  adopting any rule other than `depths[0]`, capture another area and at least
  one cutscene, and prefer this-frame freshness over list index.

**Freshness alone is not sufficient (measured 2026-10-05).** Two captures from
that day each hold three candidates, and in `14757842` candidates 0 and 1 were
*both* fresh (`age=1`) while candidate 1 read back `0.00 %` covered — cleared
every frame and never rendered into. Binding that one would put zeros in
`sceneDepth`, and under reversed Z `depth + epsilon < 0` never holds, so the
stone would draw through every object: precisely the failure this path exists
to prevent. Candidate 2 is the opposite hazard, stale with frozen content.

| capture | `depth0` | `depth1` | `depth2` |
| --- | --- | --- | --- |
| `13166284` (10-05 06:24) | 56.25 % covered, percentiles vary with the view | 0.00 % covered | 4.47 %, `age=87757` |
| `14757842` (10-05) | 100.00 % covered, median 0.0326 | 0.00 % covered, `age=1` | 4.47 %, `age=134603` |

`depth2`'s percentiles are byte-identical across the two captures
(0.074314 / 0.085027 / 0.088576) taken in different views, which is what a
frozen buffer looks like.

A third capture in a **different area** answers the "capture another area" step
above (`2089775`, 10-05 12:35, expedition camp rather than the morning's area):

| capture | `depth0` | `depth1` | `depth2` |
| --- | --- | --- | --- |
| `2089775` (10-05 12:35, second area) | 99.75 % covered, median 0.00788 | 0.00 % covered, `age=1` | 0.00 %, `age=24924` |

The fresh-but-empty hazard reproduces here exactly — `depth1` is again `age=1`
and `0.00 %` — so it is a property of the resource, not of one area. `depth2`
holds *no* content in this area (4.47 % in both morning captures), which also
means "known to contain scene content" has to be re-established occasionally
rather than decided once at discovery.

The selection rule therefore needs two conditions, not one: **fresh this
frame** *and* **known to contain scene content**, with content established by
an occasional read-back rather than on every frame. Until that exists,
`depths[0]` plus the staleness check stays, because it is the only candidate
verified to hold geometry.

### Live composition checks (2026-10-05)

The person drove these against the live games during the camera-link session
and reports them passing: the block keeps perspective while orbiting,
foreground geometry hides it and it returns when the obstruction clears, and
the map and the game cursor draw above it. The stone was fed by the running
Minecraft client (`Received actual minecraft:stone pixels and PNG from
Minecraft` in `CrafterHunter.runtime.log`), so this covers the live path that
the 2026-10-04 session deliberately did not claim.

The cutscene case is **untested** — not a failure, simply never driven. It
stays open, along with the depth-selection rule and the frame-synchronization
evidence below.

### Cutscene, teleport, and recorded composition (2026-10-05)

Two person-recorded videos plus the runtime log cover the rest of this
milestone. Evidence frames live in
`native/mhw-renderer/build/evidence/camera-link-acceptance/run/` (gitignored);
the video files stay in `~/Videos/` and are never committed.

**Camera link through a scripted sequence** — `recording_2026-10-05_12.21.00.mp4`
(146.75 s, 1920×1080). HUD read from four frames across the recording:

| t | scene | HUD |
| --- | --- | --- |
| 6 s | airship sequence | `LIVE \| 16 pkt/s \| seq 978 \| age 24ms` |
| 42 s | white transition | `LIVE \| 17 pkt/s \| seq 1610 \| age 7ms` |
| 54 s | handler cutscene | `LIVE \| 17 pkt/s \| seq 1834 \| age 29ms` |
| 90 s | monster cutscene | `LIVE \| 17 pkt/s \| seq 2458 \| age 5ms` |

1,480 packets across 84 s with no gap and no `WAITING`, age never above 29 ms,
through a loading transition and two cinematics, with Minecraft's view tracking
the cutscene camera the whole way. Frames:
`20261005T1221-cameralink-through-cutscene-{transition,handler,monster}.png`.

**Composition during live play** — `recording_2026-10-05_12.37.51.mp4` (13.16 s).
Placement anchored at 12:37:34 and never invalidated during the recording, so
the stone was live for every frame of it. The cube is visible as a
world-anchored block with two distinct faces over the camp while the HUD holds
`16–17 pkt/s`, `age 1–25 ms`, `seq 12245 → 12436`. Frames:
`20261005T1237-stone-composited-in-game-{1,2,3}.png`.

**Teleport invalidation** — `CrafterHunter.runtime.log` records three
`camera jumped over 25 m in a single frame` invalidations that day (12:19:08,
12:33:05, 12:37:23), each followed by a manual `place` and never by an
automatic re-anchor, which is the v0.3.3 placement lifecycle behaving as
specified.

**What is still not observed:** the cube on screen *during* a cutscene. In the
first recording the placement was invalidated at 12:19:08 and never
re-anchored, so `enabled=0` and the renderer did not compose at all — the
session's `renderer.log` has no `Composing before MHW UI` line after its init
until the later `place`. The camera half of that case is proven above, and the
composition half is the same per-frame path whoever owns the camera:
`Composing before MHW UI with current GPU camera constants` reads MHW's own
GPU camera state rather than assuming a free camera. The residual risk is
therefore concentrated in depth selection — a fresh-but-empty candidate during
a cutscene — which is the tracked item above, not in camera ownership.

### Remaining scope

This is experimental support for the pinned executable and tested DX11 settings,
not a guarantee across every graphics option, cutscene, resolution, or game build.
Next: live Fabric end-to-end verification, then Minecraft-owned placement/removal
state. Full color/depth passthrough and gameplay authority remain later work.

## Earlier architecture and audit (historical)

The v0.3 stone transfer and projection work, but its ImGui path is a screen
overlay. `GetBackgroundDrawList` places it beneath ImGui windows and cursor;
it does **not** compare against MHW's scene depth. The v0.3.1 recording shows
C hiding the hunter while A's custom mesh is absent even under a visible label
against open sky. More mesh/line position tweaks will not solve the layering.

The next production path is a native Direct3D 11 compositor on Proton/DXVK,
separate from the SharpPluginLoader telemetry plugin:

1. Observe the actual swap chain and per-frame depth resources on this MHW
   build. The installed config currently has DX12 disabled. Fail closed if the
   running device is DX12, the depth format is unknown, or its dimensions do
   not match the scene color target. Do not guess a retail memory offset.
2. Send Minecraft's rendered color and depth together with the exact camera
   pose and frame ID through a bounded, versioned shared-memory ring. Continue
   sending small state packets through the existing local bridge.
3. During a D3D11 scene-color composite, compare Minecraft depth with the
   observed MHW depth per pixel. Draw the Minecraft world layer only where it
   is nearer; keep hand/HUD and the final cursor in later UI passes.
4. Verify on an open view, then at a wall and the hunter silhouette. The stone
   must keep perspective while orbiting, disappear behind nearer MHW objects,
   and never cover MHW UI or cursor. If any depth resource is unavailable,
   disable world compositing rather than reverting silently to draw-through.

The [MIT-licensed crossover bridge](https://github.com/justbustin/minecraft-crossover-bridge/blob/main/docs/how-it-works.md)
demonstrates this architecture for MHW on CrossOver/DXMT, including finding
scene depth by observing depth clears. Its hooking details are not assumed to
work unchanged on DXVK. A GPU capture or equivalent read-only D3D11 trace is
the evidence gate before installing a native hook on this user's game.

## 2026-10-04 installed-build and automation audit

The latest user screenshot still shows C and the A/B labels. A repeat of the
same comparison is not an acceptance test for a fix: C explicitly has no
scene-depth input.

Read-only checks after this report established:

- The built and installed `CrafterHunter.MHW.dll` both have SHA-256
  `db31a7bffdea214fee17ace58dee1a103d555da5f933f30478ddfa9e58e5b035`.
  The earlier deployment mismatch is resolved.
- The installed loader log identifies MHW revision `421810`, initializes
  D3D11, and reports registration of four color meshes from `minecraft:stone`.
  Registration is not evidence of successful drawing or occlusion.
- `MinecraftBlockRenderer.DrawOverlay` projects and paints colored quads
  through ImGui. It does not read or test MHW depth. Moving between ImGui draw
  lists cannot add terrain/hunter occlusion, or establish ordering relative
  to MHW's own UI passes.
- The user has explicitly authorized unattended game testing. However, this
  agent session cannot access Hyprland: `hyprctl -j clients` fails with
  `Couldn't set socket timeout (2)`. A request to run it outside the sandbox
  was rejected by the execution policy, which disables sandbox approvals.
  This is an execution-access blocker, not missing user consent.
- No callable desktop-control or GPU-capture tool was exposed in this session.
  Shell networking also could not resolve GitHub; browser access could read
  the reference project's architecture but not the required native renderer
  source. No native graphics hook was installed or runtime pass claimed.

Resume with host-session access to Hyprland, the game process/GPU capture, and
the exact MHW plugin deployment directory. First capture the existing failure
and inspect scene depth and render order. Then implement and verify the depth
path above using Minecraft's existing block data. Keep camera-orbit, foreground
occlusion, and UI/cursor visibility as acceptance criteria; a successful build
or an unobstructed overlay alone must never mark this milestone complete.
