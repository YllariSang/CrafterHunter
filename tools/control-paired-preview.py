#!/usr/bin/env python3
"""Opt-in real paired-frame preview. No game launch, input injection or installation."""
import argparse
import os
from pathlib import Path
import tempfile

def atomic_write(path, text):
    path.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.NamedTemporaryFile(mode="w", dir=path.parent, delete=False) as out:
        temporary = Path(out.name)
        out.write(text)
    try:
        os.replace(temporary, path)
    finally:
        temporary.unlink(missing_ok=True)

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=["start", "align", "stop"])
    parser.add_argument("--minecraft", type=Path, default=Path.home()/".minecraft")
    parser.add_argument("--mhw", type=Path, default=Path.home()/".local/share/Steam/steamapps/common/Monster Hunter World")
    args = parser.parse_args()
    plugin = args.mhw/"nativePC/plugins/CSharp/CrafterHunter"
    render = plugin/"render"
    stream = args.minecraft/"crafterhunter/world-stream.enabled"
    if args.action == "stop":
        for path in [stream, render/"paired-compose.enabled", render/"world-upload.enabled", render/"align.request"]:
            path.unlink(missing_ok=True)
        print("Preview stopped. Existing frame channels and stone placement left intact.")
        return
    if not (plugin/"CrafterHunter.Render.dll").is_file():
        raise SystemExit("Install the newly built managed/native renderer first; this command does not install it.")
    if args.action == "align":
        atomic_write(render/"align.request", "align\n")
        print("Calibration requested: requires a loaded MHW world, fresh pair and Minecraft FIRST_PERSON.")
        return
    for path in [render/"world-upload.enabled", render/"paired-compose.enabled", stream]:
        atomic_write(path, "paired preview\n")
    print("4 Hz paired preview enabled. Disable Minecraft F7 camera and F9 proxy, select first-person, then align.")
    print("This does NOT suppress MHW input, import MHW collision, or establish playable acceptance.")

if __name__ == "__main__":
    main()
