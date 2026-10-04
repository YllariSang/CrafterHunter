# Depth-aware rendering path

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
