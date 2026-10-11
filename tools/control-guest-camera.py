#!/usr/bin/env python3
"""Opt-in Minecraft camera control in an already loaded offline MHW scene."""
import argparse
import importlib.util
import secrets
import time
from pathlib import Path


def load(name):
    spec = importlib.util.spec_from_file_location(name, Path(__file__).with_name(name + ".py"))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


paired = load("control-paired-preview")
resolver = load("resolve-exclusive-input")


def fresh_resolution(pid):
    candidates = resolver.find_matching(pid)
    mem = resolver.Mem(pid)
    try:
        winners = resolver.select_local(mem, candidates)
        if len(winners) != 1:
            raise SystemExit(f"Refused: expected one local hunter, found {len(winners)}")
        player, human, instances = winners[0]
        moves = resolver.all_cmove_slots(pid, player, resolver.VMOVE)
        turns = resolver.all_cmove_slots(pid, player, resolver.VTURN)
        if not moves or not turns:
            raise SystemExit("Refused: movement/turn instances incomplete")
        return (f"on {player} {human} {','.join(map(str, moves))} "
                f"{','.join(map(str, turns))} {instances['cMoveEnd']}\n")
    finally:
        mem.f.close()


def disarm_before_resolving(render):
    reply = render / "exclusive-input.reply"
    previous = reply.stat().st_mtime_ns if reply.exists() else 0
    paired.atomic_write(render / "exclusive-input.request", "off\n")
    deadline = time.monotonic() + 5
    while time.monotonic() < deadline:
        try:
            if reply.stat().st_mtime_ns != previous and reply.read_text().startswith("armed=0 "):
                return
        except OSError:
            pass
        time.sleep(0.025)
    raise SystemExit("Refused: renderer did not acknowledge filter release; keep MHW visible/loaded")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=["start", "stop", "status"])
    parser.add_argument("--pid", type=int, help="current MHW PID, required for start")
    parser.add_argument("--minecraft", type=Path, default=Path.home() / ".minecraft")
    parser.add_argument("--mhw", type=Path, default=resolver.DEFAULT_GAME)
    parser.add_argument("--channel", type=Path, default=Path("/dev/shm/crafterhunter"))
    args = parser.parse_args()
    render = args.mhw / "nativePC/plugins/CSharp/CrafterHunter/render"
    flag = args.minecraft / "crafterhunter/guest-control.enabled"
    if args.action == "status":
        reply = args.channel / "camera.reply"
        print(reply.read_text().strip() if reply.exists() else "No camera acknowledgement")
        print("Reply reports last state; check advancing sequence/HUD for current liveness.")
        return
    if args.action == "stop":
        flag.unlink(missing_ok=True)
        paired.atomic_write(render / "guest-camera.request", "off\n")
        paired.atomic_write(render / "exclusive-input.request", "off\n")
        print("Camera/filter release requested; capture channels and anchors preserved.")
        return
    if args.pid is None or args.pid <= 0:
        parser.error("start requires --pid with the current loaded offline MHW process")
    if not (render.parent / "CrafterHunter.Render.dll").is_file():
        raise SystemExit("Install the matching guest/native/managed camera builds first")
    if not (render / "world-upload.enabled").exists() or not (
            args.minecraft / "crafterhunter/world-stream.enabled").exists():
        raise SystemExit("Enable the paired preview and calibrate once in first person first")
    paired.atomic_write(render / "guest-camera.request", "off\n")
    flag.unlink(missing_ok=True)
    disarm_before_resolving(render)
    payload = fresh_resolution(args.pid)
    token = secrets.randbits(63) or 1
    # Complete atomic publications; no saved per-session addresses or live-channel deletion.
    paired.atomic_write(render / "exclusive-input.request", payload)
    paired.atomic_write(flag, f"{token}\n")
    paired.atomic_write(render / "guest-camera.request", f"on {token}\n")
    print(f"Minecraft control requested, token {token}; wait for CONTROL: Minecraft ACTIVE.")
    print("Keep both games visible. Walk/look in Minecraft, then F5; do not recalibrate.")
    print("F6 releases control. A stale/released request requires an explicit new start.")


if __name__ == "__main__":
    main()
