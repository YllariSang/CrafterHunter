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
