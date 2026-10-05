#!/usr/bin/env python3
"""Restart the Minecraft guest and ask it for one frame readback.

Closes Minecraft by asking the window manager to close its window, which is the
path the games take for themselves, then relaunches it with the launch command
captured from the previous run. Nothing here touches saves, accounts, or the
launcher: the game is relaunched exactly as it was started, only by us instead
of by a click.

Why capture the command rather than drive the launcher: SKLauncher's window is a
GUI, and clicking it blind is how you end up launching the wrong profile. The
previous command line is the same one the launcher would use.
"""
import argparse
import json
import os
import signal
import subprocess
import sys
import time
from pathlib import Path

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--command-file", type=Path, default=Path("/tmp/opencode/mc-command.txt"))
parser.add_argument("--frames", type=int, default=1)
parser.add_argument("--no-close", action="store_true", help="relaunch without closing first")
parser.add_argument("--capture-only", action="store_true",
                    help="skip the restart and just ask for a frame")
args = parser.parse_args()

GAME = Path.home() / ".minecraft"
LOG = GAME / "logs/latest.log"
MARK = "CrafterHunter] Frame hook is live"


def run(*args, **kwargs):
    return subprocess.check_output(args, text=True, **kwargs)


def minecraft_pid():
    listing = subprocess.run(["ps", "-u", str(os.getuid()), "-o", "pid=,args="],
                             capture_output=True, text=True).stdout
    for line in listing.splitlines():
        if "knot.KnotClient" in line:
            return int(line.split()[0])
    return None


def minecraft_window():
    clients = json.loads(run("hyprctl", "-j", "clients"))
    for client in clients:
        if "minecraft" in (client.get("class", "") + client.get("title", "")).lower():
            return client["address"]
    return None


def close_minecraft():
    """Stop Minecraft by signal, not by closing its window.

    A window close is what a person does, and Minecraft answers it with a "Save
    and Quit to Title?" dialog waiting for a click — the window stays open and
    the process never exits, which looks exactly like a broken close request.
    SIGTERM is the path the JVM itself takes on the way out and runs Minecraft's
    shutdown hooks, which is where it saves.
    """
    pid = minecraft_pid()
    if pid is None:
        return False
    print(f"Sending SIGTERM to Minecraft ({pid})")
    os.kill(pid, signal.SIGTERM)
    for _ in range(30):
        time.sleep(2)
        if minecraft_pid() is None:
            print(f"Minecraft ({pid}) exited")
            return True
    raise SystemExit(f"Minecraft ({pid}) ignored SIGTERM; not relaunching on top of it")


def relaunch():
    if not args.command_file.is_file():
        raise SystemExit(f"No saved launch command at {args.command_file}")
    command = args.command_file.read_text().strip()
    log_path = Path("/tmp/opencode/minecraft-restart.log")
    with log_path.open("ab") as output:
        subprocess.Popen(
            ["setsid", "bash", "-lc", f"exec {command}"],
            stdout=output, stderr=subprocess.STDOUT, stdin=subprocess.DEVNULL,
            start_new_session=True,
        )
    print(f"Relaunched Minecraft; its stdout is at {log_path}")


def wait_for_hook(timeout=240):
    """Wait for the frame hook line, which proves the new mod loaded."""
    deadline = time.time() + timeout
    start_marker = LOG.stat().st_mtime if LOG.exists() else 0
    while time.time() < deadline:
        if LOG.exists() and LOG.stat().st_mtime > start_marker:
            text = LOG.read_text(errors="replace")
            if MARK in text:
                return True
            for bad in ("Mixin apply failed", "Failed to load mod", "CrafterHunter] Frame copy failed"):
                if bad in text:
                    raise SystemExit(f"Minecraft reported: {bad}\nSee {LOG}")
        time.sleep(2)
    return False


def capture(frames):
    request = GAME / "crafterhunter/frame.request"
    request.parent.mkdir(parents=True, exist_ok=True)
    request.write_text(f"capture {frames}\n")
    print(f"Requested {frames} frame readback")
    out = GAME / "crafterhunter/out/capture.txt"
    summary_path = out if out.exists() else None
    for _ in range(60):
        time.sleep(1)
        if summary_path and summary_path.exists():
            time.sleep(1)
            print("--- capture.txt ---")
            print(summary_path.read_text().strip())
            return True
    print(f"No summary after a minute; look in {out.parent}")
    return False


if args.capture_only:
    if not capture(args.frames):
        sys.exit(1)
    sys.exit(0)

if not args.no_close:
    close_minecraft()

relaunch()
if not wait_for_hook():
    raise SystemExit(
        f"Minecraft did not report the frame hook within four minutes; check {LOG}")
print("Frame hook is live")
capture(args.frames)
