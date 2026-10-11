# Camera and player capture time

The ACTIVE paired rendering path now builds the host camera from the same
uploaded world capture as Steve, the cape and held item. The independent camera
feed still supplies request token, generation, advancing control sequence and
the unchanged 500ms lease. World capture receipt freshness remains 1000ms.
Snapshot and acknowledgement formats, source mesh, skin, shaders and depth tests
are unchanged. Standalone model/camera rendering still uses the fast feed.

User observations narrow the problem to walking/look under Minecraft-owned
camera control. Unarmed camera rendering is good; stationary off/on normal and
depth-source diagnostics keep Steve intact. During movement both real and white
diagnostic rendering flicker, with cape/held-item involvement. Flying makes the
player jitter. The diagnostic bypasses host depth only for the separate body;
captured attachments retain their normal depth test. It does not establish that
host depth is irrelevant to every affected fragment.

Evidence is in ignored
`captures/player-camera-occlusion/runtime-20261011-221924Z/`. Generation
43698606417375, fixed epoch 1763, token 7814624920158794757. Actual 1920x1080
colour/depth captures exist for unarmed/ACTIVE real/source/restored-real phases;
moving screenshots and advancing identities exist for all diagnostic phases.
Flags were restored to normal shading after each comparison. No F5 re-anchor.

Read-only sampling of our loaded renderer compares its copied camera plan to
the uploaded atomic capture's body and actual view/projection. Frame-straddling
reads are refused. A stationary 20s window has 959 coherent samples, one feet
position and below 0.00002-pixel projected-centre discrepancy. The requested 45s
flight observation has 2160 coherent samples, seven refused reads and 262 feet
positions. The 261 samples that advance feet have median absolute X/Y probe
discrepancy 23.77/46.16 pixels and 95th 119.59/153.21 pixels. This is an analytic
centre probe, not an anatomical pixel, simultaneous GPU capture, screen latency
or a frame-rate measurement.

Body and attachments already use a pose frozen at GPU readback issue. The camera
previously followed a newer independent position/view/effect sample, so its
movement relative to the older player could vary between capture advances.
The native correction uses world.frame's actual camera position, view, final
effect-modified projection and F5 mode for the paired render plan. Existing
projection factoring converts effects to centimetres and retains native Z/W.
Both getters still consume an immutable copy; they never read world state.
The frame's capture identity is logged separately as render-capture and
applied-capture; control/applied-sequence still names the ownership feed.
Getter counters alone do not establish native scene use.

Capture identity does not renew the independent lease. Duplicated/stale control,
generation/anchor changes and invalid transforms still refuse/release through
existing gates. Camera/body/attachment rendering now advances at capture cadence;
the responsiveness cost and visible jitter reduction need live acceptance.
F5 visibility/displacement follow the captured actual camera, never player angles
or a guessed third-person offset. The fixed anchor is untouched.

`python3 tools/test-camera-body-time.py [archived-world.frame ...]` checks flying
body/camera relative projection, fixed captures despite newer control movement,
capture advances, actual captured F5/collision offsets/effects, native Z/W,
refusals and unchanged independent lease/freshness. Substituting the newer camera
position fails the numerical oracle. Fourteen actual captures replay successfully.
Existing guest-camera, body, viewport, depth-selection, player/paired controllers,
v2-v5 transport regressions and native Release build pass. All five shader
literals remain text-identical. This is not live visual acceptance.

After verified installation and manual reopen: one first-person calibration,
then walking/look and flying in rear/front F5 with a cape/held block. Check Steve
and attachments together, host foreground occlusion, first-person visibility and
F6 release. Save fresh capture identity/control/epoch evidence and original
screenshots. Avoid the synchronous detailed GPU trace while ACTIVE: it can stall
past the 500ms lease. No gameplay expansion is part of this correction.
