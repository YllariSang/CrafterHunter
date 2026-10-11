#!/usr/bin/env python3
"""Opt-in baked player preview. Does not launch games or alter input/F5 modes."""
import argparse
import importlib.util
from pathlib import Path

spec = importlib.util.spec_from_file_location("paired", Path(__file__).with_name("control-paired-preview.py"))
paired = importlib.util.module_from_spec(spec)
spec.loader.exec_module(paired)

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=["start", "align", "stop"])
    parser.add_argument("--pose-ms", type=int, default=0,
                        help="player pose publish floor in ms, 0 samples every render frame (0..1000)")
    parser.add_argument("--minecraft", type=Path, default=Path.home()/".minecraft")
    parser.add_argument("--mhw", type=Path, default=Path.home()/".local/share/Steam/steamapps/common/Monster Hunter World")
    args = parser.parse_args()
    if not 0 <= args.pose_ms <= 1000:
        parser.error("--pose-ms must be between 0 and 1000")
    plugin = args.mhw/"nativePC/plugins/CSharp/CrafterHunter"
    render = plugin/"render"
    export = args.minecraft/"crafterhunter/player-export.enabled"
    if args.action == "stop":
        export.unlink(missing_ok=True)
        (render/"player-compose.enabled").unlink(missing_ok=True)
        print("Player preview stopped; paired flags, assets and frame channels left intact.")
    elif args.action == "align":
        paired.atomic_write(render/"align.request", "align\n")
        print("Player calibration requested. First-person, fresh paired reference and loaded MHW required; request is NOT acceptance.")
    else:
        if not (plugin/"CrafterHunter.Render.dll").is_file():
            raise SystemExit("Install matching Fabric/native artifacts first.")
        paired.atomic_write(export, f"{args.pose_ms}\n")
        paired.atomic_write(render/"player-compose.enabled", "baked player preview\n")
        # Calibration needs paired captures, but starting the player must not
        # overwrite an already configured world producer/consumer cadence.
        for path in [render/"world-upload.enabled", args.minecraft/"crafterhunter/world-stream.enabled"]:
            if not path.exists():
                paired.atomic_write(path, "250\n")
        print(f"Player pose publish floor: {args.pose_ms} ms (0 samples every render frame).")
        print("Existing frame-compositor flags and world capture/upload cadence preserved.")
        print("F7/F9 OFF. Align once in first-person, then F5; do NOT re-anchor on F5.")

if __name__ == "__main__":
    main()
