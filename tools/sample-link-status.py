#!/usr/bin/env python3
"""Sample the guest's link telemetry and report alignment and latency.

Reads ``~/.minecraft/crafterhunter/out/link.txt``, which the Fabric endpoint writes
from the packets themselves. A composited screenshot cannot say which frame it came
from, so synchronization is measured here instead.

Reports:
  * whether the camera and player links are live, from their own packet accounting;
  * camera age in milliseconds, which is the end-to-end latency of the camera link;
  * the camera pose spread over the sample window, which is what "synchronized camera
    movement" means when it is measured rather than asserted: if the pose does not
    move while packets arrive, nothing downstream is synchronized with anything.

Usage: sample-link-status.py [seconds] [interval-seconds]
"""

import statistics
import sys
import time
from pathlib import Path

LINK = Path.home() / ".minecraft" / "crafterhunter" / "out" / "link.txt"


def read_fields():
    """Parses one link.txt into a flat dict, or None if it is absent or mid-write."""
    try:
        text = LINK.read_text()
    except OSError:
        return None
    fields = {}
    for line in text.splitlines():
        if "=" not in line:
            continue
        key, _, value = line.partition("=")
        fields[key.strip()] = value.strip()
    return fields or None


def as_bool(value):
    return str(value).strip().lower() == "true"


def main():
    seconds = float(sys.argv[1]) if len(sys.argv) > 1 else 10.0
    interval = float(sys.argv[2]) if len(sys.argv) > 2 else 0.25

    samples = []
    deadline = time.monotonic() + seconds
    while time.monotonic() < deadline:
        fields = read_fields()
        if fields:
            samples.append(fields)
        time.sleep(interval)

    if not samples:
        print(f"no telemetry at {LINK} - is Minecraft running with the mod?", file=sys.stderr)
        print("If the endpoint predates link.txt, restart Minecraft once to write it.",
              file=sys.stderr)
        return 1

    camera_live = sum(1 for s in samples if as_bool(s.get("camera.live")))
    player_live = sum(1 for s in samples if as_bool(s.get("player.live")))
    ages = []
    for s in samples:
        try:
            ages.append(float(s.get("camera.ageMillis", "-1")))
        except ValueError:
            pass
    rates = []
    for s in samples:
        try:
            rates.append(float(s.get("camera.packetsPerSecond", "0")))
        except ValueError:
            pass

    poses = []
    for s in samples:
        pose = s.get("camera.pose", "none")
        if pose != "none":
            try:
                poses.append([float(v) for v in pose.split()])
            except ValueError:
                pass

    print(f"samples              : {len(samples)} over {seconds:.1f}s")
    print(f"camera link live     : {camera_live}/{len(samples)} samples"
          f"  ({'LIVE' if camera_live == len(samples) else 'INTERMITTENT'})")
    print(f"player link live     : {player_live}/{len(samples)} samples"
          f"  ({'LIVE' if player_live == len(samples) else 'INTERMITTENT'})")
    if rates:
        print(f"camera packet rate   : median {statistics.median(rates):.0f} pkt/s,"
              f" min {min(rates):.0f}, max {max(rates):.0f}")
    if ages:
        good = [a for a in ages if a >= 0]
        if good:
            print(f"camera age (latency) : median {statistics.median(good):.0f} ms,"
                  f" min {min(good):.0f}, max {max(good):.0f}")
        else:
            print("camera age           : never valid (link has not delivered a sample)")

    if len(poses) >= 2:
        yaw = [p[3] for p in poses]
        pitch = [p[4] for p in poses]
        pos = [p[:3] for p in poses]
        yaw_span = max(yaw) - min(yaw)
        pitch_span = max(pitch) - min(pitch)
        travel = max(
            max(abs(a[i] - b[i]) for i in range(3))
            for a in pos for b in pos
        )
        print(f"camera poses seen    : {len(poses)}")
        print(f"yaw span             : {yaw_span:.2f} deg")
        print(f"pitch span           : {pitch_span:.2f} deg")
        print(f"position travel      : {travel:.3f} m")
        print(f"fov                  : {poses[0][5]:.2f}")
        moved = yaw_span > 0.5 or pitch_span > 0.5 or travel > 0.05
        verdict = "yes" if moved else (
            "NO - the camera was static, so a synchronized-camera demonstration"
            " cannot be made from this window")
        print(f"camera moved         : {verdict}")
    else:
        print("camera poses         : none recorded")

    return 0


if __name__ == "__main__":
    sys.exit(main())
