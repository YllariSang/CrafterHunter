#!/usr/bin/env python3
"""Replay the existing block-feed packets from a user's installed client JAR.

Rendering/transport diagnostic only, NOT a live Fabric integration test.
Do not run alongside Minecraft: this occupies its bridge endpoint.
No game assets are written to the repository.
"""
import argparse
import io
import socket
import struct
import time
import zipfile
from PIL import Image

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("jar")
parser.add_argument("--seconds", type=int, default=600)
args = parser.parse_args()
with zipfile.ZipFile(args.jar) as jar:
    png = jar.read("assets/minecraft/textures/block/stone.png")
texture = Image.open(io.BytesIO(png)).convert("RGBA")
if texture.size != (16, 16):
    raise SystemExit("Expected vanilla 16x16 stone")
block_id = b"minecraft:stone"
prefix = bytes([len(block_id)]) + block_id
messages = [(1, b"diagnostic-asset-replay/not-live-fabric"),
            (20, prefix + bytes([16, 16]) + texture.tobytes()),
            (21, prefix + png)]
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.setblocking(False)
sequence = 0
print("Replaying installed stone asset; this is NOT live Minecraft.", flush=True)
end = time.monotonic() + args.seconds
while time.monotonic() < end:
    for kind, payload in messages:
        sequence += 1
        packet = struct.pack("<4sHHB3xIII", b"CHNT", 1, kind, 3, sequence, len(payload), 0) + payload
        sock.sendto(packet, ("127.0.0.1", 38470))
    try:
        while sock.recv(2048):
            pass
    except BlockingIOError:
        pass
    time.sleep(1)
