#!/usr/bin/env python3
"""Small, bounded Hyprland driver for the already-running MHW test session."""
import argparse
import json
import subprocess
import time


def run(*args):
    return subprocess.check_output(args, text=True)


def require_game():
    active = json.loads(run("hyprctl", "-j", "activewindow"))
    if active.get("class") != "steam_app_582010":
        raise SystemExit("Refusing input: MHW is not the focused window")
    return active


parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("action", choices=["focus", "shot", "key", "mouse"])
parser.add_argument("values", nargs="*")
args = parser.parse_args()
if args.action == "focus":
    clients = json.loads(run("hyprctl", "-j", "clients"))
    game = next((c for c in clients if c["class"] == "steam_app_582010" and
                 c["title"].startswith("MONSTER HUNTER")), None)
    if game is None:
        raise SystemExit("MHW is not running")
    address = game["address"]
    run("hyprctl", "dispatch", f'hl.dsp.focus({{window="address:{address}"}})')
elif args.action == "shot":
    game = require_game()
    x, y = game["at"]
    w, h = game["size"]
    path = args.values[0] if args.values else "/tmp/crafterhunter-live.png"
    run("grim", "-g", f"{x},{y} {w}x{h}", "-s", "0.5", path)
    print(path)
elif args.action == "key":
    require_game()
    key = int(args.values[0])
    seconds = float(args.values[1]) if len(args.values) > 1 else 0.08
    if key < 0 or key > 255 or not 0 <= seconds <= 3:
        raise SystemExit("Invalid key or duration (maximum three seconds)")
    try:
        run("ydotool", "key", f"{key}:1")
        time.sleep(seconds)
    finally:
        run("ydotool", "key", f"{key}:0")
elif args.action == "mouse":
    require_game()
    x, y = map(int, args.values)
    if abs(x) > 500 or abs(y) > 500:
        raise SystemExit("Relative motion is limited to 500 units")
    run("ydotool", "mousemove", "--", str(x), str(y))
