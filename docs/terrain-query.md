# Terrain queries: adapter design and verified ABI

Milestone 3's terrain half. `docs/host-queries.md` records what the loader does
and does not expose and the six rules the adapter must obey; this file records
the routine that was chosen, the exact calling contract recovered for it, and
how the work is staged.

The call sequence, argument list, buffer sizes, and result offsets follow the
MIT-licensed `justbustin/minecraft-crossover-bridge`,
`monster-hunter-world/mhw-bridge/src/game.cpp`, function `raycast()`, as
credited in `docs/prior-art.md`. Nothing here is disassembled from the game:
the signatures are the ones that project published for build 421810, and they
were recovered for this repository from the bytecode of the removed
`tools/inspect-mhw-signatures.py`, which had compared them against the
installed executable before `.text` was found to be opaque. All seven recovered
addresses equal the prior art's constants exactly, which is what makes the
name-to-pattern pairing trustworthy.

## The routine

`sCollision::CheckSegment` casts a segment against the stage and reports the
first hit through a `TriangleInfo` buffer, with a `Param` block selecting which
surface attributes are considered:

```
Param::ctor(param, 0x7FFFFFFF, 0x3FFFFFFF, 0, 0, 0xA, 0, 1, 1, 0, 1)
Param::setAttr(param, a, b, c, 1)        // optional filter, off by default
param[0xF9] = 0
TriangleInfo::ctor(tri)
r = sCollision::CheckSegment(collision, seg, 1, tri, param)
if (r != 0) { position at tri+0xC0; normal at tri+0xB0; attr = TriangleInfo::attr(tri, 0); }
TriangleInfo::reset(tri)
Param::dtor(param)
```

| Item | Contract |
| --- | --- |
| Segment | eight `f32`: `start.x y z 0, end.x y z 0`, 16-byte aligned |
| Buffers | `param` and `tri`, `0x140` bytes each, 16-byte aligned, zeroed first |
| `CheckSegment` | `(void* collision, float* segment, byte flag, void* tri, void* param) -> int`; `flag` is `1` |
| Result | `int` hit count; `0` means the segment missed |
| Hit position | 3 `f32` at `tri + 0xC0` |
| Hit normal | 3 `f32` at `tri + 0xB0` |
| Surface attribute | `uint32 TriangleInfo::attr(tri, 0)` |
| Units | MHW units, 100 per metre, the same divisor `Plugin.cs` already applies to positions |
| Threading | game thread only |
| Budget | the prior art spends at most ~2 ms of a frame and checks the clock every 16 rays |

Two structures are caller-owned stack buffers, not game objects the caller
allocates through the game, so the adapter keeps them in one place and zeroes
them per call: `TriangleInfo::reset` runs before the buffer goes out of scope,
and `Param::dtor` after.

## Signatures

Byte patterns for build 421810 (Ver. 15.23.00), with the addresses the prior
art published for that build. The addresses are documentation only: the adapter
never uses them, because `.text` is encrypted on disk and can only be scanned
in the loaded image.

| Signature | Published VA | Pattern |
| --- | --- | --- |
| `sCollision::CheckSegment` | `0x14231AC00` | `48 89 5C 24 08 48 89 6C 24 10 48 89 74 24 18 57 48 81 EC B0 01 00 00 48 8B F2 49 8B D9 33 D2 41 0F B6 F8` |
| `TriangleInfo::ctor` | `0x142319440` | `33 D2 C7 41 08 FF FF FF FF 33 C0` |
| `TriangleInfo::reset` | `0x1423194B0` | `33 C0 C7 41 08 FF FF FF FF 48 89 41 20` |
| `Param::ctor` | `0x14231A6E0` | `45 33 C9 48 8D 05 ?? ?? ?? ?? 48 89 01 44 89 89 BC 00 00 00` |
| `Param::setAttr` | `0x14231ABA0` | `89 51 08 44 89 41 10 44 89 49 0C C6 81 F8 00 00 00 01 C3` |
| `Param::dtor` | `0x140282390` | `48 8D 05 ?? ?? ?? ?? 48 89 01 C3` |
| `TriangleInfo::attr` | `0x140329C20` | `48 89 5C 24 08 48 89 74 24 10 57 48 83 EC 20 8B F2 48 8B D9 80 7C 0E 7E` |

`PatternScanner.FindFirst(pattern, cache: true)` resolves each pattern against
the loaded image and records the result in SharpPluginLoader's
`PluginCache.json`, keyed by the pattern string and versioned with the build
(`421810`), exactly as the two signatures already in that file are.

## The two packets

| Kind | Direction | Payload |
| --- | --- | --- |
| `TerrainRequest` = 12 | guest to host | `u32 id`, then `start.x y z` and `end.x y z` as six little-endian `f32` — 28 bytes |
| `TerrainResult` = 13 | host to guest | `u32 id`, `u8 status`, three reserved zero bytes, `position.x y z` and `normal.x y z` as six `f32`, `u32 attribute` — 36 bytes |

Endpoints and the answer are metres in host world coordinates. The guest cannot
ask in Minecraft coordinates, because the host does not know where the guest
anchored: the guest maps through the same proxy anchor it already uses for the
player, so a question and its answer live in one space the guest can reason
about.

`status` is `0` **no terrain**, `1` **miss**, `2` **hit**. The three are not
cosmetic. *No terrain* is the state a loading screen or an area transition
produces, and it must arrive rather than be inferred from silence. *Miss* is a
real answer — the segment touched nothing — and a guest that cannot tell it from
"no answer" would hold its last hit straight through a wall. A miss therefore
carries a zeroed position, so it cannot also be read as a hit at the origin.

Both payloads have golden bytes pinned in `cargo test`, the way the player
payload's are, because the plugin decodes one in C# and the guest writes the
other in Java. Decoding refuses a wrong length, a non-finite float, an unknown
status, and non-zero reserved bytes — the same fail-closed shape the header
already uses for its own reserved bits.

## Loader API shapes

Verified by reflecting the installed `SharpPluginLoader.Core.dll` (2.0.0) in
the scratch process at `/tmp/splapi`, outside the game:

| Need | API |
| --- | --- |
| Resolve a signature | `static IntPtr Memory.PatternScanner.FindFirst(String pattern, Boolean cache)`; `Pattern.FromString` parses the same byte text |
| Call it | `new NativeFunction<T1, …, TRet>(IntPtr)` exposes `Invoke(T1, …) -> TRet` — for `void` callees the adapter uses a blittable stand-in return and discards it |
| The collision singleton | `SingletonManager.GetSingleton("sMhCollision")` returns `MtObject`; `NativeWrapper.Instance` is the `void*` the routine wants |
| Ground truth | `Player.MainPlayer.CollisionPosition` (`Vector3&` on `Models.Model`) beside `Position` |
| Fingerprint | the process working directory is the game root, so `Path.GetFullPath("MonsterHunterWorld.exe")` is the pinned file `tools/verify-host-build.py` hashes |

## Rules in code

`docs/host-queries.md` states the six rules; the adapter implements them as one
state machine with three states and no fourth:

- **Disabled** — the executable's SHA-256 is not the pinned one, or any of the
  seven signatures fails to resolve. Set once, at load, before anything is
  called. Nothing later can move it back: the session never queries terrain.
- **Unavailable** — enabled, but the collision singleton, the hunter, or the
  stage is missing. Requests answer "no terrain" instead of holding the last
  answer, which is the state a loading screen or an area transition produces.
- **Ready** — singleton present, signatures resolved, at most one ray per
  one-second sample, and a queue with a fixed depth for guest requests whose
  overflow is dropped and counted rather than accumulated.

## Stages

| Stage | What it proves | State |
| --- | --- | --- |
| A. Host adapter | The routine is callable on this build: signatures resolve in the loaded image, a down-ray from the hunter lands where `CollisionPosition` says the ground is, flat and on slopes, all inside the per-tick budget | implemented in plugin 0.3.4; awaits a live run |
| B. Protocol | `TerrainRequest` and `TerrainResult` carry a segment and an answer between the games with golden bytes in Rust | kinds 12 and 13 defined and tested in Rust; neither side sends them yet |
| C. Guest | The guest maps Minecraft coordinates to host coordinates through the proxy anchor, paces requests, and feeds the answer to collision | not started |
| D. Live acceptance | Ground, slopes, movement, loading screens, chunk streaming, and control fallback in the running games | not started |

Stage A deliberately sends nothing: the acceptance question it answers — *does
the ray hit the ground the hunter stands on?* — is decided by comparing the
ray against `Model.CollisionPosition` on the host, which needs no protocol and
no Minecraft restart, only a plugin reload.

## What stage A looks like in the code

`TerrainAdapter` owns the whole host side. `Initialize()` runs once from
`OnLoad`: it hashes `MonsterHunterWorld.exe` from the process working directory,
compares it with `TerrainRay.PinnedExecutableSha256`, resolves all seven
patterns through `PatternScanner.FindFirst(pattern, cache: true)`, and allocates
the three aligned buffers. The first failure latches `Disabled` with its reason
in the diagnostic log, and the tick cannot move the state back.

`Tick(now)` runs from `OnUpdate` before the bridge gates anything, because a
terrain answer is a host fact and should not wait on the guest. It publishes the
state first — so a loading screen reads *unavailable* rather than the previous
ray's answer — and then samples once a second for a request.

**No ray is cast unless someone asks for one.** `Tick` polls for
`nativePC/plugins/CSharp/CrafterHunter/terrain/check.request`, which
`tools/control-mhw-terrain.py check` writes and `clear` removes. The file is
deleted before the ray runs, so a request fires exactly once, and a request that
arrives while the world is not ready is refused in the log rather than silently
discarded. This matters more than it looks: a periodic self-check would make the
first native call into the game's collision routine about a second after a world
loads — during a cutscene, a quest transition, or with nobody watching. On
demand, the call happens when a person is standing on the ground they want
measured, which is also the only way to compare a slope against flat ground.

Each accepted request produces one report, the evidence the acceptance table
asks for, one line per ray:

```
Terrain self-check 1: hit=1 agree=True rayY=42.310m collisionY=42.310m delta=0.000m positionY=43.040m normal=(0.00, 1.00, 0.00) attr=1
```

`CollisionPosition`'s own meaning is not assumed: the line carries the ray, the
collision point, and the model origin separately, so a constant offset between
them shows up as a delta instead of a silent pass or fail. `agree` compares the
ray with the collision point inside 0.5 m, which is the ground standing on a
slope rather than a snapped grid.

### What the first live cast showed

Plugin 0.3.4 in the running game, 2026-10-05, hunter standing inside the Astera
quest house on slanted wooden architecture:

```
15:51:18.231  Terrain adapter ready: all 7 signatures resolved against the pinned executable.
15:51:18.783  Terrain state changed to Ready.
15:51:59.803  Terrain self-check 1: hit=1 agree=True rayY=-3.821m collisionY=-3.471m delta=0.350m
              positionY=-3.821m normal=(-0.33, 0.89, -0.32) attr=1048576
```

Four requests produced four identical lines, and the game kept running: the
routine is callable, one ray per request, nothing cast on its own. Three things
in that line are worth keeping.

- `rayY` equals `positionY` to the millimetre. The ray lands exactly on the
  surface the hunter stands on, and the hunter's model origin is on that
  surface — the hunter's own height is the ground truth the guest will use.
- `collisionY` sits **0.35 m above** the hit. `CollisionPosition` is not the
  feet; on a slanted surface it behaves like a collision reference point above
  the contact. Stage C must not read it as ground height.
- The normal is a unit vector tilted about 27 degrees from vertical, which is
  what a slanted plank looks like. The ray also hits man-made geometry, because
  stage architecture, wooden platforms, and terrain all share one collision
  system. A sample taken indoors is evidence about geometry, not about terrain:
  the outdoor samples are still outstanding.

## Checks that run outside the game

`tools/test-spl-plugin.sh` builds and runs
`native/mhw-spl-plugin/tests`, which now parses all seven signature patterns
through the loader's own `Pattern.FromString` — a typo or a mangled wildcard
fails there instead of quietly disabling terrain in the next session — and
checks the down-segment geometry, the agreement tolerance including its
boundary and its refusal to judge non-finite numbers, and every branch of the
state machine.

One build change was needed for the adapter: SharpPluginLoader's
`NativeAction`/`NativeFunction` expose `delegate* unmanaged` handles, so calling
one is an unsafe call. `AllowUnsafeBlocks` is enabled in the plugin project and
the unsafe surface is confined to `TerrainAdapter.Tick`,
`TerrainAdapter.CastDownRay`, and `Plugin.OnUpdate`, which only reaches the
ray. The adapter calls the game through the loader's own types rather than
hand-rolled delegates: the runtime SharpPluginLoader hosts inside the Proton
prefix is the Windows build of .NET 8 (its `coreclr.dll` maps out of
`pfx/drive_c/Program Files/dotnet`), so function-pointer calls use the same
x64 calling convention as the game's own code.
