#!/usr/bin/env python3
"""Resolve the local hunter's transition instances and request suppression.

Read-only with respect to the game: this only reads /proc/<pid>/mem. It never
attaches, never writes to the process, and never launches or restarts anything.

Identification method, verified 2026-10-11 on build 421810:

  1. Find every object carrying the cMove / cMoveTurn / cMoveEnd vtables.
  2. Keep only those whose [obj+0x10] is a player P for which
     H = [P+0x14f8], [H+0xdb0] == P, [C+0x30] == P with C = H+0x10, and a
     sane LStick magnitude at C+0x960. A typical scene yields ~24 matches:
     the hunter plus every other character that owns these transitions.
  3. Of those, select the one whose transition owner T = H+0xa00 registers a
     callback whose [cb+0x18] == that same H. Exactly one candidate matched;
     the rest had no callbacks at all. This is what distinguishes the local
     hunter from other entities, without needing FindMasterPlayer's container.

Addresses change on scene load, so this must be re-run per scene; the renderer
re-validates everything before it acts, and fails open if anything disagrees.

Writes a request file consumed by the renderer's claim-before-read service.
"""
from __future__ import annotations

import argparse
import os
import struct
import sys
import tempfile
from pathlib import Path

VMOVE = 0x1431C53E0
VTURN = 0x1433C96D0
VEND = 0x1433C9688

DEFAULT_GAME = Path.home() / ".local/share/Steam/steamapps/common/Monster Hunter World"


class Mem:
    def __init__(self, pid: int):
        self.f = open(f"/proc/{pid}/mem", "rb", 0)

    def raw(self, addr: int, n: int) -> bytes:
        self.f.seek(addr)
        return self.f.read(n)

    def u64(self, addr: int) -> int:
        return struct.unpack("<Q", self.raw(addr, 8))[0]

    def f32(self, addr: int) -> float:
        return struct.unpack("<f", self.raw(addr, 4))[0]


def writable_regions(pid: int):
    for line in open(f"/proc/{pid}/maps"):
        parts = line.split()
        if "w" not in parts[1]:
            continue
        lo, hi = (int(x, 16) for x in parts[0].split("-"))
        if hi - lo > 400 * 1024 * 1024:
            continue
        yield lo, hi


def find_matching(pid: int) -> dict[int, dict[str, int]]:
    mem = Mem(pid)
    vtables = {VMOVE: "cMove", VTURN: "cMoveTurn", VEND: "cMoveEnd"}
    found: dict[int, dict[str, int]] = {}
    for lo, hi in writable_regions(pid):
        try:
            data = mem.raw(lo, hi - lo)
        except OSError:
            continue
        if not data:
            continue
        for vtable, name in vtables.items():
            needle = struct.pack("<Q", vtable)
            i = 0
            while True:
                i = data.find(needle, i)
                if i < 0:
                    break
                addr = lo + i
                try:
                    player = mem.u64(addr + 0x10)
                    human = mem.u64(player + 0x14F8)
                    command = human + 0x10
                    if mem.u64(human + 0xDB0) != player:
                        raise ValueError
                    if mem.u64(command + 0x30) != player:
                        raise ValueError
                    magnitude = mem.f32(command + 0x960)
                    if magnitude != magnitude or abs(magnitude) > 1.5:
                        raise ValueError
                    found.setdefault(player, {})[name] = addr
                except (OSError, ValueError, struct.error):
                    pass
                i += 1
    return found


def all_cmove_slots(pid: int, player: int) -> list[int]:
    """Every object carrying the cMove vtable whose [obj+0x10] is `player`.

    find_matching overwrites on each hit, so it yields only the last slot found.
    The owner keeps several for one player and uses one at a time, so all of
    them must be covered or suppression is intermittent.
    """
    mem = Mem(pid)
    slots: list[int] = []
    for lo, hi in writable_regions(pid):
        try:
            data = mem.raw(lo, hi - lo)
        except OSError:
            continue
        if not data:
            continue
        needle = struct.pack("<Q", VMOVE)
        i = 0
        while True:
            i = data.find(needle, i)
            if i < 0:
                break
            addr = lo + i
            try:
                if mem.u64(addr + 0x10) == player:
                    slots.append(addr)
            except OSError:
                pass
            i += 1
    return sorted(set(slots))


def select_local(mem: Mem, candidates: dict[int, dict[str, int]]):
    """The single candidate whose transition owner registers a callback tied to H."""
    winners = []
    for player, inst in candidates.items():
        if len(inst) < 3:
            continue
        human = mem.u64(player + 0x14F8)
        owner = human + 0xA00
        try:
            count = mem.u64(owner + 0x268)
        except OSError:
            continue
        if not count or count > 8:
            continue
        for k in range(count):
            try:
                callback = mem.u64(owner + 0x270 + k * 0x40 + 0x38)
                if callback and mem.u64(callback + 0x18) == human:
                    winners.append((player, human, inst))
                    break
            except OSError:
                pass
    return winners


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--pid", type=int, required=True, help="MonsterHunterWorld.exe PID")
    parser.add_argument("--game", type=Path, default=DEFAULT_GAME)
    parser.add_argument("--publish", action="store_true",
                        help="write the on-request file; without it, only report")
    args = parser.parse_args()

    candidates = find_matching(args.pid)
    if not candidates:
        print("no transition objects found; is the game in a loaded scene?", file=sys.stderr)
        return 2

    mem = Mem(args.pid)
    winners = select_local(mem, candidates)
    if len(winners) != 1:
        print(f"expected exactly one local player, matched {len(winners)}", file=sys.stderr)
        return 3

    player, human, inst = winners[0]
    # The owner keeps several cMove slots for one player and uses one at a
    # time. find_matching overwrites on each hit, so this is the last one found,
    # not the only one. Collect every slot explicitly and emit them all.
    moves = all_cmove_slots(args.pid, player)
    if not moves:
        print("no cMove slots matched the local player", file=sys.stderr)
        return 3

    print(f"candidates validated : {len(candidates)}")
    print(f"local player P       : 0x{player:x}")
    print(f"human controller H   : 0x{human:x}")
    print(f"command controller C : 0x{human + 0x10:x}")
    print(f"cMove slots ({len(moves)})     : {', '.join('0x%x' % m for m in moves)}")
    print(f"cMoveTurn            : 0x{inst['cMoveTurn']:x}")
    print(f"cMoveEnd             : 0x{inst['cMoveEnd']:x}")

    if not args.publish:
        print("\n(dry run; pass --publish to arm the filter)")
        return 0

    plugin = args.game / "nativePC/plugins/CSharp/CrafterHunter"
    if not (plugin / "CrafterHunter.Render.dll").is_file():
        print("native renderer is not installed", file=sys.stderr)
        return 4
    request = plugin / "render" / "exclusive-input.request"
    request.parent.mkdir(parents=True, exist_ok=True)
    payload = (f"on {player} {human} {','.join(str(m) for m in moves)} "
               f"{inst['cMoveTurn']} {inst['cMoveEnd']}\n")
    with tempfile.NamedTemporaryFile("w", dir=request.parent, delete=False) as out:
        temporary = Path(out.name)
        out.write(payload)
    try:
        os.replace(temporary, request)
    finally:
        temporary.unlink(missing_ok=True)
    print(f"\npublished {request}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())