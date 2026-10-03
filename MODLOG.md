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
