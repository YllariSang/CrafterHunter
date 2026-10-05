#!/usr/bin/env python3
"""Ask the Minecraft guest to read its rendered frame off the GPU.

Writes a request file the Fabric endpoint polls. Nothing here starts or stops a
game: the endpoint reads one request, deletes it, and captures only what was
asked for. Results land in the game's `crafterhunter/out` directory as a PNG and
a timing summary, and the raw frame in /dev/shm for the native side to pick up.
"""
import argparse
from pathlib import Path

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("action", choices=["capture", "clear"])
parser.add_argument(
    "--frames",
    type=int,
    default=1,
    help="how many frames to read back; 1 unless you are measuring throughput",
)
parser.add_argument(
    "--game",
    type=Path,
    default=Path.home() / ".minecraft",
    help="the Minecraft game directory",
)
args = parser.parse_args()

request = args.game / "crafterhunter" / "frame.request"
if args.action == "clear":
    request.unlink(missing_ok=True)
    print(f"Cleared any pending frame request: {request}")
    raise SystemExit(0)

if not 1 <= args.frames <= 600:
    raise SystemExit("--frames must be between 1 and 600")

request.parent.mkdir(parents=True, exist_ok=True)
request.write_text(f"capture {args.frames}\n")
print(f"Requested {args.frames} frame readback: {request}")
print(f"Watch the HUD FRAME: line, then read {args.game / 'crafterhunter' / 'out'}")
