#!/usr/bin/env python3
"""Sample the guest's link telemetry and report liveness and freshness.

Reads ``~/.minecraft/crafterhunter/out/link.txt``, which the Fabric endpoint writes
from the packets themselves. A composited screenshot cannot say which frame it came
from, so synchronization is measured here instead.

Terminology, which matters more than it sounds:

``camera.ageMillis`` is *packet freshness* - how long ago the guest's own writer
published the newest packet, measured on the guest's own clock. It is **not**
end-to-end latency. Latency would be the time from a camera sample being produced
to the host reading it, and that cannot be computed here: the two clocks were
measured 3,422,487 ms apart and fitting an offset from a single sample yields an
age of identically zero, which is an artifact of the method rather than a
measurement. The frame path handles this the same way - it judges freshness on the
reader's own clock, from publication liveness, and never by comparing the two.

Reports:
  * whether the camera and player links are live, from their own packet accounting;
  * packet freshness in milliseconds, per the definition above;
  * the camera pose spread over the sample window, which is what "synchronized
    camera movement" means when measured rather than asserted: if the pose does not
    move while packets arrive, nothing downstream is synchronized with anything.

Stale snapshots are refused rather than reported. A telemetry file that has stopped
being rewritten, or whose guest-side clock has stopped advancing, describes a guest
that is gone or wedged, and reporting its numbers as if they were current is worse
than reporting nothing.

Usage: sample-link-status.py [seconds] [interval-seconds]
"""

import statistics
import sys
import time
from pathlib import Path

LINK = Path.home() / ".minecraft" / "crafterhunter" / "out" / "link.txt"

# link.txt is rewritten every 250 ms by the endpoint. Two seconds of slack is
# generous for a loaded machine and still far below "left over from last session".
STALE_AFTER_SECONDS = 2.0


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


def wall_millis(fields):
    try:
        return int(float(fields.get("wallMillis", "-1")))
    except ValueError:
        return -1


def main():
    seconds = float(sys.argv[1]) if len(sys.argv) > 1 else 10.0
    interval = float(sys.argv[2]) if len(sys.argv) > 2 else 0.25

    # Staleness is checked before anything is measured, because a stale file's
    # contents are the whole reason this check exists.
    if not LINK.exists():
        print(f"no telemetry at {LINK}", file=sys.stderr)
        print("That is the game's own directory. If Minecraft is running, the",
              file=sys.stderr)
        print("endpoint has not written it yet - restart Minecraft once.", file=sys.stderr)
        return 1

    samples = []
    deadline = time.monotonic() + seconds
    while time.monotonic() < deadline:
        fields = read_fields()
        if fields:
            samples.append((time.monotonic(), fields))
        time.sleep(interval)

    if not samples:
        print("telemetry present but never parsed - it is being rewritten "
              "mid-read every time, or is empty", file=sys.stderr)
        return 1

    # A snapshot left over from a previous session parses perfectly. Its host-side
    # modification time is the honest test for that.
    newest_age = time.time() - LINK.stat().st_mtime
    if newest_age > STALE_AFTER_SECONDS:
        print(f"refusing a stale snapshot: link.txt was last written "
              f"{newest_age:.1f}s ago (limit {STALE_AFTER_SECONDS:.0f}s)",
              file=sys.stderr)
        print("The file exists but the endpoint is not maintaining it, so its "
              "contents describe a guest that is gone.", file=sys.stderr)
        return 1

    # And the guest-side clock advancing is the same fact seen from the writer,
    # which survives a filesystem with a clock that does.
    stamps = [wall_millis(f) for _, f in samples]
    stamps = [s for s in stamps if s >= 0]
    if len(stamps) >= 2 and stamps[-1] == stamps[0]:
        print(f"refusing a frozen snapshot: the guest's own wallMillis did not "
              f"advance across {len(stamps)} samples ({stamps[0]})", file=sys.stderr)
        return 1

    fields = [f for _, f in samples]
    camera_live = sum(1 for s in fields if as_bool(s.get("camera.live")))
    player_live = sum(1 for s in fields if as_bool(s.get("player.live")))

    ages = []
    for s in fields:
        try:
            ages.append(float(s.get("camera.ageMillis", "-1")))
        except ValueError:
            pass
    rates = []
    for s in fields:
        try:
            rates.append(float(s.get("camera.packetsPerSecond", "0")))
        except ValueError:
            pass

    poses = []
    for s in fields:
        pose = s.get("camera.pose", "none")
        if pose != "none":
            try:
                poses.append([float(v) for v in pose.split()])
            except ValueError:
                pass

    print(f"samples              : {len(fields)} over {seconds:.1f}s")
    print(f"snapshot age         : {newest_age:.2f}s (fresh, limit {STALE_AFTER_SECONDS:.0f}s)")
    print(f"camera link live     : {camera_live}/{len(fields)} samples"
          f"  ({'LIVE' if camera_live == len(fields) else 'INTERMITTENT'})")
    print(f"player link live     : {player_live}/{len(fields)} samples"
          f"  ({'LIVE' if player_live == len(fields) else 'INTERMITTENT'})")
    if rates:
        print(f"camera packet rate   : median {statistics.median(rates):.0f} pkt/s,"
              f" min {min(rates):.0f}, max {max(rates):.0f}")
    if ages:
        good = [a for a in ages if a >= 0]
        if good:
            print(f"packet freshness     : median {statistics.median(good):.0f} ms,"
                  f" min {min(good):.0f}, max {max(good):.0f}")
            print("                       (guest-clock age since its newest packet")
            print("                        was published - not end-to-end latency)")
        else:
            print("packet freshness     : never valid (link has not delivered a sample)")

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