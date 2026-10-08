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
    parser.add_argument("--minecraft", type=Path, default=Path.home()/".minecraft")
    parser.add_argument("--mhw", type=Path, default=Path.home()/".local/share/Steam/steamapps/common/Monster Hunter World")
    args = parser.parse_args()
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
        for path in [export, render/"player-compose.enabled", render/"world-upload.enabled",
                     args.minecraft/"crafterhunter/world-stream.enabled"]:
            paired.atomic_write(path, "baked player preview\n")
        print("4 Hz baked player export/draw enabled; existing frame-compositor flags unchanged.")
        print("F7/F9 OFF. Align once in first-person, then F5; do NOT re-anchor on F5.")

if __name__ == "__main__":
    main()
