# Camera and player capture time

The ACTIVE paired rendering path now builds the host camera from the same
uploaded world capture as Steve, the cape and held item. The independent camera
feed still supplies request token, generation, advancing control sequence and
the unchanged 500ms lease. World capture receipt freshness remains 1000ms.
Snapshot and acknowledgement formats, source mesh, skin, shaders and depth tests
are unchanged. Standalone model/camera rendering still uses the fast feed.

User video `recording_2026-10-12_19.29.26.mp4` follows reactivation with token
8860886702079709933 at the same generation 152840515317/epoch 1378, without
recalibration. The 43.00s 1920x1080 recording has 1287 decoded frames. Reviewed
walking and forward/vertical flying in first/rear/front views; examined consecutive
frames support Steve/cape/held-block alignment. Foreground beams, railings and
the hunter cover Steve; striped/checkered fragments still occur. This does not
accept pixel-perfect dynamic depth, input responsiveness or all flicker removed.

Two first-to-rear transitions show a host-only extreme Steve close-up. It occupies
recorded frames at PTS 10.185744 and 10.205322, with normal framing at 10.222678,
then PTS 26.908567, normal again at 26.932811. Minecraft already has ordinary rear
framing. Original full-size frames and consecutive strips are saved under ignored
`captures/player-camera-occlusion/runtime-20261012-111306Z/video-192926/`, with
`video-review.json`, video identity and matching reactivation logs. The log window
derived from filename/activation UTC timestamps is approximate, not an exact
GPU/video frame correspondence. Recording cadence is not game Present fps.

Next bounded goal is the F5 transition flash: identify the camera actually used
for the host draw versus the capture/body selected at the transition, then choose
a fix. Host render-phase mismatch is a hypothesis, not a measured cause. Preserve
same-capture body/attachments, existing host/private guest depth, first-person
hiding, fixed anchor and independent lease; no synchronous detailed trace ACTIVE.
No F6 event is visible in this clip. Review-end replies remain advancing ACTIVE,
guest request present and filter armed; control was left as requested. No game
mutation or implementation change occurred during video review.

Fresh post-install observation is saved under ignored
`captures/player-camera-occlusion/runtime-20261012-111306Z/`. Installed guest,
managed and native artifacts and the loaded native shadow cmp-match the tested
outputs. Generation 152840515317, one calibration epoch 1378, token
5759468977933993933: 2313 samples over 254.90s, 2300 matching ACTIVE and 13
matching released acknowledgements, zero sampling errors or alignment
invalidations. All three F5 modes and 467 distinct body feet positions occur;
every captured body's generation/feet matches its atomic world capture. User
original screenshots show first-person body hiding and rear/front Steve.
Native captures were requested in each mode but may complete after a view change;
do not assume their mode from the containing folder alone.

The read-only centre probe accepts 2130 coherent third-person samples with 311
distinct feet positions; maximum absolute X/Y discrepancy is 0.000189/0.000089
pixels. It refuses 4741 first-person samples and 463 reads crossing a frame or
capture/plan update. This supports the implemented relative camera/body timing
correction, not exact GPU/display pairing, screen latency or visual acceptance.
Camera sequence 7640 temporarily stopped advancing before control released;
the guest request flag remained present, so this is **not F6 acceptance**.
Explicit cleanup then confirmed a fresh `armed=0` reply. Channels are preserved,
games remain open and control is OFF. User walking/flying jitter, cape/item,
occlusion and responsiveness reports remain pending; previous F6 acceptance
stands separately. Do not relax the independent lease to keep this request alive.

Verified native implementation 74f2437 is installed, with the expanded regression
corrected and passing at a722763. Both games exited under the user's explicit
closure permission; fresh process/window checks preceded installation. Guest and
managed artifacts are preserved; all three installed builds cmp-match their tested
outputs. Scoped backup/manifest:
`native/mhw-renderer/build/rollback.camera-body-time.bme7a_bt/`; pinned installer
also saved `rollback.q8frzx/`. Control starts OFF, normal shading/body separation,
full/player composition, zero floors and live channels preserved. Live walking,
flying, F5/F6, foreground occlusion and camera responsiveness remain pending.

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
