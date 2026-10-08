#!/usr/bin/env python3
"""Request a native renderer action; never launches games or changes accounts."""
import argparse
import os
import tempfile
from pathlib import Path

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("action", choices=["place", "clear", "capture", "trace", "framesync", "reload", "align"])
parser.add_argument("--game", type=Path, default=Path.home() / ".local/share/Steam/steamapps/common/Monster Hunter World")
args = parser.parse_args()
plugin = args.game / "nativePC/plugins/CSharp/CrafterHunter"
if not (plugin / "native-renderer.enabled").is_file() or not (plugin / "CrafterHunter.Render.dll").is_file():
    raise SystemExit("Native renderer is not installed/enabled")
if args.action == "place":
    print("Place is for a loaded world: anchors the existing stone 3 metres along the current view.")
elif args.action == "clear":
    print("Clear drops the anchor; the stone stays hidden until the next place.")
elif args.action == "framesync":
    print("Frame sync arms 60 composed frames of camera/depth correlation in renderer.log.")
request = plugin / "native-renderer.reload" if args.action == "reload" else plugin / "render" / f"{args.action}.request"
request.parent.mkdir(exist_ok=True)
with tempfile.NamedTemporaryFile(mode="w", dir=request.parent, delete=False) as out:
    temporary = Path(out.name)
    out.write(args.action + "\n")
try:
    os.replace(temporary, request)
finally:
    temporary.unlink(missing_ok=True)
print(f"Requested {args.action}: {request}")
