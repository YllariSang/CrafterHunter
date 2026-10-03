# CrafterHunter Fabric endpoint

This module is pinned to Minecraft Java `26.2`, Fabric Loader `0.19.5`, and JDK
25. It registers with the local bridge and consumes MHW camera telemetry. A
client-only Mixin applies fresh MHW position, rotation, and vertical FOV samples
to Minecraft's camera. If packets stop for 500 ms, vanilla camera behavior is
restored automatically.

Do not run a development client without a legitimate Minecraft Java license or
an official trial that permits the intended testing. A third-party launcher is
not a substitute for ownership; it is acceptable only when it authenticates a
licensed Microsoft account.

Once the Gradle wrapper is present, build the standalone mod artifact with
`./gradlew build`; the output is written to `build/libs/`. The initial build
contacts Mojang, Fabric, and Gradle's artifact repositories.
