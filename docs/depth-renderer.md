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
