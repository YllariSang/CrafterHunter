# Camera link v0.2 runtime test

Build verification: all client sources compiled against Loom's cached Minecraft
26.2 classes using Java target 25 with warnings treated as errors. The headless
camera behavior checks passed. Gradle could not start inside the restricted
sandbox (its FileLockContentionHandler requires a socket). The supplied 0.2 JAR
was assembled from those compiled classes and the mod resources; a normal
`./gradlew build` and actual game acceptance remain to be verified by the user.

1. Close Minecraft before replacing its mod. Keep exactly one CrafterHunter JAR
   in the mods folder: `crafterhunter-fabric-0.2.0.jar`. Preserve the old JAR
   outside that folder if you want a rollback.
2. Start the bridge, then MHW and Minecraft through your usual launch workflow.
   Enter a safe Minecraft world, preferably creative/spectator, and an MHW
   expedition. Allow MHW's ten-second startup grace to expire.
3. Disable Minecraft focus pausing with F3+P and resume its world. Put both
   windows side by side. A paused server log alone does not prove rendering or
   packet reception stopped; use the HUD and visible camera response as evidence.
4. Expect `MHW LINK: LIVE` and about 20 valid camera packets per second, with
   increasing sequence numbers and age normally below 100 ms. `WAITING` means
   no fresh valid camera sample; registration alone is not sufficient.
5. Rotate MHW left/right and up/down. Minecraft should follow the same direction.
   Note inverted axes if any: the conversion still needs this real-game check.
6. Move a short distance in MHW. Minecraft's view should move relative to the
   initial Minecraft viewpoint, rather than jump to MHW's absolute coordinates.
   Press F8 to start from the current vanilla Minecraft camera again.
7. Press F7: HUD becomes `OFF` and Minecraft's own camera responds normally.
   Press it again: a fresh relative anchor is created.
8. Stop the bridge. Within 500 ms of the final valid sample, Minecraft must
   return to its normal camera and show `WAITING`. Restart the bridge and verify
   fresh packets resume and a new anchor is created.
9. Test leaving/rejoining a Minecraft world and changing dimension. The next
   live sample must anchor in the new world. Run an expedition for 15 minutes
   and record HUD rate, any exceptions, and visual stability.

## Observed results (2026-10-05)

Live run with MHW plugin 0.3.3, Fabric mod 0.3.0, bridge v0.1.0 on
`127.0.0.1:38470`. Screenshots are in
`native/mhw-renderer/build/evidence/camera-link-acceptance/` (gitignored).

**Step 3, layout.** Both games must share the active workspace. Hyprland does
not render a window on an inactive workspace, and MHW then stops producing
camera packets (step 4). Minecraft opened its pause menu whenever it lost
focus, so its world had to be resumed after each focus change; F3+P was not
applied during this run.

**Step 4, verified.** With MHW rendering, a 68 s / 30-sample OCR series read
`MHW LINK: LIVE | 16-18 pkt/s | seq increasing | age 3-55 ms`, held for every
sample (`04-side-by-side-live-17pkts.png`). The producer caps at 20 Hz, so the
observed rate tracks MHW's frame rate rather than a fixed value.

**Step 4, the `1 pkt/s` observation resolved.** With MHW on an inactive
workspace the HUD reports `1 pkt/s` and flaps between `LIVE` and `WAITING`,
because the age crosses the 500 ms freshness window
(`05-waiting-1pkts-mhw-off-workspace.png`). This is background throttling, not
a protocol defect: MHW blocks on Present when it is not rendered and the
camera producer runs at the frame rate. Read the HUD with MHW visible.

**Step 7, verified.** F7 changes the HUD to `OFF` while packets keep flowing
(16 pkt/s, sequence still advancing); the second press returns `LIVE`
(`06-f7-camera-off.png`). Note for injected checks: Minecraft runs on
XWayland, so `wtype` (Wayland virtual keyboard) does not reach it. Use
Hyprland's dispatcher instead:
`hyprctl dispatch 'hl.dsp.send_shortcut{mods="", key="F7", window=<window>}'`.

**Step 8, verified, both halves.** The bridge process died at 05:02:07 after a
healthy 03:06:07-05:02:07 stretch with zero retries. Minecraft logged
`Bridge I/O failed; retrying: java.net.PortUnreachableException` at 60/min and
the HUD showed `WAITING | 0 pkt/s | seq - | age -1ms`
(`01-bridge-down-hud.png`). Restarting the bridge logged `registered Mhw` and
`registered Minecraft` within seconds, Fabric logged a second
`Connected to crafterhunter-bridge/v1`, and the HUD returned to `LIVE`
without restarting either game (`02-recovery-hud-A.png`,
`03-recovery-hud-B.png`). MHW recovers silently: its socket never errored, so
it sent no second HelloAck and the ten-second camera grace was not re-applied;
the native placement stayed anchored across the outage.

**Steps 5 and 6, blocked, needs the player.** No injected input reaches the
Proton window. `ydotool key` (with `ydotoold` running, exit 0), Hyprland
`send_key_state` and `send_shortcut` targeted at the MHW window, and `wtype`
all left the game unchanged, while `ydotool mousemove` does move the system
cursor. uinput therefore reaches the compositor and the failure is specific to
delivery into Proton. Rotation/translation response, inverted axes, and the F8
fresh anchor still require a person driving MHW.

**Step 9, not run.** Leaving/rejoining a world, a dimension change, and the
15-minute expedition still need in-world navigation.

This milestone renders the status and camera in Minecraft. Steve replacement,
Minecraft HUD inside MHW, and rendering Minecraft blocks in MHW are later work.
The camera can travel beyond the Minecraft player's loaded chunks because the
player remains stationary. Keep movement local and use F8; chunk streaming and
gameplay entity synchronization are not part of v0.2. An MHW area transition
can also create a large relative jump; re-anchor after the loading screen.
