# Host terrain and player queries

Milestone 3 design and reconnaissance, 2026-10-05. Nothing in the query path is
implemented yet; this file records what the pinned build was found to offer,
what was ruled out, and the rules the adapter must obey.

## Scope

Milestone 3 in `MODDING_PLAN.md`: bounded host terrain queries feed Minecraft
collision, a host player proxy is aligned with the Minecraft player, and ground,
slopes, movement, loading screens, chunk streaming, and immediate control
fallback are verified in the real games.

## What the pinned loader exposes

Verified by reflecting the installed `SharpPluginLoader.Core.dll` (2.0.0) in a
scratch process outside the game. No API below was executed in-process; the
reflection run only read the assembly's metadata.

| API | What it provides |
| --- | --- |
| `IPlugin.OnUpdate(float)` | A game-thread tick. Camera sampling already runs here, so terrain queries have a native, game-thread-safe place to execute. |
| `Entities.Player.MainPlayer` | The hunter, through `Model`: `Position`, `Rotation`, `Forward`, `CollisionPosition`, plus `Health` and `MaxHealth`. |
| `SingletonManager.GetSingleton(string)` | Loader-resolved singletons; `sMhCamera` is already used this way by `Plugin.cs`. The game also names `sMhCollision`, `sMhArea`, `sMhZone`, and `sMhScene`. |
| `Memory.PatternScanner.FindFirst(string, bool)` | A byte signature resolved against the loaded image, optionally written to and read from a per-build cache. |
| `NativeFunction<...>` | A typed delegate over a resolved address. |
| `CollisionComponent`, `CollisionNode`, `MtGeometry` | Collision geometry owned by *models* (the player, monsters), not by stage terrain. |

Two consequences follow.

**The player proxy needs no reverse engineering.** `Player.MainPlayer` already
carries position and rotation through loader APIs, on the same game thread as
the camera sample.

**A terrain raycast has no loader API.** SPL's `SharpPluginLoader.Core.Collision`
namespace is attack and hit-detection data (`AttackParam`, `CollGeomResource`,
`HitZoneResource`), and `MtGeometry` describes the shapes attached to a model.
Stage terrain is queried by the game's own collision routine instead.

## The code section is opaque on disk

`MonsterHunterWorld.exe` ships with a `.text` section of 48,304,640 bytes at
entropy 8.00/8.00. Random data at that size would contain roughly eleven
`48 89 5C 24` prologues by chance; the file contains none, and the counts of
`40 53`, `E8`, and `C3` are exactly the chance expectation. The section carries
no instruction structure: it is encrypted on disk and decrypted when the
process loads it.

Byte signatures therefore **cannot be checked against the file**, only against
the loaded image. `tools/verify-host-build.py` verifies everything that can be
verified without running the game and exits non-zero otherwise:

- the installed executable is the SHA-256 pinned in `MODDING_PLAN.md`;
- `.text` is opaque, which is the reason the scan has to happen at runtime;
- SharpPluginLoader's runtime address cache for this installation resolves
  `Player:FindMasterPlayer` to the same address the MIT-licensed
  `justbustin/minecraft-crossover-bridge` published for build 421810, and places
  it inside `.text` — so the runtime layout of this executable matches the build
  the terrain-ray signatures were written against;
- SharpPluginLoader's own pattern cache already holds two plugin signatures it
  resolved in the loaded image on this build, both inside `.text`, which proves
  the scan path works here.

## Terrain rays: the routine and the rules

Prior art (credited in `docs/prior-art.md`) reached MHW's terrain by segment
test: build a filter parameter block, construct a triangle-info buffer, call the
game's collision routine against a segment, and read back hit position, normal,
and surface attribute. Ray queries run on the game thread only and carry a
per-frame time budget.

CrafterHunter's adapter must satisfy all of the following, which is stricter
than "call the same function":

1. **No address in gameplay code.** Signatures live in one adapter type as byte
   patterns, resolved once through `PatternScanner.FindFirst(pattern, cache:
   true)`. Nothing numeric is written to the protocol or to a packet.
2. **Fingerprint before use.** The installed executable must match the pinned
   SHA-256 before any signature is resolved; a different build resolves nothing
   and enables nothing.
3. **Fail closed on a miss.** Any signature that does not resolve disables
   terrain queries for the session. A missing routine means no terrain data, not
   a guess about which routine was meant.
4. **Game thread only.** Queries execute inside `OnUpdate`, never from the UDP
   receive path.
5. **Bounded.** A fixed number of queries per tick and a queue depth on requests
   arriving from the guest; overflow is dropped and reported, never accumulated.
6. **Unavailable is a state.** While the singleton, the player, or the stage is
   missing — loading screens, area transitions — the adapter publishes "no
   terrain" rather than the last answer it happened to have.

## Player proxy and coordinates

MHW world coordinates are hundreds of metres from Minecraft's origin, so the
proxy maps by **relative displacement**, exactly as the camera link already
does: the first fresh sample anchors both the host position and the player's
current Minecraft position, and later samples move the proxy by their
difference. A world change, a stale sample, or a disabled link clears the
anchor and hands control back immediately.

Telemetry is `PlayerState` (kind 11): position in metres and a rotation
quaternion, mirroring `CameraState`. The plugin samples it on `OnUpdate` and
sends it only while the player exists — a loading screen produces no sample,
which produces no packet, which ages out on the guest. Silence is the failure
signal; the guest never keeps the last known position alive past the freshness
window.

**Status: accepted in the live games on 2026-10-05.** The plugin samples the
hunter at 20 Hz beside the camera and sends kind 11. On the guest,
`PlayerMixin` runs at the tail of `LocalPlayer.tick()`, so vanilla finishes its
own movement for the frame before the proxy places the player, zeroes velocity,
and clears accumulated fall distance. F9 toggles the link.

One deployment fact is worth keeping next to the claim that "the bridge needs
no change": that is true of the bridge's *source*, false of a bridge binary
built before the kind existed. The running bridge had been started hours
earlier and discarded every player packet with `UnknownKind(11)` — 5,138
times — while the plugin logged `First guarded player read succeeded` and
Minecraft logged no decode error, so neither end looked broken. The loss was
visible only in the bridge's own log. `cargo build --offline --workspace` plus
a bridge restart re-registered both endpoints and stopped the discards. Any
future protocol kind repeats this: rebuild and restart the bridge too.

Two details were made deliberate rather than incidental:

- **The 25 m single-frame rule is shared with the placement anchor.** A jump
  larger than that between consecutive samples re-anchors the proxy at the
  player's current position instead of sweeping the player across the world to
  catch up, because such a jump is a scene change, not walking.
- **The camera and player feeds age by one rule, not two.** `SampleFeed` holds
  the freshness and arrival logic both streams use, and the quaternion-to-yaw
  conversion lives in `Rotation`, shared by `CameraState` and `PlayerState`.
  Extracting them was a refactor of working camera code; both headless suites
  pass over the result.

## Verification still required in the real games

| Question | How it will be answered |
| --- | --- |
| Does the ray hit the ground the hunter stands on? | **Answered 2026-10-05** on three surfaces: flat dirt in Astera's base camp (normal within 4 degrees of vertical), a 27-degree wooden plank inside the quest house, and a dirt path in the Wildspire Wetlands. In every case `rayY` matched `positionY` to within a few millimetres, so the ray lands on the surface the hunter stands on. **Two findings carried forward.** `CollisionPosition` sits 0.35 m above the hit on all three (0.350, 0.344, 0.350), so it is a reference point and not ground height — stage C must use the ray's own hit. And the normal is **not guaranteed to be a unit vector**: the Wetlands surface returned `|n| = 0.010`, stably, with a distinct surface attribute, so a guest must treat a sub-threshold normal as "none reported" rather than normalizing noise. **Still open:** one natural hillside, to see an inclined terrain normal rather than a slanted plank's. |
| Does it survive a loading screen? | Enter and leave an area; confirm queries report "no terrain" rather than a stale hit, and that the proxy anchor clears. Stage A's state machine already publishes *unavailable* for a missing hunter or singleton and resets the ray cadence, so the observable half of this is `Terrain state changed to Unavailable` in the log. **Pending:** the live run. |
| Does chunk streaming disturb it? | Query across an area boundary during streaming and reject answers that disagree with the ground under the hunter. |
| Does control fall back immediately? | Stop the bridge mid-motion and confirm the guest stops driving on the freshness timeout. |
| Does the proxy follow and face the hunter? | **Partly answered 2026-10-05:** with the bridge fixed, the person drove both games and reported camera and movement between the two characters precise and accurate, which is the same judgement that closed the camera link in milestone 2. An intermediate reading of "kinda reversed" was traced to the camera link (F7) being *off*: with F7 on, `CameraMixin` overwrites the view from the MHW camera every frame and hides whatever the model quaternion does; with F7 off the vanilla camera follows `LocalPlayer.yRot`, so the first-person view is the model-quaternion path and nothing else. **Still open:** one turn-direction check with F7 off and F9 on — turn the hunter 90 degrees left and confirm the Minecraft character turns left — plus F9 mid-motion returning control to the keyboard on that tick. |

These acceptance steps need the games running and are recorded in `MODLOG.md`
when they run.
