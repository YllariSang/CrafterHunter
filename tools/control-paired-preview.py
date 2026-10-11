#!/usr/bin/env python3
"""Opt-in real paired-frame composition. No game launch, input injection or installation.

Modes:
  preview  the stride-4 reprojection splat that has been built longest
  full     the spike path: one point primitive per captured texel

Both need the same gates - world upload, a fresh pair, calibration, first-person
Minecraft - and both leave MHW input alone. "full" is still request-paced, so it
is full resolution and not yet full frame rate.
"""
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
    parser.add_argument("action", choices=["start", "start-full", "align", "stop"])
    parser.add_argument("--mode", choices=["preview", "full"], default="preview",
                        help="composition path for start; start-full means full")
    parser.add_argument("--minecraft", type=Path, default=Path.home()/".minecraft")
    parser.add_argument("--mhw", type=Path, default=Path.home()/".local/share/Steam/steamapps/common/Monster Hunter World")
    args = parser.parse_args()
    plugin = args.mhw/"nativePC/plugins/CSharp/CrafterHunter"
    render = plugin/"render"
    stream = args.minecraft/"crafterhunter/world-stream.enabled"
    if args.action == "stop":
        for path in [stream, render/"paired-compose.enabled", render/"paired-full.enabled",
                     render/"world-upload.enabled", render/"align.request"]:
            path.unlink(missing_ok=True)
        print("Composition stopped. Existing frame channels and stone placement left intact.")
        return
    full = args.action == "start-full" or args.mode == "full"
    if args.action == "start-full":
        args.mode = "full"
    if not (plugin/"CrafterHunter.Render.dll").is_file():
        raise SystemExit("Install the newly built managed/native renderer first; this command does not install it.")
    if args.action == "align":
        atomic_write(render/"align.request", "align\n")
        print("Calibration requested: requires a loaded MHW world, fresh pair and Minecraft FIRST_PERSON.")
        return
    flags = [render/"world-upload.enabled", stream]
    # Exactly one composition path at a time: the full path takes precedence when
    # both flags exist, and leaving the preview flag on under it would make the
    # fallback ambiguous.
    flags.append(render/("paired-full.enabled" if full else "paired-compose.enabled"))
    for path in flags:
        atomic_write(path, "paired composition\n")
    if full:
        print("FULL-RESOLUTION paired composition enabled (one point per captured texel).")
        print("Still request-paced: the guest publishes on its own interval, so this is not 60 fps.")
    else:
        print("Stride-4 paired preview enabled.")
    print("Disable Minecraft F7 camera and F9 proxy, select first-person, then align.")
    print("This does NOT suppress MHW input, import MHW collision, or establish playable acceptance.")

if __name__ == "__main__":
    main()
