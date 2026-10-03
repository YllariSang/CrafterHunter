# Architecture

## Process model

CrafterHunter keeps the original games as separate processes:

```text
MHW process                    CrafterHunter bridge              Minecraft process
SPL plugin     <--- UDP --->   127.0.0.1:38470   <--- UDP --->  Fabric client mod
```

The bridge never loads game assets. It routes small, versioned state messages
and gives us one place for diagnostics, session ownership, rate limiting, and
future coordinate conversion.

## Linux/Proton layout

Windows is not required. On Linux, Minecraft and the Rust bridge run as native
Linux processes. Steam launches MHW's Windows executable through Proton, and
SharpPluginLoader loads the .NET 8 MHW endpoint inside that process. All three
processes share the host's loopback interface:

```text
Linux Minecraft <---> Linux bridge <---> Windows MHW DLL inside Proton
```

The later native DirectX 11 composition layer will sit above DXVK from the
game's perspective; it will not replace Proton or DXVK. That renderer can be a
separate native component while SharpPluginLoader remains the lifecycle and
game-API boundary.

## Coordinate contract

Wire-level world transforms use a canonical coordinate system:

- right-handed;
- positive Y is up;
- one unit is one metre;
- quaternions are encoded as `(x, y, z, w)`;
- angles are radians;
- matrices, when introduced, will be column-major.

Each game adapter converts at its own boundary. Minecraft uses one block as one
metre unless a session negotiates another scale.

## Milestones

1. **Transport** — endpoint registration, heartbeats, packet forwarding, and
   synthetic probes.
2. **Proton loader proof** — load the managed endpoint through
   SharpPluginLoader and observe its heartbeat from native Linux. **Complete on
   build 421810.**
3. **MHW telemetry** — publish a camera transform through supported loader APIs
   without changing game state. **Implemented; live motion validation pending.**
4. **Minecraft camera slave** — consume the transform in a licensed Fabric dev
   client and verify coordinate conversion.
5. **Visual composition** — capture Minecraft color/depth and composite it into
   MHW's DirectX 11 frame.
6. **Collision proxy** — expose a bounded MHW collision surface to Minecraft.
7. **Interaction** — block placement, projectiles, damage events, and entity
   mapping, still offline-first.

## Stability policy

Game addresses must never appear directly in protocol or gameplay code.
Version-specific signatures and offsets will live behind an adapter and include
an executable fingerprint. Unknown builds must fail closed and leave the game
untouched. Loader APIs are preferred when they provide the required data.
