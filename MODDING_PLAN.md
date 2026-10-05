# Minecraft inside Monster Hunter: World – Iceborne

Recon date: 2026-10-04. Direction corrected 2026-10-05. This is an
implementation plan, not a claim that gameplay integration works.

## Target

**Minecraft is playable inside Monster Hunter: World.** You are Steve, standing
in Astera or the Wildspire Wetlands, and Monster Hunter's world is the world
around you.

Concretely, and this is the contract the milestones below are written to satisfy:

- You control Steve with Minecraft's own input, movement, and physics.
- Steve's body, blocks, items, mobs, and particles are visible inside MHW's
  frame, occluded correctly by MHW geometry.
- Steve stands on MHW's ground and is stopped by MHW's cliffs, not on
  Minecraft's flat world with Monster Hunter's mountains passing through him.
- Minecraft's own verbs work against MHW's world: an iron sword damages a real
  MHW monster, TNT damages a real MHW monster, a monster's claw damages Steve,
  a Minecraft shield refuses that claw, elytra fly in MHW's sky.
- MHW keeps its AI, monster health, quests, story, and area transitions. Nothing
  is faked in Minecraft's direction; MHW does the bookkeeping.
- **Both games never drive the player at the same time.** Control has exactly
  one owner at every instant, and it is visible in the HUD which one.

Everything Minecraft-related interacts with MHW, and nothing Minecraft-related
overrides MHW's own logic.

## Target and current evidence

Run real Minecraft Java alongside MHW. This matches the repository's existing
separate-process architecture.

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
- Implemented and accepted live: localhost transport, read-only MHW camera
  telemetry driving the Minecraft camera, Minecraft colour/depth composition into
  MHW's renderer with validated occlusion and frame synchronization, a player
  proxy, and host terrain queries answered from the game's own collision routine.
- Missing: a visible, controllable Steve inside MHW, Minecraft collision on MHW
  ground, monster combat in both directions, building, quest and story
  integration.

## Route and ownership

Keep the Rust bridge and SPL lifecycle boundary. Add a separate native D3D11
composition component, with bounded shared frame buffers for color/depth.
UDP carries control and telemetry, never full video frames.

**Ownership is the keystone of the new direction.** The camera-follow milestone
built one mode: MHW owns the player, and Minecraft mirrors it. That mode stays —
it is a working spectator view and the evidence base for everything else — but
the playable goal needs the other mode: Minecraft owns input, movement and
camera, and MHW renders from Steve's eye. The two are mutually exclusive by
construction:

| | MHW-owned (today) | Minecraft-owned (the goal) |
| --- | --- | --- |
| Input | MHW | Minecraft |
| Movement | MHW | Minecraft |
| Camera | MHW drives the guest | Minecraft drives the host |
| Minecraft player | proxy mirroring the hunter | the player |
| Entered by | default | guest requests, host acknowledges |

The owner flag is negotiated, never inferred: the guest asks, the host
acknowledges, and both sides refuse to drive until the handshake completes. A
disconnect, a stale frame, or a scene change hands control back within the
freshness window without a Minecraft restart. Scripted sequences in MHW take
control back for their duration and return it with a fresh anchor.

MHW owns monster AI, monster health, hit reactions, quest success/failure,
story flags, NPC interactions, rewards, and area transitions. Minecraft owns
blocks, block inventory, crafting, and Minecraft simulation.

## Ordered acceptance milestones

1. **Camera acceptance:** finish `docs/camera-link-test.md` in both games;
   verify axes, scale, transitions, and disconnect recovery.
   *Status 2026-10-05: pass. All nine steps recorded in
   `docs/camera-link-test.md`, with axes, transitions and re-anchoring driven
   by the person against the live games.*
2. **Composition:** show one Minecraft block inside an MHW expedition,
   correctly hidden by MHW terrain at different camera angles. Verify the
   actual DXVK color/depth resources and frame synchronization first.
   *Status 2026-10-05: pass, cutscene case included. The person reports orbit,
   occlusion and UI/cursor ordering passing against the live games, with the
   stone fed by the running Minecraft client. Three recordings cover the rest
   and are detailed in `docs/depth-renderer.md`: the link held 16–18 pkt/s
   across a loading transition and two cutscenes (age ≤29 ms) and a 13 s clip
   shows the stone composited in normal play; a 101 s dialogue cutscene shows
   the cube behind the cutscene characters with them drawing in front of it,
   and the depth bound during that cutscene held its geometry, so the
   fresh-but-empty hazard did not fire. Three single-frame >25 m teleports
   invalidated placement as specified. Both follow-ups are now **closed**:
   the depth-resource selection rule (`selection.hpp`, 13 off-target checks)
   and the frame-synchronization trace (`framesync` plus
   `tools/inspect-framesync.py`) were validated against the running games —
   composition failed closed until the first read-back, then bound a fresh
   measured candidate while refusing a fresh-but-empty one measured at
   `age=0`, and 958 traced frames each bound a same-frame depth with 0.000 px
   CPU/GPU projection agreement across 37 camera states. Yielding input and
   camera, and the wider story flow, remain milestone 8's.*
3. **Playable Steve — a visible, controllable Minecraft player inside MHW.** The
   milestone the whole project was aimed at, and the first one that can fail
   outright, so it is derisked first:
   - **Spike, before anything else:** composite a *full* Minecraft frame into
     MHW's renderer at native resolution and 60 fps, with the player model
     visible, instead of one depth-selected block. Everything after this depends
     on it, and it is the only item here with an unknown cost.
   - **Ownership handshake:** guest requests control, host acknowledges, the HUD
     shows the owner, and no side drives the player until it completes.
   - **Minecraft-driven camera:** Steve's eye and yaw become MHW's camera, using
   the rotation conversion the camera link already proves, in the other
   direction.
   - **Acceptance, in the running games:** you stand in an expedition as Steve,
   move with Minecraft input, MHW renders the world from your eyes, MHW input
   does not move you, Minecraft is not overriding you, control returns to MHW and
   back within the freshness window, and a scene change re-anchors instead of
   teleporting.
4. **Terrain under Steve:** MHW's ground is the ground Steve walks on. The host
   side is already proven — see `docs/terrain-query.md`: the game's own
   collision routine, resolved from byte signatures in the loaded image behind an
   executable fingerprint, hit heights matching the hunter's own to within 11 cm
   on five surfaces from flat ground to a 22-degree hillside. What remains is the
   guest side and the live acceptance.
   - Map column heights into Minecraft coordinates through the proxy anchor, and
     pace requests on a bounded budget rather than per frame.
   - Build slope from neighbouring column heights. **Not from the surface
     normal:** three of five measured surfaces report the identical
     `(-0.00, 0.01, 0.00)`, so a normal is either a direction or absent, and the
     guest treats a sub-threshold one as absent.
   - Use the ray's own hit as ground height. `CollisionPosition` sits
     0.24–0.35 m above it depending on the surface and is a reference point, not
     ground.
   - *Acceptance:* walk MHW's slopes without clipping or floating, get stopped by
     MHW geometry, see "no terrain" on loading screens rather than the last
     answer, and survive chunk streaming and area transitions.
5. **Combat exchange — Minecraft's verbs against MHW's monsters.** MHW executes
   its normal death and quest logic; nothing writes monster health directly.
   - Read side: stable session entity ID, transform, hit volumes, health.
   - **Sword on monster:** one Minecraft attack produces one validated host
     combat action. An iron sword must reduce a real monster's real health, and
     the monster must die through MHW's own logic.
   - **Monster on Steve:** one monster attack produces one Minecraft damage
     event, so Steve is hurt, and a raised shield refuses it.
   - **TNT** as the proof that the path carries arbitrary Minecraft damage
     sources, not just melee.
   - **Elytra** as the proof that Minecraft movement can exceed MHW's own.
   - Verify duplicates, stale targets, disconnects, despawns, and death
     semantics: what Steve's death means for an MHW quest, and the reverse.
6. **Building:** place and break blocks, persist them by host area/session, and
   validate interaction with host creatures. Guest collision alone does not make
   monsters collide with blocks; host collision or validated damage queries
   require a separate adapter. Test a monster breaking one structure. Built
   geometry is Minecraft's, rendered into MHW's frame by milestone 3's composite.
7. **One assigned quest:** start, enter, complete, fail, abandon, and return
   through normal MHW flow. Minecraft combat must satisfy the host objective;
   verify rewards and progression exactly once. Scene changes invalidate old
   entity IDs, commands, and coordinate anchors.
8. **Story participation:** support one dialogue/cutscene sequence and its
   following quest. Yield input and camera to MHW during scripted sequences —
   the ownership model's other half — hide the Minecraft layer when necessary,
   then resume with a fresh anchor. Track compatibility per quest before expanding
   to the campaign.
9. **Broader interaction:** monster parts, statuses, capture, gathering, NPCs,
   crafting exchanges, and environmental effects each get a dedicated mapping
   and a real-game acceptance scene. Cross-game item conversions need a
   transaction identity to prevent duplicate rewards.

## What retired from the old direction

The player proxy stays, but as the **MHW-owned spectator mode** it was always
good for: MHW drives, Minecraft shows the hunter. It is no longer the player's
body, and the milestone-3 work that aligned them is not a step toward the goal —
it is the opposite of it. Region-selected block placement is superseded by
milestone 3's full-frame composite, which is why the spike comes first.

## What could still kill this

Named so nobody discovers them halfway through:

- **Full-frame composite at 60 fps.** Everything in milestone 3 rests on it.
  Region-selected blocks are cheap; a full frame is not, and it is the one item
  whose cost is genuinely unknown today.
- **Writing damage into MHW.** Reading monster state is inspection; making an
  iron sword reduce real health needs the host's own combat entry point, resolved
  the same way the terrain ray was, fail-closed. The prior art demonstrates it is
  possible; it does not demonstrate it is safe on this build.
- **Elytra against MHW's own camera and physics.** Two movement systems
  disagreeing about gravity is a bug waiting to be found.
- **Quest state.** Minecraft must satisfy MHW's objective, never write it.

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

Every protocol kind added later requires the bridge to be rebuilt **and
restarted**: a running binary predating a kind discards those packets as
`UnknownKind` while both game ends look healthy. Symptom and fix are recorded in
`MODLOG.md`.

### Save backups (2026-10-05)

The pre-write backups exist and were verified against their sources by
`sha256`. They live outside the repository, so they cannot be committed.

| Data | Source | Backup |
| --- | --- | --- |
| MHW Steam `582010` save | `/home/yllaris/.local/share/Steam/userdata/914702006/582010/` (`remote/SAVEDATA1000`, `remote/PhotoData00.dat`, `remotecache.vdf`) | `/home/yllaris/.local/share/crafterhunter-backups/mhw-582010-20261005T0549/` |
| Minecraft world `New World` | `/home/yllaris/.minecraft/saves/New World/` | `/home/yllaris/.local/share/crafterhunter-backups/minecraft-new-world-20261005T0549/` |

Restore procedure: quit the owning game first, then copy the backup directory
back over the source path with `cp -a <backup>/. <source>/`. The Minecraft
world stores its regions under `dimensions/minecraft/overworld/{region,entities,poi}`.

Both snapshots were taken while the games were running (MHW in Astera, the
Minecraft world loaded), so they are live copies: `level.dat` is written
periodically and `SAVEDATA1000` on MHW save events. Take a fresh pair once the
games are idle if a fully flushed snapshot is needed.

## Sources

- Existing local evidence: `docs/tested-builds.md`, `docs/camera-link-test.md`,
  and the installed-game fingerprint tool.
- [SPL plugin callbacks](https://fexty12573.github.io/SharpPluginLoader/API/SharpPluginLoader.Core.IPlugin.html).
- [SPL monster API](https://fexty12573.github.io/SharpPluginLoader/API/SharpPluginLoader.Core.Entities.Monster.html).
- [Existing macOS crossover](https://github.com/justbustin/minecraft-crossover-bridge):
  demonstrates composition, terrain rays, monster proxies, and damage exchange;
  does not establish Linux/DXVK or whole-campaign compatibility. Any reused
  source must retain its license notices; see `docs/prior-art.md`.
- Universal-modder's local knowledge index currently has no MHW-specific note.
