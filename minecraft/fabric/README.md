# CrafterHunter Fabric endpoint

This module is pinned to Minecraft Java `26.2`, Fabric Loader `0.19.5`, and JDK
25. It registers with the local bridge and consumes MHW camera telemetry. A
client-only Mixin applies fresh MHW position, rotation, and vertical FOV samples
to Minecraft's camera. If packets stop for 500 ms, vanilla camera behavior is
restored automatically.

A second stream, `PlayerState`, carries the host hunter's position and rotation.
While it is live, a client-only Mixin aligns the Minecraft player with it:
position and facing are taken from the host each tick, and velocity and
accumulated fall distance are cleared so vanilla physics cannot apply a second
move for one the host already made. The mapping is relative — the first fresh
sample anchors wherever the player is standing, and one metre of hunter travel
becomes one block — because MHW's world sits hundreds of metres from anything in
a Minecraft world. When the stream ages out, the link is toggled off, or the
world changes, the mixin stops touching the player on that same tick and normal
keyboard control applies immediately.

Version 0.2 anchors the first MHW sample at Minecraft's current camera position.
One metre of subsequent MHW movement becomes one Minecraft block. Rotation
uses the existing quaternion-to-yaw/pitch mapping; roll is not applied. A 50 ms
time-based filter smooths position, yaw, pitch, and FOV. Returning from a stale
feed or changing worlds automatically creates a new anchor.

The top-left HUD shows two lines: `MHW LINK` for camera telemetry and `PLAYER`
for the host hunter stream, each `LIVE`, `WAITING`, or `OFF` with packet age,
plus the proxy target in Minecraft coordinates. `LIVE` confirms reception, not
visual rendering inside MHW. Press F7 to release or re-enable the Minecraft
camera, F8 to re-anchor it at the vanilla viewpoint, and F9 to release or
re-enable the player proxy. These controls work while in-world with no screen
open.

The camera controls never change gameplay input: the camera is a view, not the
player. The player proxy does move the Minecraft player while it is live, which
is the point of milestone 3 — F9 (or letting the stream age out) hands movement
straight back to the keyboard with no further correction.

Run the headless behavior checks from the project root:

```bash
bash tools/test-fabric-camera.sh
bash tools/test-fabric-player.sh
```

Both read one shared source list, `tools/fabric-headless-sources.txt`, so they
compile the same dependency set and cannot drift apart.

For runtime acceptance and known limits, see
[the v0.2 test procedure](../../docs/camera-link-test.md).

Do not run a development client without a legitimate Minecraft Java license or
an official trial that permits the intended testing. A third-party launcher is
not a substitute for ownership; it is acceptable only when it authenticates a
licensed Microsoft account.

Once the Gradle wrapper is present, build the standalone mod artifact with
`./gradlew build`; the output is written to `build/libs/`. The initial build
contacts Mojang, Fabric, and Gradle's artifact repositories.
