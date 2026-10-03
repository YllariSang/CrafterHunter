# Prior art and project scope

CrafterHunter is not the first implementation of Minecraft inside Monster
Hunter: World. The MIT-licensed
[`justbustin/minecraft-crossover-bridge`](https://github.com/justbustin/minecraft-crossover-bridge)
publicly demonstrates the concept on Apple Silicon using CrossOver and DXMT.
It includes shared-memory frame transfer, camera synchronization, MHW terrain
rays, monster proxies, damage exchange, and Direct3D 11 composition.

That project is valuable prior art and proof that the concept is technically
possible. CrafterHunter must credit it in public descriptions and must not claim
to be a clean-room implementation: contributors have inspected its public
documentation and portions of its MIT-licensed source.

## Why CrafterHunter remains useful

CrafterHunter targets a different primary environment and engineering shape:

| Area | Existing crossover | CrafterHunter target |
|---|---|---|
| Host | Apple Silicon macOS | Linux and Steam Deck class systems |
| Windows compatibility | CrossOver with DXMT | Steam Proton with DXVK |
| Control transport | Shared mapped files | Versioned localhost protocol |
| Initial MHW lifecycle | Custom `dinput8.dll` proxy | SharpPluginLoader's supported Proton path |
| Testing | Both games required | Synthetic endpoints before game integration |
| Installation | CrossOver bottle scripts | Detectable, reversible Steam-library tooling |

High-bandwidth color and depth frames will need shared memory or another local
zero-copy mechanism; UDP remains the control and low-rate telemetry plane, not
the final video transport.

## Reuse policy

- Ideas, published observations, and interoperability facts should be cited.
- Reused source must be clearly identified and retain every required MIT or
  third-party notice.
- Original CrafterHunter code must not silently copy upstream implementation
  details.
- No game binaries, extracted assets, credentials, or authentication bypasses
  belong in this repository.
- Minecraft integration is tested only with a legitimately licensed account or
  an official trial that permits the test.

This is an engineering policy, not a statement about ownership of the general
crossover idea and not legal advice.
