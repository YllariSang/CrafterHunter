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

**Steps 5 and 6, still blocked on a person driving MHW, now with a measured
cause.** Injected pointer input reaches MHW's camera *sometimes* and never
repeatably. With MHW focused and the cursor verified over its window,
`ydotool mousemove` twice rotated the camera — measured exactly, because the
anchor's projection walked from (483,406) to (1335,822) across one -600 px
move — but after any focus switch to Minecraft the identical moves leave MHW
pixel-identical (mean 0.00-0.14 over 3 s) while the game is still rendering
(6,790 pixels change in 3 s elsewhere) and the link stays
`LIVE | 12 pkt/s | age 1-4 ms`. A click in an empty area to re-grab the pointer
does not restore mouse-look, and `ydotool key` with W held for a full second
never moves the hunter, so translation and the F8 fresh anchor stay
untestable. Keys to Minecraft also need focus: `send_shortcut` aimed at an
unfocused Minecraft silently no-ops, which is why F7 had to be driven with that
window focused. `wtype` still never reaches either game (XWayland).

Two halves of the question were answered without a person:

- **Minecraft does apply the link's pose.** Toggling F7 with Minecraft focused
  flips the HUD to `OFF` and visibly changes its world view (mean 8.45 over
  3,514 pixels), and back to `LIVE`. Still unverified is the *direction* of the
  response: Minecraft's view is a featureless dark grass field, so a yaw change
  cannot be read off the pixels, and MHW would not accept the input needed to
  drive a controlled rotation while it was watched.
- **The placement tracks rotation correctly.** After the -600 px rotation the
  log still reads `anchored` — no invalidation since 04:46:10, which is the
  specified behaviour because rotation must not displace the camera — and the
  stone moved on screen exactly as the new projection predicts: a box at
  (1335,822), clear of the pinned window, is 4200/4200 neutral texels with
  `127,127,127` and `143,143,143` at the top of the histogram, while the old
  (483,406) position now holds only dark scene.

**Step 9, world leave and rejoin verified.** Done with injected input alone:
Hyprland `send_shortcut{window=<window>}` opens Minecraft's pause menu and
`ydotool` reaches it through uinput, so mouse moves plus `ydotool click 0xC0`
selected *Save and Quit to Title*, *Singleplayer*, the world entry and *Play
Selected World*. The world unloaded at 06:04:11 and reloaded at 06:10:40
(`~/.minecraft/logs/latest.log`), and by 06:11:49 the HUD read
`LIVE | 16-17 pkt/s` with the sequence advancing and age under 68 ms — the
link came back without restarting either game or the bridge.

`control-mhw-renderer.py capture` at 06:24 gives the placement half: the
`enabled=1` branch was taken with `parameters.centre` still holding
`<-378.6784, -462.9005, -164.4717>`, and the texels where that anchor projects
(row-vector `v * viewProjection`, screen 483,406) are exactly `127,127,127`,
`143,143,143`, `114,114,114` — the Minecraft stone palette, which the
surrounding Astera frame never produces. The placement log shows no
invalidation since `anchored` at 04:46:10, so the stone survived the bridge
outage, the world unload and the reload
(`11-stone-still-rendered-after-rejoin.png`).

Still open from step 9: the dimension change and the fifteen-minute expedition,
both of which need a person driving MHW.

This milestone renders the status and camera in Minecraft. Steve replacement,
Minecraft HUD inside MHW, and rendering Minecraft blocks in MHW are later work.
The camera can travel beyond the Minecraft player's loaded chunks because the
player remains stationary. Keep movement local and use F8; chunk streaming and
gameplay entity synchronization are not part of v0.2. An MHW area transition
can also create a large relative jump; re-anchor after the loading screen.
