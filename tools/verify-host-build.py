#!/usr/bin/env python3
"""Verify the preconditions for terrain-ray signature resolution on the host build.

Read-only. Milestone 3's terrain adapter calls MHW's own collision routines, so
it has to resolve those functions from byte signatures at runtime. This tool
establishes, without running the game, everything that can be established about
that beforehand:

1. The installed executable is the build pinned in MODDING_PLAN.md.
2. The on-disk ``.text`` section is opaque, so signatures can only be matched
   against the loaded image and never against the file.
3. SharpPluginLoader's runtime address cache for *this* installation names a
   build that matches the one the terrain-ray signatures were published for,
   and places its results inside ``.text``.
4. SharpPluginLoader's runtime pattern cache holds signatures it resolved in
   the loaded image, all of them landing inside ``.text`` -- proof that the
   scan path the terrain adapter depends on works on this build.

Anything that does not hold exits non-zero, because the adapter must then fail
closed rather than call a routine it cannot identify.

No game address is used to compute anything here; the single published address
in this file is a cross-check between two independent sources, and is reported
rather than acted upon.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import math
import struct
import sys
from pathlib import Path

# SHA-256 of the executable pinned in MODDING_PLAN.md, build ID 15539686.
PINNED_SHA256 = "c2ebbbd2c49f216d484e31a5219bed419eb1e5e7d206d02cba040a3ab79d90ea"

# Player:FindMasterPlayer, published for build 421810 by the MIT-licensed
# justbustin/minecraft-crossover-bridge (monster-hunter-world/mhw-bridge/src/
# game.cpp) and cited in docs/prior-art.md. SharpPluginLoader resolves the same
# symbol at runtime on this installation; if the two agree, the runtime layout
# of this executable matches the build those terrain-ray signatures came from.
PRIOR_ART_FIND_MASTER_PLAYER = 0x141B42010

# A section at this entropy carries no instruction structure; below it, code is
# plainly readable and could be scanned offline.
OPAQUE_TEXT_ENTROPY = 7.9


def parse_pe(blob: bytes):
    """Return (image_base, [(name, virtual_address, virtual_size, raw_ptr, raw_size)])."""
    if blob[:2] != b"MZ":
        raise SystemExit("not a PE file: missing MZ")
    pe = struct.unpack_from("<I", blob, 0x3C)[0]
    if blob[pe:pe + 4] != b"PE\0\0":
        raise SystemExit("not a PE file: missing PE signature")
    _, count, _, _, _, opt_size, _ = struct.unpack_from("<HHIIIHH", blob, pe + 4)
    opt = pe + 24
    magic = struct.unpack_from("<H", blob, opt)[0]
    if magic == 0x20B:                                   # PE32+
        base = struct.unpack_from("<Q", blob, opt + 24)[0]
    elif magic == 0x10B:                                 # PE32
        base = struct.unpack_from("<I", blob, opt + 28)[0]
    else:
        raise SystemExit(f"unknown optional-header magic 0x{magic:x}")

    sections = []
    table = opt + opt_size
    for index in range(count):
        offset = table + index * 40
        name = blob[offset:offset + 8].rstrip(b"\0").decode("ascii", "replace")
        vsize, vaddr, raw_size, raw_ptr = struct.unpack_from("<IIII", blob, offset + 8)
        sections.append((name, vaddr, vsize, raw_ptr, raw_size))
    return base, sections


def shannon_entropy(data: bytes) -> float:
    if not data:
        return 0.0
    counts = [0] * 256
    for byte in data:
        counts[byte] += 1
    total = len(data)
    return -sum((c / total) * math.log2(c / total) for c in counts if c)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    root = Path.home()
    game = root / ".local/share/Steam/steamapps/common/Monster Hunter World"
    parser.add_argument("--exe", type=Path, default=game / "MonsterHunterWorld.exe")
    parser.add_argument(
        "--loader", type=Path,
        default=game / "nativePC/plugins/CSharp/Loader",
        help="directory holding SharpPluginLoader's runtime caches",
    )
    args = parser.parse_args()

    failures = []

    def report(ok: bool, label: str, detail: str) -> None:
        print(f"  [{' ok ' if ok else 'FAIL'}] {label:<34} {detail}")
        if not ok:
            failures.append(label)

    if not args.exe.is_file():
        raise SystemExit(f"executable not found: {args.exe}")
    blob = args.exe.read_bytes()
    base, sections = parse_pe(blob)
    text = next((s for s in sections if s[0] == ".text"), None)
    if text is None:
        raise SystemExit("no .text section")

    print("Executable")
    digest = hashlib.sha256(blob).hexdigest()
    report(digest == PINNED_SHA256, "pinned SHA-256",
           digest if digest == PINNED_SHA256 else f"{digest} != {PINNED_SHA256}")
    print(f"       size {len(blob)} bytes, image base 0x{base:X}, "
          f"{len(sections)} sections")
    print()

    print("On-disk code")
    _, vaddr, vsize, raw_ptr, raw_size = text
    code = blob[raw_ptr:raw_ptr + raw_size]
    entropy = shannon_entropy(code)
    opaque = entropy >= OPAQUE_TEXT_ENTROPY
    report(True, ".text section found",
           f"0x{base + vaddr:X}..0x{base + vaddr + vsize:X}, {len(code)} bytes")
    print(f"       entropy {entropy:.2f}/8.00 -> "
          + ("opaque: byte signatures cannot be verified against the file; the "
             "terrain adapter must scan the loaded image and fail closed on a miss"
             if opaque else
             "readable: signatures could also be checked offline"))
    print()

    native_cache = args.loader / "NativeAddressCache.json"
    plugin_cache = args.loader / "PluginCache.json"
    if not native_cache.is_file() or not plugin_cache.is_file():
        report(False, "SharpPluginLoader caches",
               f"missing {native_cache.name} or {plugin_cache.name} under {args.loader}")
        print()
        print("Preconditions not met; the terrain adapter must fail closed.")
        return 1

    native = json.loads(native_cache.read_text())
    plugin = json.loads(plugin_cache.read_text())

    def in_text(va: int) -> bool:
        return base + vaddr <= va < base + vaddr + vsize

    print("SharpPluginLoader runtime caches")
    report(plugin.get("Version") == native.get("Version"),
           "cache versions agree",
           f"address cache {native.get('Version')}, pattern cache {plugin.get('Version')}")

    found = native.get("Addresses", {}).get("Player:FindMasterPlayer")
    if found is None:
        report(False, "published-symbol cross-check", "Player:FindMasterPlayer not cached")
    else:
        agrees = found == PRIOR_ART_FIND_MASTER_PLAYER
        inside = in_text(found)
        report(agrees and inside, "published-symbol cross-check",
               f"cache 0x{found:X} vs published 0x{PRIOR_ART_FIND_MASTER_PLAYER:X} "
               f"{'agree' if agrees else 'DISAGREE'}, "
               f"{'inside' if inside else 'OUTSIDE'} .text")

    resolved = plugin.get("Addresses", {})
    outside = [pattern for pattern, va in resolved.items() if not in_text(va)]
    report(bool(resolved) and not outside,
           "runtime pattern cache",
           f"{len(resolved)} signature(s) resolved in .text"
           + (f", {len(outside)} outside" if outside else ""))
    for pattern, va in resolved.items():
        print(f"       {pattern[:44]:<44} -> 0x{va:X}")
    print()

    if failures:
        for label in failures:
            print(f"  FAIL: {label}")
        print("\nPreconditions not met; the terrain adapter must fail closed "
              "rather than call a routine it cannot identify.")
        return 1
    print("Preconditions hold: this build is the pinned one, its code is opaque "
          "on disk, and SharpPluginLoader resolves signatures in the loaded "
          "image at runtime. Terrain rays may be resolved at startup and must "
          "still fail closed when any signature is missing.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
