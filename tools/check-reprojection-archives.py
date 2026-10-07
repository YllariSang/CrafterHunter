"""Read-only replay of the two documented real bedrock captures; no game writes."""
import hashlib
import math
from pathlib import Path
import struct
import subprocess
import sys

binary = sys.argv[1]
for directory in sys.argv[2:]:
    path = Path(directory)
    meta = dict(line.split("=", 1) for line in (path / "world-capture.meta").read_text().splitlines()
                if "=" in line and not line.startswith("#"))
    assert meta["mode"] == "BOTH" and meta["anchorSource"] == "camera-render-state"
    assert meta["depthRowOrder"] == "bottom-up" and meta["depthRowsPadded"] == "false"
    depth = (path / "world-depth.f32").read_bytes()
    colour = (path / "world-colour.rgba").read_bytes()
    assert hashlib.sha256(depth).hexdigest() == meta["depthSha256"]
    assert hashlib.sha256(colour).hexdigest() == meta["colourSha256"]
    width, height = int(meta["width"]), int(meta["height"])
    assert (width, height) == (949, 1028) and len(depth) == width * height * 4
    assert len(colour) == len(depth)
    z = struct.unpack_from("<f", depth, (514 * width + 474) * 4)[0]
    measured = float(subprocess.check_output([binary, "--sample", str(width), str(height), "474", "514",
                                            repr(z), *meta["projectionMatrix"].split()], text=True))
    position = list(map(float, meta["anchor"].split()))
    pitch, yaw = map(math.radians, map(float, meta["anchorRot"].split()))
    # Known bedrock near-face plane Z=348, independently confirmed by the user.
    expected = (348 - position[2]) / (math.cos(pitch) * math.cos(yaw))
    hit_x = position[0] - math.sin(yaw) * math.cos(pitch) * expected
    hit_y = position[1] - math.sin(pitch) * expected
    assert 917 <= hit_x <= 918 and -60 <= hit_y <= -59, "ray misses the known bedrock face"
    assert expected > 0 and abs(measured - expected) / expected < 0.01
    print(f"PASS: archive identity {meta['identity']}: native eye distance {measured:.6f}, known face {expected:.6f}")
