# F5 transition drawing

The native paired renderer now holds guest world/body drawing across an F5 mode
change until the host frame's supplied camera agrees with a camera plan of the
selected capture's mode. This addresses the brief huge Steve close-up exposed
by the user's 43s recording `recording_2026-10-12_19.29.26.mp4`, at PTS
10.185744/10.205322 and 26.908567. Minecraft already shows a normal rear view.
See [camera/body timing](camera-body-time.md) for the preceding correction.

The managed caller computes `MainViewport.ViewMatrix * ProjectionMatrix` before
calling `CH_Frame`. Native then uploads the newest capture and selects its camera
plan. Selecting a rear body while that supplied matrix still represents first
person puts its face near the old eye plane. Merely reading the latest getter's
applied capture does not identify the matrix already supplied to the frame.

Read-only sampling of our loaded renderer after the requested F5 cycle has 6081
coherent observations and nine refused reads. Nine samples spanning three
transitions match an older-mode supplied CPU matrix despite a newer selected
capture/plan; first-to-rear differs by approximately 400 centimetres in the W-row
translation. The matching prior matrix's largest X/Y/W error is approximately
0.000002. Evidence and probe source are ignored under
`captures/player-camera-occlusion/runtime-20261012-111306Z/`, named
`f5-camera-frame-samples.json` and `f5-camera-frame-report.json`. This measures the
CPU matrix handed to native, not a simultaneous GPU camera/depth capture.

`guest_camera_frame.hpp` retains at most 32 small CPU plans. New ownership token,
generation or anchor epoch clears the mode state. At frame setup, a new F5 mode
must match the supplied matrix's X/Y/W rows using the actual host aspect and a
float-product rounding bound. Native Z terms are neither compared to guest depth
nor modified. A prior plan of the selected mode may match when movement has
already advanced the capture; once that mode is established, same-mode walking,
look and flying do not wait for exact newest-camera equality. No capture, pose or
upload floor is added, and first-person body hiding stays in place.

Both full/preview guest composition and the separate body consult the same
per-frame decision. A held transition draws the ordinary MHW scene for that
frame; the guest surfaces, attachments and body resume together. A full paired
refusal previously selected the sky overlay if preview was disabled; the fallback
now permits only explicitly enabled preview, or skips. The independent standalone
overlay remains available when neither paired mode is requested.

Independent camera ownership and its 500ms lease, 1000ms world receipt freshness,
fixed F5 anchor, GPU capture lifetime, atomic channel/body binding, host viewport
conversion and host/private guest depth rules are unchanged. All five HLSL
literals are text-identical. No new game hook, protocol, guest or managed build.
Frame-gate logs report selected and last accepted modes plus readiness, not an
assertion that all GPU consumers use that matrix.

`python3 tools/test-f5-frame.py [own-renderer-observation.json]` checks the
near-eye enlargement numerically, all F5 transitions, moving-plan history,
ownership/expiry/invalid refusal, asymmetric/effect/large-origin float products,
native Z independence and opt-in fallback routing. All nine measured mismatches
replay. Restoring premature mode admission or the old sky fallback fails the
regression. Native Release build and relevant camera/body, viewport, coverage,
preview/controller and composition-request checks pass.

Verified native implementation 28c19e5 is installed after exact game closure and
fresh process/window checks. Scoped backup/manifest:
`native/mhw-renderer/build/rollback.f5-frame.zbap48nt/`; pinned installer also
saved `rollback.ffR6lo/`. All three installed artifacts cmp-match tested builds;
guest/managed preserved. Normal window closure was unavailable (legacy Hyprland
dispatcher rejected, xdotool absent); PID-pinned SIGTERM closed only MHW 4803 and
Minecraft 4710 without SIGKILL. Control OFF; composition/body separation and zero
floors preserved, channels retained, no game launches. Visible flash removal
remains pending a fresh manual restart, one first-person
calibration and repeated F5 transitions while still/walking/flying, including
cape/held item and F6. The CPU guard does not itself prove exact GPU frame pairing
or solve every dynamic occlusion artefact. Do not run synchronous detailed GPU
trace while ACTIVE; preserve the independent lease.
