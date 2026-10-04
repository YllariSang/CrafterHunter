# CrafterHunter mod journal

## 2026-10-04 — scope and recon

User target: real Minecraft inside MHW Iceborne with world, monster, and story
interaction. Existing repository already chooses a separate-process bridge.
Created `MODDING_PLAN.md` with ordered acceptance milestones and explicit
ownership of simulation and progression.

Read universal-modder mod-any-game, game-recon, mashup-mods, and
share-field-notes instructions. Checked the plugin's local knowledge index;
no MHW-specific entry. The CLI is absent from PATH, but the plugin's source
and skills are locally available.

`tools/doctor-linux.sh` found the host installation, Proton prefix, SPL
bootstrap, CrafterHunter endpoint, and build prerequisites. Fingerprint matches
the previous recorded executable and Steam build. No game installation changed.
Reviewed public SPL callback docs and the credited macOS crossover README.
Full story compatibility is not established by these sources.

`cargo test --workspace`: seven tests passed. Direct execution of
`tools/test-fabric-camera.sh` failed because the file is not executable;
run it using Bash without changing file permissions.
`bash tools/test-fabric-camera.sh`: passed anchor, movement, yaw wrapping,
recovery, world change, toggle, decode, and freshness checks. These are
headless checks, not evidence of live game compatibility.

Next acceptance: live camera test, then a depth-composited cube on Linux/DXVK.
Ownership/offline test profile confirmation is pending. No gameplay mutation,
save editing, or game launch was performed during recon.

## 2026-10-04 — live camera and rendering probe

User confirmed that Minecraft follows the MHW camera while both games run.
Screenshots show the Fabric HUD reporting a live packet feed and bright green
probe geometry in an MHW expedition. The first probe used an assumed camera
forward axis; its edges appeared off-centre and sometimes over foreground
objects. This is evidence that `OnRender` and primitive drawing work, but not
proof of correct placement or depth occlusion.

Version 0.2.1 derives the probe's centre ray from the viewport matrices.
Headless tests pass for two view directions, reversed depth, and a singular
matrix; the plugin builds with no warnings. The user still needs to check its
placement and whether MHW terrain hides it. No game was launched or installed
by the agent during this change.

Further 0.2.1 screenshots from Astera's smithy show only a few green cube
edges. The installed DLL hash matches the published 0.2.1 artifact. Because
the smithy is full of close geometry and the camera is aimed steeply down or
up, these images cannot distinguish clipping/occlusion from a placement error.
The next acceptance scene is a broad clearing with a level camera, followed
by a controlled wall-occlusion shot.

User corrected the interpretation of those screenshots: the green geometry
stays in a player-relative direction, moves incorrectly with camera rotation,
and appears over foreground objects. This is a placement/depth failure of the
0.2.1 probe, not merely a crowded-scene artifact. Version 0.2.2 switches the
render probe to the loader's world-space camera target and logs up to twelve
camera/target/centre/projection samples to the loader log. It adds a magenta
centre marker to distinguish pose from line rendering. Headless target-ray
checks and the .NET plugin build passed; live placement is pending.

The user then shared expedition screenshots and a recording of 0.2.2 showing
a magenta wire sphere near screen centre and isolated green lines. The probe
log shows camera/target motion, but every recomputed centre projects to about
(960, 540). This exposes the placement error: the draw location was recreated
in front of the camera every sample. Version 0.2.3 anchors the cube once in
world space during ordinary camera movement, re-anchors after a camera jump
over 25 m, and removes the magenta diagnostic sphere. The screenshots still
suggest that debug primitives do not provide the depth behaviour needed for
Minecraft geometry; that remains an open milestone.

## 2026-10-04 — stone A/B/C first live result

The user's 23-second expedition recording shows the v0.3 comparison labels.
The visible red cube sits under **C**, the projected image-quad path, and draws
through MHW foreground. **A** has no visible mesh and **B** shows only a few
white points. The SPL log confirms Minecraft's stone asset arrived, four
color meshes were registered for A, and the 16×16 PNG loaded for C. The stone
PNG in Minecraft's local client cache is grayscale, so the red C result is
not the intended block color. A/B are staged against dense nearby terrain;
this footage alone does not prove whether they are occluded or the primitive
draws fail. These are observations, not proof of a depth-correct render.

Version 0.3.1 changes C to draw the actual transmitted RGBA pixels directly
as ImGui quads, eliminating the suspect GPU texture-handle path. It moves the
three 70 cm blocks to a tighter row, 40 cm above the original anchor, and adds
full-length pixel-colored edges to B so its line pass is legible. C remains
depth-unaware by design; a host depth-buffer integration is still required.

## 2026-10-04 — cursor overlap confirms overlay ordering

The v0.3.1 recording shows C with the actual gray stone pixels and cube
perspective, but it still covers the hunter and terrain. A's label is visible
against open sky while its mesh is absent; B still contributes only tiny
fragments. The user's observation that C covers the cursor is additional
evidence of the foreground ImGui draw-list ordering, not evidence that the
camera transform itself is wrong.

Version 0.3.2 puts C's pixels on ImGui's background draw list, leaving labels
foreground. This should place UI windows and ImGui's software cursor above the
block, but **does not** put MHW world geometry above it. SharpPluginLoader's
public 1.0 rendering API has no scene-depth texture accessor. The reference
MHW crossover uses a Direct3D Present composite with the game's depth buffer;
porting that behavior to Proton/DXVK requires a separate native graphics path
and validation of the actual D3D version and depth resource before injection.

## 2026-10-05 — explicit placement invalidation (0.3.3)

Baseline first: MHW in-world with the stone composing under the HUD, and the
Minecraft HUD reporting `MHW LINK: LIVE | 1 pkt/s | seq 8147 | age 269ms`,
later `WAITING | seq – | age –1ms` while Minecraft was backgrounded. The bridge
performs no rate limiting and the Minecraft log carried no errors, so the rate
reads as background throttling rather than a protocol defect. Recorded as an
open observation, not changed in this entry.

`PlacementLifecycle` replaces the process-lifetime anchor. It drops the anchor
on a single-frame camera jump over 25 m, or on over a second without a usable
camera sample, and never re-anchors: after a scene change the stone stays absent
until an explicit `place`. `control-mhw-renderer.py clear` drops the anchor on
demand. Headless checks cover the jump, camera-timeout, non-finite sample, clear
and reset paths (`Placement lifecycle checks passed`); Rust, Fabric and the
existing .NET checks stayed green. Version bump to 0.3.3.

Verified live in the running game with 0.3.3 hot-reloaded without an MHW
restart: `place` → `clear` → `place` produced anchored / cleared by request /
anchored log lines with the stone present, absent and present; camera rotation
kept the anchor and re-projected the cube; and a player-driven transition to
Astera logged `Native placement invalidated: camera jumped over 25 m in a
single frame`, after which no anchor event followed for the rest of the
observation — no automatic re-anchor — until a fresh `place` anchored at new
Astera coordinates and the stone drew there.

Injected input does not reach Proton in this session: uinput injection and
Hyprland's `hl.dsp.send_key_state` both left the game unaffected, so the scene
transition had to be player-driven.

No SharpPluginLoader lifecycle callback is relied upon, so none was verified;
placement invalidation uses camera telemetry alone. Depth-candidate selection in
`renderer.cpp` is untouched.

## 2026-10-05 — depth-candidate investigation (no selection change)

Measured the existing read-back sets instead of changing the selection rule.
`depth[0]` held scene geometry in all four capture sets across two sessions
(97.4–99.97 % coverage when terrain filled the frame, 54.2 % when sky filled
46 % of it), `depth[1]` was entirely zero at capture time in every set, and
`depth[2]` was never captured. The paired depth/colour PNGs put sky at the
reversed-Z far value, match foliage and hunter silhouette positions, and show
the stone absent from depth because the renderer reads depth and never writes it.

Candidates are appended on first discovery and pruned only on resize: `depth[1]`
appeared about two minutes after `depth[0]` in this session, so list index
records discovery order rather than scene-depth priority. `renderer.cpp` is
unchanged; any future rule must be validated in more areas and should prefer
per-frame freshness over list index.

## 2026-10-05 — live camera link acceptance: outage, rate, F7

Step 8 ran itself: the bridge process died at 05:02:07 after a clean
03:06:07-05:02:07 stretch, and Minecraft logged
`Bridge I/O failed; retrying: java.net.PortUnreachableException` at 60/min
while the HUD read `WAITING | 0 pkt/s | seq – | age –1ms`. Restarting the
bridge printed `registered Mhw` and `registered Minecraft` within seconds, a
second `Connected to crafterhunter-bridge/v1` appeared in the Minecraft log,
and the HUD returned to `LIVE` with neither game restarted. MHW noticed
nothing: its socket never errored, so no second HelloAck went out and the
ten-second camera grace was not re-applied. The bridge now runs detached from
any terminal via
`setsid nohup ./target/debug/crafterhunter-bridge > native/mhw-renderer/build/evidence/camera-link-acceptance/bridge.log 2>&1 &`.

The open `1 pkt/s` observation resolves as background throttling. Across a
68 s / 30-sample OCR series with MHW rendering, the HUD held
`LIVE | 16-18 pkt/s | age 3-55 ms` on every sample with an increasing
sequence; with MHW on an inactive workspace it fell back to `1 pkt/s` and
flapped between `LIVE` and `WAITING` as age crossed 500 ms. The producer caps
at 20 Hz, so the rate is MHW's frame rate: a window Hyprland is not rendering
blocks on Present and the camera payloads stop with it. Read the HUD with both
games on the active workspace.

F7 toggles the link cleanly: the HUD reads `OFF` while packets keep arriving,
and the second press returns to `LIVE`. Minecraft runs on XWayland, so `wtype`
never reached it; Hyprland's `hl.dsp.send_shortcut{mods="", key=..., window=...}`
did. That dispatcher targeted at the MHW window, `send_key_state`, and
`ydotool key` still leave Proton unchanged, while `ydotool mousemove` does move
the system cursor, so uinput reaches the compositor and only delivery into
Proton fails. Rotation, translation, inverted axes and the F8 fresh anchor stay
player-driven; steps 5, 6 and 9 of `docs/camera-link-test.md` remain open.

## 2026-10-05 — step 9 verified, and what a measured rotation proved

Quit and rejoined the Minecraft world with injected input only. Hyprland's
`send_shortcut{window=<window>}` drives Minecraft *when that window has focus*
(silently no-ops otherwise, which cost several attempts), and `ydotool` reaches
it through uinput, so mouse moves plus `ydotool click 0xC0` walked the pause
menu and the world list. The world unloaded at 06:04:11 and reloaded at
06:10:40; a minute later the HUD read `LIVE | 16-17 pkt/s | age < 68 ms` with
neither game nor the bridge restarted. The placement half comes from
`control-mhw-renderer.py capture`: the `enabled=1` branch was taken with
`parameters.centre` still holding the anchor, and the texels at the anchor's
projected screen position are exactly `127,127,127` and `143,143,143` — the
Minecraft stone palette, which Astera's frame never produces. The runtime log
has logged no invalidation since `anchored` at 04:46:10, so the stone survived
the bridge outage, the world unload and the reload.

Reading the renderer back also produced the measuring tool the eyeball tests
were missing. `parameters.bin` is `{inverse, projection, centre, screen}`,
`centre` is the anchor and `matrices.bin` is the same viewProjection, so the
anchor's screen position is `centre * viewProjection` under System.Numerics'
row-vector convention — the column-vector reading lands on screen centre and is
wrong (it does not match where the stone actually is). One -600 px
`ydotool mousemove` walked that projection from (483,406) to (1335,822): a real
camera rotation that did *not* invalidate the placement, exactly as the
lifecycle specifies, and the stone moved with it — the box at the new
projection is 4200/4200 neutral texels of the stone palette while the old
position holds only dark scene.

Input into Proton is still the blocker for steps 5 and 6. Pointer motion
rotated MHW twice, but after any focus switch to Minecraft the identical moves
leave MHW pixel-identical (mean 0.00-0.14 over 3 s) while the game keeps
rendering (6,790 pixels change elsewhere in those 3 s) and the link stays
`LIVE | 12 pkt/s`; a click to re-grab the pointer does not help and
`ydotool key` with W held a full second never moves the hunter. F7 and F3 do
work on Minecraft when it is focused, which is how the ON/OFF comparison was
taken: `OFF` visibly changes Minecraft's world view (mean 8.45 over 3,514
pixels), so the link's pose really is being applied. Whether it follows MHW in
the same direction still needs a person — Minecraft renders a featureless dark
grass field, and MHW ignores injected input.
