#!/usr/bin/env python3
"""Case-2 acceptance runner: arm the filter mid-run and watch the hunter stop.

Read-only with respect to the game. It resolves the local instances exactly as
`resolve-exclusive-input.py` does, publishes the same request file the
renderer consumes (atomic same-directory rename), and reads `/proc/<pid>/mem`.
It never attaches, never writes to the process, and never launches anything.

The run has three phases and the key must be held across all of them:

  A  baseline - filter not armed, the hunter must be moving
  B  armed    - filter armed mid-run, the hunter must stop
  C  released - filter disarmed, the hunter must move again

Movement is reported as units/s for every player field that moved in any phase,
so the numbers come from the game's own position rather than from an assumed
offset. The reply file is written when a request is processed, so `released`
advancing across the arm is the direct evidence that the end-predicate rewrite
fired.

The user must be told to hold the key for the whole run before it starts;
nothing here can press a key.
"""
from __future__ import annotations

import argparse
import importlib.util
import math
import os
import struct
import sys
import tempfile
import time
from pathlib import Path

_SPEC = importlib.util.spec_from_file_location(
    "resolver", Path(__file__).with_name("resolve-exclusive-input.py")
)
resolver = importlib.util.module_from_spec(_SPEC)
_SPEC.loader.exec_module(resolver)

# Player fields sampled every tick: the world-frame position (0x390) and the
# logic-frame position (0x160) both live in here, plus their neighbours. Nothing
# depends on which one is authoritative; both are reported.
WINDOWS = ((0x140, 0x60), (0x380, 0x40))


def publish(text: str, plugin: Path) -> None:
    request = plugin / "render" / "exclusive-input.request"
    request.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.NamedTemporaryFile("w", dir=request.parent, delete=False) as out:
        temporary = Path(out.name)
        out.write(text)
    try:
        os.replace(temporary, request)
    finally:
        temporary.unlink(missing_ok=True)


def read_reply(plugin: Path) -> str:
    try:
        return (plugin / "render" / "exclusive-input.reply").read_text().strip()
    except OSError:
        return "(no reply)"


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--pid", type=int, required=True, help="MonsterHunterWorld.exe PID")
    parser.add_argument("--game", type=Path, default=resolver.DEFAULT_GAME)
    parser.add_argument("--seconds", type=float, default=7.0,
                        help="length of each of the three phases")
    parser.add_argument("--rate", type=float, default=20.0, help="samples per second")
    args = parser.parse_args()

    candidates = resolver.find_matching(args.pid)
    if not candidates:
        print("no transition objects found; is the game in a loaded scene?",
              file=sys.stderr)
        return 2
    mem = resolver.Mem(args.pid)
    winners = resolver.select_local(mem, candidates)
    if len(winners) != 1:
        print(f"expected exactly one local player, matched {len(winners)}",
              file=sys.stderr)
        return 3
    player, human, inst = winners[0]
    moves = resolver.all_cmove_slots(args.pid, player, resolver.VMOVE)
    turns = resolver.all_cmove_slots(args.pid, player, resolver.VTURN)
    end = inst["cMoveEnd"]
    if not moves or not turns:
        print("no cMove/cMoveTurn slots matched the local player", file=sys.stderr)
        return 3

    plugin = args.game / "nativePC/plugins/CSharp/CrafterHunter"
    controller = human + 0x10
    state = player + 0x61C8
    offsets = [base + i * 4 for base, size in WINDOWS for i in range(size // 4)]
    print(f"P=0x{player:x} H=0x{human:x} move_slots={len(moves)} "
          f"turn_slots={len(turns)} end=0x{end:x}")

    start = time.monotonic()

    def snapshot() -> dict[int, float]:
        out = {}
        for off in offsets:
            try:
                out[off] = mem.f32(player + off)
            except (OSError, ValueError, OverflowError, struct.error):
                out[off] = float("nan")
        return out

    samples: list[tuple[float, dict[int, float], float, int, int, int]] = []

    def sample() -> None:
        snap = snapshot()
        try:
            magnitude = mem.f32(controller + 0x960)
            mode = struct.unpack("<I", mem.raw(state + 0xA8, 4))[0]
            current = struct.unpack("<2i", mem.raw(state + 0xAC, 8))
        except (OSError, ValueError, OverflowError, struct.error):
            magnitude, mode, current = float("nan"), -1, (-1, -1)
        samples.append((time.monotonic() - start, snap, magnitude, mode, *current))

    bounds: list[float] = []
    print("\nHOLD the key now, continuously, for the whole run. Do not release it.\n")
    sample()
    for phase in range(3):
        while time.monotonic() - start < len(bounds) * args.seconds:
            sample()
            time.sleep(1.0 / args.rate)
        bounds.append(time.monotonic() - start)
        if phase == 0:
            publish(f"on {player} {human} {','.join(str(m) for m in moves)} "
                    f"{','.join(str(t) for t in turns)} {end}\n", plugin)
            print(f"[{bounds[-1]:5.1f}s] published arm request")
        elif phase == 1:
            publish("off\n", plugin)
            print(f"[{bounds[-1]:5.1f}s] published disarm request")
        # Give the renderer a frame or two to claim the request and reply.
        time.sleep(0.6)
        sample()
        print(f"[{time.monotonic() - start:5.1f}s] reply: {read_reply(plugin)}")

    def rate(off: int, lo: float, hi: float) -> float:
        """Sum of |delta| between consecutive samples in the window, per second."""
        pts = [(t, s[off]) for t, s, *_ in samples if lo <= t < hi]
        if len(pts) < 2:
            return float("nan")
        travel = sum(
            abs(b - a)
            for (_, a), (_, b) in zip(pts, pts[1:])
            if math.isfinite(a) and math.isfinite(b)
        )
        span = pts[-1][0] - pts[0][0]
        return travel / span if span > 0.5 else float("nan")

    bounds.append(samples[-1][0])
    print(f"\n{'field':>10} {'A baseline':>12} {'B armed':>12} {'C released':>12}   units/s")
    for off in offsets:
        rates = [rate(off, bounds[i], bounds[i + 1]) for i in range(3)]
        if any(r == r and r > 0.5 for r in rates):
            print(f"  P+0x{off:04x} {rates[0]:12.2f} {rates[1]:12.2f} {rates[2]:12.2f}")

    print("\nstate timeline (mode, current pair, stick magnitude):")
    last = None
    for t, _, magnitude, mode, group, index in samples:
        key = (mode, group, index, round(magnitude, 2))
        if key != last:
            print(f"  [{t:5.1f}s] mode={mode} current=({group},{index}) "
                  f"magnitude={magnitude:.3f}")
            last = key
    print(f"\nfinal reply: {read_reply(plugin)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())