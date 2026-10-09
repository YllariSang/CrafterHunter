# Exclusive input: reference investigation

2026-10-09. Status: investigation only; exclusive input is NOT implemented.
Rendering, pose cadence and installed game binaries are unchanged.

## Reference and history

Inspected MIT reference at commit
`d9cac4690682131140f736f9ec60302b877e258d` (copyright 2026 justbustin).
GitHub path history for both game.cpp and Overlay.java returned only that initial
publication commit; no earlier implementation/restoration history was exposed.
This document describes observed source, not a runtime test of the reference.
No reference implementation code was copied.

- [MHW README](https://github.com/justbustin/minecraft-crossover-bridge/blob/d9cac4690682131140f736f9ec60302b877e258d/monster-hunter-world/README.md)
- [Design](https://github.com/justbustin/minecraft-crossover-bridge/blob/d9cac4690682131140f736f9ec60302b877e258d/docs/how-it-works.md)
- [Host handle_switching](https://github.com/justbustin/minecraft-crossover-bridge/blob/d9cac4690682131140f736f9ec60302b877e258d/monster-hunter-world/mhw-bridge/src/game.cpp)
- [Guest Overlay](https://github.com/justbustin/minecraft-crossover-bridge/blob/d9cac4690682131140f736f9ec60302b877e258d/monster-hunter-world/mc-bridge/src/client/java/dev/mhwmc/bridge/client/Overlay.java)
- [Guest key bindings](https://github.com/justbustin/minecraft-crossover-bridge/blob/d9cac4690682131140f736f9ec60302b877e258d/monster-hunter-world/mc-bridge/src/client/java/dev/mhwmc/bridge/client/MhwBridgeClient.java)
- [DirectInput forwarding proxy](https://github.com/justbustin/minecraft-crossover-bridge/blob/d9cac4690682131140f736f9ec60302b877e258d/monster-hunter-world/mhw-bridge/src/proxy.cpp)

## Actual mechanism

Minecraft registers F8 as a Fabric key binding. switchToMhw is idempotent:
it sets mhwMode, opens the pause screen when appropriate, releases Minecraft's
mouse, hides its GLFW window and increments a shared-memory MHW-focus request.
The host sees a changed request counter and calls ShowWindow(SW_RESTORE),
SetForegroundWindow, SetActiveWindow and SetFocus. It logs whether foreground
focus actually succeeded, rather than treating the request as guaranteed.

Host F8 is GetAsyncKeyState(VK_F8), gated by the MHW foreground window and a
rising-edge latch. It increments the shared-memory Minecraft-switch counter.
The guest polls that counter, shows/focuses its GLFW window and dismisses its
pause screen if present. No native input filter is enabled/disabled in these
handoff functions, so restoration here is focus restoration, not undoing a
hunter-input patch.

While Minecraft owns focus, its transparent borderless floating window lies over
MHW. macOS NSWindow setIgnoresMouseEvents(false) prevents clicks falling through.
GLFW handles Minecraft's own mouse capture. The proxy forwards DirectInput8Create
to the real library; it is not itself an input-suppression device wrapper.

Menu access is achieved by handing focus back to MHW, not by keeping MHW UI
inputs selectively available while hunter inputs are filtered. The inspected
handoff has no controller polling/filtering or controller ownership handshake.
Do not infer that background MHW gamepad input is suppressed.

## Transferability and missing prerequisite

GLFW window calls and edge-triggered request/acknowledgment concepts can inform a
Linux implementation. Cocoa mouse-event policy and CrossOver foreground behavior
are platform-specific. Wine foreground requests and native Linux Minecraft focus
must not be assumed to work identically under Wayland. No desktop configuration
or focus automation is introduced here. DX11/DXVK rendering is unrelated to this
input boundary and requires no changes.

Installed SPL Core 1.0.0 XML documents IO.Input state queries, not hunter-only
suppression. Upstream Input.cs reads sMhKeyboard/sMhSteamController. Upstream
CoreModule.cpp invokes plugins before the original main update; this does not
establish whether native device polling occurs before or after the callback, or
which hunter consumer handles keyboard, mouse and controller actions.

Required prerequisite: verify the pinned build's hunter gameplay input consumer
or a supported suppression API, including its calling convention, identity,
thread/order and separation from menu/camera input. A focus handoff is not proof
of that boundary. No global input clearing or speculative hook is installed.

Once established, suppression must be scoped to the local hunter, disabled by
default, with a reachable disable path, idempotent transitions and restoration
on unload/error. Ownership should expire on stale guest health if its safety
depends on a live guest; a timeout must use advancing session/sequence evidence,
not merely the presence of an old file. Held-button transitions, player
replacement/loading, hook lifetime and release-before-reenable require tests.
Controller coverage must remain explicitly unsupported until tested.

## Acceptance plan (not yet executable)

1. Startup: normal MHW keyboard/mouse movement/actions; no suppression.
2. Enable: acknowledged owner state; Minecraft moves Steve, hunter stays still
   and does not attack/use items. Do not confuse lack of focus with suppression.
3. Disable: immediately regain MHW input, including after a held key/button.
4. Repeat toggles and duplicate commands: no stacked hooks/stuck buttons.
5. Test menus/UI separately; record which remain usable in each mode.
6. Repeat keyboard/mouse checks with controller only if support is implemented.
7. Disconnect guest/bridge, then unload plugin while enabled: verify restoration
   for each supported path and no callback into unloaded code.
8. Check player/skin/pose/orbit/occlusion against the accepted renderer. Measure
   frame times in the same scene/focus state before and after, not just Steve's
   intentionally 4 Hz animation cadence.

No implementation/lifecycle tests or live ownership acceptance are claimed.
