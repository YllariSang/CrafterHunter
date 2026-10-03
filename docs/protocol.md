# CrafterHunter wire protocol v1

All traffic is local UDP sent to the bridge at `127.0.0.1:38470`. Multi-byte
integers and floats are little-endian.

## Header (24 bytes)

| Offset | Size | Field | Meaning |
|---:|---:|---|---|
| 0 | 4 | magic | ASCII `CHNT` |
| 4 | 2 | version | Protocol version, currently `1` |
| 6 | 2 | kind | Message kind |
| 8 | 1 | source | Endpoint role |
| 9 | 3 | reserved | Must be zero |
| 12 | 4 | sequence | Wrapping sender-local sequence number |
| 16 | 4 | payload length | Bytes following the header, maximum 1200 |
| 20 | 4 | session | Zero until session negotiation is implemented |

## Endpoint roles

| Value | Role |
|---:|---|
| 0 | Unknown |
| 1 | Bridge |
| 2 | MHW |
| 3 | Minecraft |
| 255 | Synthetic probe |

Synthetic probes set their logical role to MHW or Minecraft; `255` is reserved
for future diagnostic packets.

## Message kinds

| Value | Name | Payload |
|---:|---|---|
| 1 | Hello | UTF-8 endpoint name |
| 2 | HelloAck | UTF-8 bridge description |
| 3 | Heartbeat | Empty |
| 10 | CameraState | 44 bytes, described below |
| 20 | BlockPixels | Vanilla block ID and 16×16 RGBA pixels |
| 21 | BlockPng | Vanilla block ID and PNG bytes from the active resource manager |

### Minecraft block asset (experimental v0.3)

The Minecraft client currently exports `minecraft:stone` after entering a world.
The ID is one unsigned byte of UTF-8 length followed by that many UTF-8 bytes.
`BlockPixels` then has one byte each for width and height (`16`, `16`), followed
by 1024 bytes of row-major RGBA8 pixels. `BlockPng` has the same ID prefix
followed by the PNG from Minecraft's active resource manager. Each packet must
fit the 1200-byte payload limit. The MHW endpoint requires both forms of the
same ID before drawing and times the asset out after six seconds without a
refresh. This is a comparison-only asset transfer, not block placement or
world-state synchronization.

### CameraState

Eleven `f32` values:

```text
position x, y, z
rotation quaternion x, y, z, w
vertical FOV radians
aspect ratio
near plane metres
far plane metres
```

Packets with an invalid magic, version, source, declared length, or payload
shape are discarded. Endpoints expire after five seconds without a valid
packet.
