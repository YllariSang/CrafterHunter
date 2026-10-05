# CrafterHunter

CrafterHunter is an experimental, unofficial interoperability bridge between a
locally installed copy of **Monster Hunter: World – Iceborne** and a locally
installed, licensed copy of **Minecraft: Java Edition**.

The project does not contain or distribute either game's code, assets, keys, or
authentication material. It is not affiliated with or endorsed by Capcom,
Mojang, or Microsoft.

## Direction

**Minecraft is playable inside Monster Hunter: World.** You are Steve, standing
in Astera or the Wildspire Wetlands, and Monster Hunter's world is the world
around you: MHW owns the monsters, their AI and health, the quests and the story,
and Minecraft owns your body, your items, your damage, your shield, your elytra.
An iron sword damages a real MHW monster, a monster's claw hurts Steve, a raised
shield refuses it, TNT works. Both games never drive the player at once.

See [MODDING_PLAN.md](MODDING_PLAN.md) for the milestone list and the acceptance
contract that "playable" is measured against.

CrafterHunter is a Linux-first implementation for Steam Proton/DXVK. A prior
MIT-licensed project already demonstrates this crossover on Apple Silicon with
CrossOver/DXMT. CrafterHunter does not claim the original concept; its focus is
a reproducible Linux path, an independently designed protocol and tooling, and
fail-closed version handling. See [docs/prior-art.md](docs/prior-art.md).

## Current milestone

Milestones 1 and 2 are accepted in the real games: the transport and camera link
work, and the native DX11 renderer displays Minecraft geometry with fixed world
position, scene-depth occlusion, and drawing before MHW's HUD and cursor. The
native depth renderer has been validated on MHW build 421810 under Proton/DXVK
including across a loading transition and a dialogue cutscene, with 958 traced
frames each binding a same-frame depth at 0.000 px CPU/GPU projection agreement.
Live Fabric block-asset transfer is confirmed. Minecraft supplies its texture at
runtime; no Minecraft asset is bundled. See
[the rendering verification and setup](docs/depth-renderer.md).

Milestone 3 is the current work: **Playable Steve** — a visible, controllable
Minecraft player inside MHW — and it starts by derisking the one item whose cost
is unknown, compositing a full Minecraft frame at native resolution and 60 fps.
Terrain queries are answered today from the game's own collision routine; see
[docs/terrain-query.md](docs/terrain-query.md).

Collision, Steve's model, inventory integration, and cross-game combat are still
future work. The earlier [A/B/C comparison](docs/block-compare-test.md) remains
documented as development history.

The repository includes:

- `crafterhunter-bridge`: a local UDP router written in Rust.
- `crafterhunter-probe`: a synthetic endpoint used to test both game roles.
- `minecraft/fabric`: the Fabric camera link and runtime block-asset sender.
- `native/mhw-spl-plugin`: the supported MHW transport endpoint, loaded by
  SharpPluginLoader on Proton.
- `native/mhw-renderer`: the opt-in DX11 stone renderer using observed MHW
  scene depth and the current GPU camera constants.
- `native/mhw-plugin`: an experimental native DLL retained for future graphics
  work; it is not the current installation path.
- `docs/protocol.md`: the versioned wire protocol shared by all components.

Gameplay modification is not implemented yet. The Proton loader checkpoint is
proven on MHW build 421810; the endpoint publishes read-only camera telemetry
and connects the runtime block-asset receiver to the opt-in native renderer.

## Build the testable core

Rust 1.70 or newer is sufficient. The Rust workspace deliberately has no
third-party dependencies.

```bash
cargo test --workspace
cargo build --workspace
```

Run the bridge:

```bash
cargo run -p crafterhunter-bridge
```

In two other terminals, start synthetic game endpoints:

```bash
cargo run -p crafterhunter-probe -- mhw
cargo run -p crafterhunter-probe -- minecraft
```

Each probe sends a hello and heartbeat. Once both are registered, the bridge
forwards game packets between them.

On Linux or WSL, the same check is automated by:

```bash
./tools/smoke-test.sh
```

The script requires permission to open localhost UDP sockets.

## Identify an MHW build safely

The native adapter must be pinned to the exact executable build. On Windows,
run the read-only fingerprint tool from this repository:

```powershell
.\tools\fingerprint-mhw.ps1 "C:\Program Files (x86)\Steam\steamapps\common\Monster Hunter World"
```

Share only its text output. Never upload `MonsterHunterWorld.exe`.

On Linux/Steam Proton, use:

```bash
./tools/fingerprint-mhw.sh
```

If MHW is in a secondary Steam library, pass the directory shown by Steam's
**Properties > Installed Files > Browse**:

```bash
./tools/fingerprint-mhw.sh "/games/SteamLibrary/steamapps/common/Monster Hunter World"
```

## First Proton integration check

CrafterHunter uses
[SharpPluginLoader](https://github.com/Fexty12573/SharpPluginLoader) for the
initial managed MHW adapter because its Linux release officially supports
Proton/Wine. Install SharpPluginLoader's official Linux package and its Proton
dependencies according to its documentation. Then build and install our
endpoint:

```bash
./tools/doctor-linux.sh
./tools/build-mhw-spl.sh
./tools/install-mhw-spl-plugin.sh install
cargo run -p crafterhunter-bridge
```

Set MHW's Steam launch options to the value printed by the installer. With the
bridge running, launch MHW in offline/private mode. The bridge should print a
`registered Mhw` line. Remove only our endpoint with:

```bash
./tools/install-mhw-spl-plugin.sh remove
```

The installer never installs or removes SharpPluginLoader itself.

### Verify live MHW camera telemetry

With the bridge and MHW running, install the latest managed plugin build:

```bash
./tools/build-mhw-spl.sh
./tools/install-mhw-spl-plugin.sh install
```

SharpPluginLoader normally hot-reloads the changed DLL. If it does not, restart
MHW. In another terminal, use the Minecraft-role probe as a temporary receiver:

```bash
cargo run -p crafterhunter-probe -- minecraft
```

It should print camera position in metres, rotation, vertical FOV, aspect ratio,
and clip planes about four times per second. Moving the MHW camera should change
the reported values. This verifies read-only telemetry without requiring or
launching Minecraft.

## Project boundaries

- Users must supply legitimate installations of both games.
- CrafterHunter never downloads, patches, or redistributes either game.
- Authentication and DRM are out of scope.
- Development targets offline/private play first.
- Public multiplayer and competitive advantages are out of scope.
- Proprietary assets must never be checked into this repository.

See [docs/architecture.md](docs/architecture.md) and
[docs/legal-boundaries.md](docs/legal-boundaries.md).

## Credits

- [SharpPluginLoader](https://github.com/Fexty12573/SharpPluginLoader) provides
  the MHW loader and managed API.
- [MinHook](https://github.com/TsudaKageyu/minhook) provides native hooks; its
  redistribution notice is included by the renderer build/install scripts.
- [Fabric](https://fabricmc.net/) and Gradle provide the Minecraft build tooling.
- [minecraft-crossover-bridge](https://github.com/justbustin/minecraft-crossover-bridge)
  informed the interoperability and depth-rendering approach; see
  [prior art](docs/prior-art.md).
- Development includes assistance from OpenAI Codex. The logo source was supplied
  by the project's author.

## License

No public software license has been selected yet. All original CrafterHunter
code is currently reserved pending that decision.
