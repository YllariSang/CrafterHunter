# Minecraft inside Monster Hunter: World – Iceborne

Recon date: 2026-10-04. This is an implementation plan, not a claim that
gameplay integration works.

## Target and current evidence

Run real Minecraft Java alongside MHW, sharing a playable world with building,
monster combat, and participation in MHW's existing quests and story.
This matches the repository's existing separate-process architecture.

- Host installation: `/home/yllaris/.local/share/Steam/steamapps/common/Monster Hunter World`.
- Host executable: 84,225,952 bytes; SHA-256
  `c2ebbbd2c49f216d484e31a5219bed419eb1e5e7d206d02cba040a3ab79d90ea`.
- Steam build ID: 15539686. Previous recorded window build: 421810.
- Runtime: Linux, Steam Proton/DXVK; native Windows host code.
- Existing loader: SharpPluginLoader; managed endpoint references Core 1.0.0.
  Loader bootstrap and installed endpoint were detected by `tools/doctor-linux.sh`.
- Guest source target: Minecraft 26.2, Fabric Loader 0.19.5,
  Loom 1.18-SNAPSHOT. Launcher authentication and live guest compatibility
  remain to be confirmed.
- Implemented: localhost transport, read-only MHW camera telemetry, Fabric
  camera-follow code. Live camera acceptance is still pending.
- Missing: frame composition, terrain collision, shared player control,
  monster proxies, damage exchange, quest and story integration.

## Route and ownership

Keep the Rust bridge and SPL lifecycle boundary. Add a separate native D3D11
composition component, with bounded shared frame buffers for color/depth.
UDP carries control and telemetry, never full video frames.

MHW owns monster AI, monster health, hit reactions, quest success/failure,
story flags, NPC interactions, rewards, and area transitions. Minecraft owns
blocks, block inventory, crafting, and Minecraft simulation. Player movement,
input, camera, and player health need an explicitly negotiated owner: the
current camera-follow milestone gives control to MHW; a Minecraft-controlled
playable mode is a later feature and must not have both games drive the player.

## Ordered acceptance milestones

1. **Camera acceptance:** finish `docs/camera-link-test.md` in both games;
   verify axes, scale, transitions, and disconnect recovery.
2. **Composition:** show one Minecraft block inside an MHW expedition,
   correctly hidden by MHW terrain at different camera angles. Verify the
   actual DXVK color/depth resources and frame synchronization first.
3. **Terrain and player:** bounded host terrain queries feed Minecraft collision;
   align a host player proxy with the Minecraft player; verify ground, slopes,
   movement, loading screens, chunk streaming, and immediate control fallback.
4. **One monster:** export a stable session entity ID, transform, hit volumes,
   and health. One Minecraft attack produces one validated host combat action;
   one monster attack produces one player hit. MHW executes its normal death
   and quest logic. Verify duplicates, stale targets, disconnects, and despawns.
5. **Building:** place and break blocks, persist them by host area/session,
   and validate interaction with host creatures. Guest collision alone does
   not make monsters collide with blocks; host collision or validated damage
   queries require a separate adapter. Test a monster breaking one structure.
6. **One assigned quest:** start, enter, complete, fail, abandon, and return
   through normal MHW flow. Minecraft combat must satisfy the host objective;
   verify rewards and progression exactly once. Scene changes invalidate old
   entity IDs, commands, and coordinate anchors.
7. **Story participation:** support one dialogue/cutscene sequence and its
   following quest. Yield input and camera to MHW during scripted sequences,
   hide the Minecraft layer when necessary, then resume with a fresh anchor.
   Track compatibility per quest before expanding to the campaign.
8. **Broader interaction:** monster parts, statuses, capture, gathering, NPCs,
   crafting exchanges, and environmental effects each get a dedicated mapping
   and a real-game acceptance scene. Cross-game item conversions need a
   transaction identity to prevent duplicate rewards.

## Quest and story boundary

SPL documents quest accept/enter/leave/complete/fail and monster lifecycle
callbacks. These are useful integration points, but do not prove cutscene,
dialogue, collision, or damage support on this installed build. Inspect the
pinned loader implementation before using each API in process.

"Interacts with everything" is the long-term target. It needs a compatibility
matrix for quests, monsters, NPCs, and areas. Completing one quest through the
bridge cannot establish whole-campaign support. Preserve native quest logic
instead of writing story-completion flags from Minecraft.

## Lab and unresolved checks

Use owned installations and an offline test save. Confirm ownership and the
guest launcher/profile with the user before game integration tests.
Locate and back up MHW's Steam `582010` save data and the selected Minecraft
world before enabling gameplay writes; record exact backup and restore paths.
The current recon changes no installed game files or saves.

Before native work, establish runtime feature checks against the pinned build,
game-thread command execution, bounded queues, duplicate-resistant gameplay
commands, session/area epochs, and a disconnect watchdog. Unsupported features
must disable themselves. Do not use raw health writes as a substitute for the
host's combat and quest bookkeeping.

## Sources

- Existing local evidence: `docs/tested-builds.md`, `docs/camera-link-test.md`,
  and the installed-game fingerprint tool.
- [SPL plugin callbacks](https://fexty12573.github.io/SharpPluginLoader/API/SharpPluginLoader.Core.IPlugin.html).
- [SPL monster API](https://fexty12573.github.io/SharpPluginLoader/API/SharpPluginLoader.Core.Entities.Monster.html).
- [Existing macOS crossover](https://github.com/justbustin/minecraft-crossover-bridge):
  demonstrates composition, terrain rays, monster proxies, and damage exchange;
  does not establish Linux/DXVK or full story compatibility. Any reused source
  must retain its license notices; see `docs/prior-art.md`.
- Universal-modder's local knowledge index currently has no MHW-specific note.
