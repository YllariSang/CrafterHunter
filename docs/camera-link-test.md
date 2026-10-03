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

This milestone renders the status and camera in Minecraft. Steve replacement,
Minecraft HUD inside MHW, and rendering Minecraft blocks in MHW are later work.
The camera can travel beyond the Minecraft player's loaded chunks because the
player remains stationary. Keep movement local and use F8; chunk streaming and
gameplay entity synchronization are not part of v0.2. An MHW area transition
can also create a large relative jump; re-anchor after the loading screen.
