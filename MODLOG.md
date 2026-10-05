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

## 2026-10-05 — steps 5 and 6 verified by the person

The person drove MHW directly and reported that camera, movement and position
track between the two games: Minecraft follows MHW's rotation in the same
direction with no inverted axis, and its view moves relative to the starting
Minecraft viewpoint instead of jumping to MHW's absolute coordinates. F8
re-anchors from the current vanilla Minecraft camera and re-orients both the
player and the camera, so the fresh-anchor half of step 6 passes too. The two
halves the agent could only prove separately — that Minecraft applies the pose,
and that the native placement tracks rotation without invalidating — are now
covered end to end by a person seeing the link behave.

The run was performed with Minecraft parked on a menu, because Minecraft
periodically takes input away from MHW. That is a workaround around window
focus, not a link defect, and it is recorded in `docs/camera-link-test.md` so
the next run does not mistake it for one.

Two operational notes from the session. The bridge died on `EINTR` with
`Error: Os { code: 4, kind: Interrupted, message: "Interrupted system call" }`
and exited instead of retrying the socket read; a restart brought both clients
back (`registered Mhw`, and `registered Minecraft` on a fresh port) with neither
game relaunched, so the recovery path in step 8 held a second time, but the
missing retry is a real defect to fix. And the layout cost of testing is now
measured: both games must share the active workspace or MHW stops rendering and
the feed falls to `1 pkt/s`, while Minecraft must stay visible above MHW's
window or its response cannot be observed — Hyprland renders only the active
workspace, and MHW re-raises itself over the pinned Minecraft overlay at
intervals.

Milestone 1 in `MODDING_PLAN.md` is recorded with the step 9 dimension change
and fifteen-minute expedition still open. Next milestone is 2, composition: one
Minecraft block inside an MHW expedition, hidden by MHW terrain at different
camera angles, after verifying the actual DXVK color/depth resources and frame
synchronization.

## 2026-10-05 — step 9 closed, camera acceptance passes

The person ran the expedition and the dimension change. `CrafterHunter.runtime.log`
anchors the placement at 11:04:54 and holds it for twenty-seven minutes with no
invalidation, then logs `Native placement invalidated: camera jumped over 25 m
in a single frame` at 11:32:25 — the camp teleport. That is the specified
explicit invalidation for an area transition rather than a loss: the stone goes
away and nothing re-anchors it on its own, so F8 is required to start again.
The Nether change was reported flawless.

The corroborating counts matter as much as the report. `latest.log` contains no
CrafterHunter exception anywhere in the run window; the only error lines are
the bridge outage at 60 per minute from 11:00 to the 11:04:32 restart, and none
after it, with no further `PortUnreachableException` through 11:36. So the link
ran the whole expedition without a single retry, and the outage recorded
earlier in this journal was confined to before the run.

Milestone 1 in `MODDING_PLAN.md` is now a pass: all nine steps of
`docs/camera-link-test.md` are recorded, with the live-game halves driven by the
person. Next is milestone 2, composition — the occlusion acceptance scene is
the gap, since the native depth-tested renderer, the identified scene depth
resource and the placement lifecycle already exist.

## 2026-10-05 — the bridge survives an interrupted read

The outage recorded earlier today was the bridge itself. A signal landed on
`recv_from`, which returned `Interrupted`, and the receive loop only tolerated
`WouldBlock` and `TimedOut`, so `main` returned `Err` and the process printed
`Error: Os { code: 4, kind: Interrupted, message: "Interrupted system call" }`
and exited. From the games' side that is just a dead peer: Minecraft retried at
60/min until the restart, MHW kept sending into the void.

`is_transient` now covers `Interrupted` alongside the timeout cases, so the
receive loop simply tries again. Every other error still returns, because a
bridge that fails loudly beats one that goes quiet during a live session. The
send path carried the same latent fault — `send_packet` and `send_bytes` used
`?`, so an interrupted datagram send would have killed the process in exactly
the same way. Sends now go through `send_with_retries`, extracted as a
closure-taking function so the retry is testable without a socket: it retries
an `Interrupted` send until it lands and returns any other failure at once.

Five tests cover the decision: interrupted, would-block and timed-out reads are
retried; broken-pipe and permission-denied are not; an interrupted send is
retried to completion; a real send failure returns on the first attempt; a
truncated datagram is still an error. `cargo test --workspace` reports twelve
passing, `cargo clippy --workspace --all-targets` is clean, and
`tools/test-fabric-camera.sh` passes. The bridge was rebuilt and restarted so
the running process carries the fix.

This was step 0 of the composition prep. Next: depth-resource selection by
this-frame freshness instead of discovery order, plus the frame-synchronization
trace, before the cutscene case is attempted.

## 2026-10-05 — freshness alone would have broken occlusion

Reading the two captures left in `render/` from today (`13166284` at 06:24 and
`14757842` at 06:51) rather than launching anything turned up the design
constraint for milestone 2's depth rule. Both hold three candidates now, not
the two the earlier table saw. In `14757842` candidates 0 and 1 were **both**
fresh (`age=1`), and `inspect-depth-capture.py` reports candidate 1 at `0.00 %`
covered — cleared every frame and never rendered into.

That matters because the naive rule the plan was heading towards, "pick whichever
depth was cleared this frame", would have selected an empty buffer. Under
reversed Z an empty depth is all zeros, `sceneDepth` then reads 0, and
`depth + epsilon < 0` is never true, so nothing is ever discarded and the stone
draws through every object in front of it — the exact failure the depth path
exists to prevent, reached by a "safer" rule rather than by the old one.
Candidate 2 shows the mirror image: `age=87757` and `age=134603` with
byte-identical percentiles across two different views, so it is stale and
frozen.

Selection needs two conditions — fresh this frame **and** known to contain
scene content — with content coming from an occasional read-back rather than
every frame, since a per-frame read-back stalls the GPU. Until that exists
`depths[0]` plus the staleness check stays, because it is the only candidate
verified to hold geometry. Recorded in `docs/depth-renderer.md` next to the
table it extends.

## 2026-10-05 — recorded cutscene, teleport, and second-area evidence

Two recordings from the person close the observable half of milestone 2, and
one of them changes what was claimable about cutscenes.

`recording_2026-10-05_12.21.00.mp4` (146.75 s) is a scripted sequence: a white
transition at 42 s, a handler cutscene at 54 s, a monster cutscene at 90 s. The
HUD reads `LIVE | 16–17 pkt/s` at `age 5–29 ms` with `seq 978 → 2458` over 84 s
— no gap, no `WAITING`, Minecraft's view following the cutscene camera
throughout. That is the camera half of the cutscene case, measured rather than
argued.

It is also the recording that shows why the *composition* half was never
observed: `CrafterHunter.runtime.log` has placement anchored at 12:19:01 and
invalidated seven seconds later by a >25 m jump, with no re-anchor before or
during the recording. With `enabled=0` the renderer does not compose — the
session's `renderer.log` has no `Composing before MHW UI` line after its init —
and indeed no cube appears in any of the twelve frames sampled. So that video
is evidence for the link and explicitly not evidence for composition during a
cutscene. Recorded as such instead of being counted as a pass.

`recording_2026-10-05_12.37.51.mp4` (13.16 s) supplies the other half in
normal play: placement anchored at 12:37:34, never invalidated during the
recording, and the stone visible as a world-anchored cube with two faces over
the camp while the HUD holds `16–17 pkt/s`, `age 1–25 ms`, `seq 12245 → 12436`.
Three frames saved to the gitignored evidence run directory. Separately, the
log shows three single-frame >25 m invalidations that day (12:19:08, 12:33:05,
12:37:23), each followed by a manual `place` and never by an automatic
re-anchor.

The capture taken at 12:35 in a second area is the "capture another area" step
the depth table asked for, and it reproduces the morning's hazard exactly:
`depth0` 99.75 % covered, `depth1` `age=1` and `0.00 %`, `depth2` `age=24924`
and `0.00 %`. Fresh-but-empty is a property of the resource, not of one area,
and `depth2` having no content here at all (4.47 % in the morning) means
"known content" must be re-checked periodically rather than fixed at discovery.

Milestone 2 is recorded as passing on these checks. What stays open is honest
and small: the cube was never seen *on screen during* a cutscene, the
depth-selection rule still needs its fresh-and-content design, and
frame-synchronization evidence has not been traced. Composition reads MHW's own
GPU camera constants each frame, so camera ownership is not the risk; the
depth candidate is.

## 2026-10-05 — the cube during a cutscene, finally observed

The gap the previous entry left open is closed by
`recording_2026-10-05_12.48.56.mp4`: 101.31 s of dialogue cutscene, subtitles
at 10 s, 66 s and 86 s, with the stone live the whole time. Placement anchored
at 12:48:52 and the only invalidation is a >25 m jump at 12:50:51 — fourteen
seconds *after* the recording stopped — so `enabled=1` for every frame of it.

Four sampled frames show the cube world-anchored inside the cutscene: behind a
close-up character at 10 s, two faces behind the Handler at 34 s with her
drawn in front of them, behind the white-fade shot at 50 s, and among three
talking characters at 86 s with a foreground character occluding it. At 66 s it
is simply out of frame, which is what a world-anchored block should do when the
cutscene camera looks elsewhere.

The occlusion is the part worth writing down. The cutscene's own characters
draw *in front of* the cube, so the depth resource bound during the cutscene
held the cutscene's geometry — the fresh-but-empty candidate that the morning's
captures made the leading selection hazard did not fire here. Selection still
needs its rule; this says the risk is real but not constant.

Link quality through the sequence: `seq 14706 → 15217 → 15557` measures 16.0
and 17.0 packets/s over the two intervals, agreeing with the `17–18 pkt/s` on
screen, `age` between 7 and 57 ms, `LIVE` in every frame that has a HUD at all.
A 101 s scripted sequence with no stall.

Eight `Native placement anchored` lines span the recording 4–21 s apart.
`render/place.request` has exactly one writer, `tools/control-mhw-renderer.py
place`, and no request file was left behind, so each anchor came through that
sanctioned path rather than from anything re-anchoring on its own.

Milestone 2 is now recorded as passing **with the cutscene case included**;
`MODDING_PLAN.md` and `docs/depth-renderer.md` updated. What remains on this
milestone is design work rather than observation: the depth-selection rule
(fresh *and* known content, re-checked periodically — `depth2` held content in
one area and none in another) and the frame-synchronization trace. Next phase:
terrain and player queries for milestone 3.

## 2026-10-05 — depth selection implemented, frame-sync trace added

The two follow-ups that were recorded as design work are now code.

**Selection** moved into `native/mhw-renderer/selection.hpp`, which carries no
D3D types on purpose so it can be tested with the host compiler. A candidate
binds only when it matches the backbuffer, was cleared to 0 (reversed Z), is at
most one frame old, **and** a read-back has shown it holds geometry. That last
condition is the one the morning's captures demanded: `14757842` and `2089775`
both held a candidate at `age=1` reading `0.00 %` covered, so freshness alone
would have bound an all-zero buffer and drawn the stone through everything.
An unmeasured candidate is not bindable at all, so on startup composition waits
for the first read-back instead of defaulting to index 0.

Content measurement is deliberately cheap and non-blocking: one copy in flight,
`Map` with `D3D11_MAP_FLAG_DO_NOT_WAIT` retried on later frames, a 90-frame
timeout, only for candidates that are fresh right now, and only on first sight
then every 300 frames. The re-check interval matters because content is not a
property of a resource for ever — `depth2` held 4.47 % in one area and 0.00 %
in another. Threshold is 2 % of a whole-frame sample grid, chosen to err
strict: too strict hides the stone, too lenient draws through.

`bash tools/test-depth-selection.sh` compiles the header with `-Werror` and
runs 13 checks — freshness boundaries, the reversed-Z clear, target mismatch,
the fresh-but-empty case, the threshold from both sides, discovery-order
fall-through, corner content still counting as content, and every failure mode
resolving to "skip this frame".

**Frame sync** is a bounded diagnostic rather than a permanent cost:
`python tools/control-mhw-renderer.py framesync` arms 60 composed frames, and
each one logs the bound depth with its coverage and age, an FNV-1a hash of the
GPU host view-projection of the very draw being composed (so a frozen camera
buffer shows up as `same` across frames that should differ), and the block
centre projected twice — once with those GPU constants and once with the
CPU-side view-projection handed in for the same frame. The pixel difference
between the two is the CPU/GPU sync error. `tools/inspect-framesync.py` judges
a trace and exits non-zero if a frame bound no depth, a bound depth was older
than one frame, the camera could not be read, or the projections disagree by
more than `--max-delta` (default 2 px); its failure paths were checked against
synthetic good and bad traces.

Renderer rebuilds clean under mingw with `-Wall -Wextra`. Gates run: 12 Rust
tests, the fabric camera checks, and the new depth-selection tests, all green.
Still outstanding is the live half — deploy the DLL, run the framesync trace
against the running games, and judge it.

## 2026-10-05 — depth selection and frame sync validated against the live game

The rebuilt DLL was deployed through the development reload path — previous
build backed up to `~/.local/share/crafterhunter-backups/renderer-20261005T131415/`,
pinned executable hash re-checked first — and the plugin picked it up at
13:14:16, one second after the reload request.

The new selection rule then showed its two behaviours in the running game,
which is what the whole change existed for.

**It fails closed.** The first composed frame logged
`no eligible depth candidate (1 observed; unmeasured, stale or empty) -
composition skipped`, then at 13:16:17, two seconds later, `depth[0] content
56.94% holds geometry (age=0)` and `depth select=0`. Nothing binds until a
read-back has proven the buffer holds geometry — no defaulting to index 0, and
no draw-through while the measurement is pending.

**It refuses a fresh empty buffer.** At 13:17:58 a second candidate appeared
(`depth[1] 1920x1080 format=39 bind=72 clear=0.000`) and was measured
`content 0.00% empty (age=0)`. That is the hazard from this morning's captures
— fresh, correctly cleared, holding nothing — occurring on its own in live
play, and the rule declined it. It never appears in a selection line.

The frame-sync trace was armed repeatedly across a minute of camera movement.
`python tools/inspect-framesync.py` reports PASS (exit 0) over 958 frames:

- every frame bound a depth, `depth=0` throughout, `age=0` throughout, no
  `depth=-1` anywhere;
- 37 distinct GPU camera hashes with 36 flagged `changed`, and the projected
  anchor moving from `(960.0, 540.0)` to `(1060.5, 504.4)` across 35 distinct
  positions — so the camera was demonstrably live rather than stuck;
- `delta` 0.000 px on all 958 frames including the moving ones: the CPU-side
  view-projection handed to `CH_Frame` for a frame is bit-identical to the GPU
  host matrix of the draw composed for that frame.

Rate context worth keeping: 28.9 fps averaged across the run, 37.7 fps with
MHW's window focused, and roughly 1 fps while MHW sat on an inactive
workspace. That last number is why the games must share the active workspace —
it is the same rule the packet-rate observation recorded in milestone 1, now
measured on the renderer side.

Evidence archived as `20261005T1321-framesync-trace.log` (958 frames,
gitignored). Both milestone 2 follow-ups are closed; `docs/depth-renderer.md`
carries the full table and `MODDING_PLAN.md` the updated status. The trace is
bounded — once disarmed, `frameSyncRemaining` hits zero and no further GPU
read-backs happen.

## 2026-10-05 — milestone 3 reconnaissance: what the host actually offers

Started milestone 3 by finding out what the pinned build will and will not do,
before writing any of the query path. Findings are recorded in
`docs/host-queries.md`.

**The player side needs no reverse engineering.** Reflecting the installed
`SharpPluginLoader.Core.dll` (2.0.0) in a scratch process — metadata only,
nothing executed inside the game — confirmed `IPlugin.OnUpdate(float)` as a
game-thread tick, `Entities.Player.MainPlayer` carrying `Model.Position`,
`Rotation`, `Forward`, and `CollisionPosition`, `SingletonManager.GetSingleton`,
and `Memory.PatternScanner.FindFirst(pattern, cache)` with `NativeFunction<..>`
for typed calls. Camera sampling already runs on that same tick.

**There is no loader API for a terrain raycast.** SPL's
`SharpPluginLoader.Core.Collision` namespace turned out to be attack and
hit-detection data (`AttackParam`, `CollGeomResource`, `HitZoneResource`), and
`MtGeometry` describes shapes attached to a model. Stage terrain has to be
queried through the game's own collision routine.

**The executable's code section is encrypted on disk.** `.text` is 48,304,640
bytes at entropy 8.00/8.00 with zero `48 89 5C 24` prologues — about eleven
would appear by chance in random data of that size — and `40 53`, `E8`, and `C3`
counts exactly at their chance expectation. The section has no instruction
structure at all.

This is worth recording honestly because it cost a wrong turn: the first tool I
wrote, `tools/inspect-mhw-signatures.py`, scanned the file for the seven
terrain-ray signatures and reported all seven MISSING in 0.8 seconds. I assumed
my matcher was broken and went to debug it. The matcher was fine; the bytes
really are not code. Offline signature verification is impossible on this build,
so the tool was replaced by `tools/verify-host-build.py`, which verifies what
can be verified and exits non-zero when any of it fails:

- the installed executable is the SHA-256 pinned in `MODDING_PLAN.md`;
- `.text` is opaque, which is the reason the scan happens in the loaded image;
- SharpPluginLoader's runtime address cache resolves `Player:FindMasterPlayer`
  to `0x141B42010`, identical to the address the MIT-licensed
  `justbustin/minecraft-crossover-bridge` published for build 421810, inside
  `.text` — the runtime layout of this executable matches the build those
  terrain signatures were written against;
- SharpPluginLoader's pattern cache already holds two plugin signatures it
  resolved in the loaded image on this build, both inside `.text`, so the scan
  path the adapter depends on demonstrably works here.

The adapter rules that follow are stricter than "call the same function":
signatures in one adapter type as byte patterns, fingerprint before resolving,
any miss disables terrain queries for the session, game-thread execution only,
a fixed query budget per tick with dropped-and-reported overflow, and "no
terrain" published instead of a stale answer while the singleton, player, or
stage is missing. The player proxy maps by relative displacement for the same
reason the camera link does: MHW world coordinates sit hundreds of metres from
Minecraft's origin.

No game file was modified and nothing was executed inside the game process.

## 2026-10-05 — milestone 3: the host player proxy, end to end

The player half of milestone 3 now runs from the game thread in MHW to the
player in Minecraft.

**Protocol.** `PlayerState` (kind 11) is seven `f32`: world position in metres
and the model rotation quaternion, mirroring `CameraState`. The Rust crate
encodes and decodes it and pins the layout with golden bytes, because C# writes
it and Java reads it and neither compiler checks the other's work.

**Plugin.** `SamplePlayer` runs inside `OnUpdate`, the same game-thread tick as
the camera, at the same 20 Hz. No player is a normal state rather than an
error: a loading screen or an area transition produces no sample, therefore no
packet, and the guest ages the stream out instead of holding a position that no
longer exists. Only a managed API failure stops player sampling, and it stops
the player alone - camera telemetry keeps running. The bridge needed no change,
since it already forwards every game-to-game packet.

**Guest.** `PlayerMixin` runs at the tail of `LocalPlayer.tick()`, so vanilla
finishes its own movement for the frame first; the proxy then places the player,
sets facing, zeroes velocity, and clears accumulated fall distance so physics
cannot apply a second move for one the host already made. F9 toggles the link.
The release path is the same code with no work to do: stale feed, disabled
toggle, or world change makes `PlayerLink.update` return null and the mixin
touches nothing, so movement returns to the keyboard on that tick.

Two decisions are worth stating rather than leaving implicit.

*Relative displacement, not absolute coordinates.* MHW's world sits hundreds of
metres from anything in a Minecraft world, so the first fresh sample anchors
both the host position and the player's current position, and later samples
apply their difference: one metre of hunter travel becomes one block.

*The 25 m single-frame jump rule, reused.* The placement anchor already treats a
single frame that moves more than 25 m as a scene change rather than motion. The
proxy does the same: such a jump re-anchors it where the player stands instead
of sweeping the player across the world to catch up.

**Refactors the change forced, both with their suites green afterwards.** The
camera's quaternion-to-yaw conversion moved to `Rotation`, shared with the
player, because two copies of that math would drift the first time one is
corrected. The freshness and arrival rules moved to `SampleFeed`, so camera and
player age samples by exactly one rule while remaining independent streams -
the player check asserts that a stale player sample stays stale while the camera
feed goes live, and vice versa.

**Checks.** Rust: 10 protocol tests including golden bytes and rejection of
wrong length, non-finite values, and an unassigned kind. Java: a new
`PlayerLinkTest` covering anchor, movement, yaw wrapping across +/-180, the jump
re-anchor, every release path, world change, payload decoding, and feed
independence; the camera suite still passes after both refactors. The two
headless scripts now read one shared source list so they cannot drift. The
mixins compile against the mapped 26.2 jar through `./gradlew --offline
compileClientJava`, which is the gate that proves `PlayerMixin` and its
injection target are real.

**Not done.** The terrain half of milestone 3 has not started. The proxy has not
yet been accepted in the running games: the yaw convention in particular is
derived from the camera's proven conversion and has to be watched against the
hunter's actual facing, because a mirrored proxy would still look plausible in
motion. My first run of the new suite failed on an expectation of mine, not the
code: identity wraps to -180, not +180.

## 2026-10-05 — player proxy accepted, and the bridge that was eating it

Minecraft was restarted with the rebuilt jar (40,805 bytes, previous jar saved
to `~/.local/share/crafterhunter-backups/minecraft-mod-20261005T1412/`), the
plugin hot-reloaded at 14:11:51, and both guarded reads armed at 14:12:03. The
guest connected on launch with no discarded-packet errors, so the deployment
looked complete.

**It was not, and neither end could tell.** The bridge binary had been started
at 12:20, before `Kind::PlayerState` existed, and the protocol rejects an
unassigned kind, so every player packet died in the bridge:

```
discarded invalid packet from 127.0.0.1:39442: UnknownKind(11)
```

5,138 times, at 20 Hz, while the plugin logged `First guarded player read
succeeded` and Minecraft logged nothing at all. The camera kept working — its
kind was already known — so the proxy simply never arrived and the HUD stayed
on `WAITING`. The only evidence was in the bridge's own log.

`cargo build --offline --workspace` recompiled the bridge and probe, and a
restart on the same log file re-registered both endpoints (`registered Mhw`,
`registered Minecraft`) with zero discards afterwards; the guest reconnected at
14:36:01 after one `PortUnreachableException` retry. This is the operational
half of "the bridge needs no change": the source is forward-compatible with new
kinds, a running binary is not. The line to add next time a kind is introduced
is in `docs/host-queries.md`, next to the status claim where it will be read.

**Verdict from the person:** camera and movement between the two characters are
precise and accurate, the same judgement that closed milestone 2, and they
asked to proceed to the next phase.

**What is still open, deliberately recorded rather than folded into the pass.**
The one reading of the proxy as *reversed* was taken with the camera link (F7)
off. That matters because F7 is what normally overwrites the view from the MHW
camera every frame: with it on, the model quaternion never reaches the screen in
first person, and with it off the vanilla camera follows `LocalPlayer.yRot`, so
the view *is* the model-quaternion path. The person attributed the reading to
the camera link being off rather than to the conversion, but the clean test —
F7 off, F9 on, turn the hunter 90 degrees and confirm the character turns the
same way — has not been run, nor has F9 mid-motion. Both stay in the
verification table in `docs/host-queries.md`.

**Why the agent could not take the screenshot itself.** MHW runs floating on
the same workspace, above Minecraft and re-grabbing focus, so `grim` captures
and an injected F2 both landed on MHW. Hyprland here dispatches through Lua:
`hyprctl dispatch 'hl.dsp.focus{workspace=hl.get_workspace(3)}'` switches
workspace, `hl.get_windows()` returns window objects, and `hyprctl eval` can
write files with `io.open`, which is how the window geometry was read. Raising
the game or lowering MHW is the person's layout, so the HUD reading stays with
them until they say otherwise.

## 2026-10-05 — terrain stage A: the game's own segment cast, behind a fingerprint

Milestone 3's terrain half was a design and nothing else. It now has an
adapter, `native/mhw-spl-plugin/TerrainAdapter.cs`, that resolves the game's
segment cast in the loaded image and proves it on the host.

**Getting the routine's contract back.** The seven terrain signatures were
known to exist only as bytecode: `tools/inspect-mhw-signatures.py` was never
committed, and its `__pycache__` entry was the only surviving copy. Marshalling
that `.pyc` back and disassembling the module gave the seven name-to-pattern
pairs and their published addresses. All seven match the MIT prior art's
constants exactly (`sCollision::CheckSegment` at `0x14231AC00`, the `Param`
block's three routines, `TriangleInfo`'s three), which is what makes the
pairing trustworthy: an off-by-one pair would have disagreed. The prior art's
`raycast()` supplied the rest — `Param::ctor(param, 0x7FFFFFFF, 0x3FFFFFFF, 0,
0, 0xA, 0, 1, 1, 0, 1)`, `param[0xF9] = 0`, two `0x140` caller-owned buffers
zeroed and 16-byte aligned, eight segment floats `start.xyz 0 end.xyz 0`, flag
`1`, hit position at `tri+0xC0`, normal at `tri+0xB0`, attribute through
`TriangleInfo::attr(tri, 0)`, and `reset`/`dtor` before the buffers go out of
scope. `docs/terrain-query.md` records it in full.

**The calling convention, answered from the running process.** SPL's
`NativeAction`/`NativeFunction` call through `delegate* unmanaged` handles, so
the first question was whether a managed call can reach game code at all under
Proton: the game's PE uses the Windows x64 convention, while a Linux CLR would
emit the System V one. `/proc/4959/maps` answers it — the game's process maps
`coreclr.dll` out of
`steamapps/compatdata/582010/pfx/drive_c/Program Files/dotnet/shared/Microsoft.NETCore.App/8.0.12`.
The runtime inside the prefix is the Windows build, so its function-pointer
calls already match the game, and the loader's own types are the right door.
Calling one is an unsafe call, so `AllowUnsafeBlocks` is now on in the plugin
project and the unsafe surface is three methods: `TerrainAdapter.Tick`,
`TerrainAdapter.CastDownRay`, and `Plugin.OnUpdate`, which exists only to reach
the ray. `Pattern.FromString` was round-tripped in the scratch reflection tool
against both the loader's 2.0.0 and the 1.0.0 package the plugin compiles
against, confirming `??` wildcards parse and garbage throws.

**The adapter.** `OnLoad` hashes `MonsterHunterWorld.exe` from the process
working directory (the game root, confirmed by `readlink /proc/4959/cwd`),
compares it with the pinned SHA-256, resolves all seven patterns with
`PatternScanner.FindFirst(pattern, cache: true)`, and allocates the aligned
buffers. The first failure latches `Disabled` with its reason and no tick can
move it back. `OnUpdate` ticks the adapter *before* the bridge gates anything,
because a terrain answer is a host fact and should not wait for a guest that is
still connecting; it publishes the state before it casts, so a loading screen
reads *unavailable* rather than the previous ray's answer. One ray per tick,
one-second burst of twelve self-checks then a five-minute heartbeat, each line
carrying the ray height, `CollisionPosition`, the model origin, the normal, and
the attribute. `CollisionPosition`'s meaning is not assumed: if it is the feet
rather than the capsule centre, the delta shows it instead of hiding it. Every
native call is wrapped, so a failure disables terrain for the session instead
of escaping into the game process.

**A gate that was missing.** The plugin's headless tests
(`native/mhw-spl-plugin/tests`) existed but nothing in the gate ran them.
`tools/test-spl-plugin.sh` does now, and the terrain policy went into them:
every signature parsed through the loader's own `Pattern.FromString`, the
down-segment geometry, the agreement tolerance including its boundary and its
refusal to judge non-finite numbers, and all seven branches of the state
machine. A typo in a byte pattern fails there instead of quietly disabling
terrain in the next session.

**Not done, and not hot-reloaded yet.** The protocol half (a request kind, a
result kind, and a bounded queue for requests arriving from the guest), the
guest half (mapping Minecraft coordinates through the proxy anchor), and the
live run are all still ahead. The adapter has deliberately not been loaded into
the running game: a native call into the game's collision routine is the first
thing in this project that can take MHW down if the recovered ABI is wrong, and
it should go in on purpose, with someone at the keyboard who can stop it.

## 2026-10-05 — the terrain self-check became a request, not a heartbeat

The adapter committed in `f572a79` cast a down-ray on a one-second burst as soon
as the world was ready. Reviewing it before deploying: that puts the first
native call into the game's collision routine about a second after a world
loads — during a cutscene, a quest transition, or with nobody watching — and it
would have fired unattended in a session that had not asked for terrain at all.
It was never hot-reloaded, so no game was ever exposed to it.

The rule is now explicit. `TerrainAdapter.Tick` publishes the state every
sample and casts **at most one ray, only when a request is pending**, and the
request is a file next to the plugin's log:
`nativePC/plugins/CSharp/CrafterHunter/terrain/check.request`, written by
`tools/control-mhw-terrain.py check` and removed by `clear`. The file is deleted
before the ray runs, so a request fires exactly once; a request that arrives
while the singleton or the hunter is missing is refused in the log with the
state that refused it, rather than silently discarded. This is the same
request-file pattern the native renderer already uses for `place` and `clear`,
and it has the property the acceptance run actually needs: the cast happens
when a person is standing on the ground they want measured, which is also the
only way to measure a slope and compare it with flat ground.

`TerrainRequest` holds the channel and the test project covers it: an absent
request produces nothing, a pending request is taken exactly once, a cleared
request never fires. The per-tick budget is unchanged in spirit and tighter in
fact — one ray per one-second sample, requested — and stage B's guest queue will
spend that same budget rather than a second, larger one.
