#!/usr/bin/env python3
"""Summarise the renderer's frame-sync trace and judge it.

Reads `framesync` lines from renderer.log and answers the one question the
trace exists for: did every composed frame bind a same-frame depth resource,
and did the GPU camera of that draw agree with the CPU-side view-projection
handed in for the same frame?

Exits non-zero when the trace is missing or any criterion fails, so it can be
part of a gate.
"""
import argparse
import re
import sys
from pathlib import Path

LINE = re.compile(
    r"framesync frame=(?P<frame>\d+) depth=(?P<depth>-?\d+) covered=(?P<covered>[\d.]+)% "
    r"age=(?P<age>\d+) cam=(?P<cam>[0-9a-f]+) (?P<camera>changed|same|unread) "
    r"gpu=\((?P<gx>-?[\d.]+),\s*(?P<gy>-?[\d.]+)\) cpu=\((?P<cx>-?[\d.]+),\s*(?P<cy>-?[\d.]+)\) "
    r"delta=\((?P<dx>-?[\d.]+),\s*(?P<dy>-?[\d.]+)\) left=(?P<left>\d+)"
)

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--log", type=Path,
                    default=Path.home() / ".local/share/Steam/steamapps/common"
                    "/Monster Hunter World/nativePC/plugins/CSharp/CrafterHunter/render/renderer.log")
parser.add_argument("--max-delta", type=float, default=2.0,
                    help="largest acceptable CPU/GPU projection disagreement in pixels")
args = parser.parse_args()

if not args.log.is_file():
    raise SystemExit(f"no renderer.log at {args.log}")

rows = []
NUMERIC = {"covered", "gx", "gy", "cx", "cy", "dx", "dy"}
INTEGER = {"frame", "depth", "age", "left"}
for raw in args.log.read_text(errors="replace").splitlines():
    match = LINE.search(raw)
    if match:
        rows.append({key: (float(value) if key in NUMERIC
                           else int(value) if key in INTEGER else value)
                     for key, value in match.groupdict().items()})

if not rows:
    raise SystemExit("no framesync lines found; arm the trace with "
                     "`python tools/control-mhw-renderer.py framesync`")

depths = sorted({row["depth"] for row in rows})
ages = [row["age"] for row in rows]
cameras = [row["camera"] for row in rows]
covered = [row["covered"] for row in rows]
abs_dx = [abs(row["dx"]) for row in rows]
abs_dy = [abs(row["dy"]) for row in rows]
unread = cameras.count("unread")

print(f"  frames traced   : {len(rows)} (frame {rows[0]['frame']}..{rows[-1]['frame']})")
print(f"  depth bound     : {depths}   age {min(ages)}..{max(ages)} frames")
print(f"  coverage        : {min(covered):.2f}..{max(covered):.2f} %")
print(f"  camera          : changed={cameras.count('changed')} same={cameras.count('same')} unread={unread}")
print(f"  CPU/GPU delta   : max |dx|={max(abs_dx):.3f} px  max |dy|={max(abs_dy):.3f} px  "
      f"mean={sum(abs_dx) / len(abs_dx):.3f} px")

failures = []
if any(row["depth"] < 0 for row in rows):
    failures.append("a frame composed without an eligible depth candidate")
if max(ages) > 1:
    failures.append(f"a bound depth was {max(ages)} frames old (must be <= 1)")
if unread:
    failures.append(f"{unread} frame(s) could not read the GPU camera")
if max(abs_dx) > args.max_delta or max(abs_dy) > args.max_delta:
    failures.append(f"CPU and GPU projections disagree by more than {args.max_delta} px")

if failures:
    for failure in failures:
        print(f"  FAIL: {failure}")
    sys.exit(1)
print("Frame-sync checks passed: same-frame depth on every traced frame, live GPU camera, "
      "CPU and GPU projections agree.")
