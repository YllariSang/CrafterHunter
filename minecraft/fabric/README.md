# CrafterHunter Fabric endpoint

This module is pinned to Minecraft Java `26.2`, Fabric Loader `0.19.5`, and JDK
25. It registers with the local bridge and consumes MHW camera telemetry. A
client-only Mixin applies fresh MHW position, rotation, and vertical FOV samples
to Minecraft's camera. If packets stop for 500 ms, vanilla camera behavior is
restored automatically.

Version 0.2 anchors the first MHW sample at Minecraft's current camera position.
One metre of subsequent MHW movement becomes one Minecraft block. Rotation
uses the existing quaternion-to-yaw/pitch mapping; roll is not applied. A 50 ms
time-based filter smooths position, yaw, pitch, and FOV. Returning from a stale
feed or changing worlds automatically creates a new anchor.

The top-left HUD shows `LIVE`, `WAITING`, or `OFF`, plus valid camera packets per
second, sequence, and packet age. `LIVE` confirms reception, not visual rendering
inside MHW. Press F7 to release/re-enable the Minecraft camera and F8 to re-anchor
at the vanilla Minecraft viewpoint. These controls work while in-world with no
screen open. They do not move the Minecraft player or change gameplay input.

Run the headless camera behavior checks from the project root:

```bash
bash tools/test-fabric-camera.sh
```

For runtime acceptance and known limits, see
[the v0.2 test procedure](../../docs/camera-link-test.md).

Do not run a development client without a legitimate Minecraft Java license or
an official trial that permits the intended testing. A third-party launcher is
not a substitute for ownership; it is acceptable only when it authenticates a
licensed Microsoft account.

Once the Gradle wrapper is present, build the standalone mod artifact with
`./gradlew build`; the output is written to `build/libs/`. The initial build
contacts Mojang, Fabric, and Gradle's artifact repositories.
