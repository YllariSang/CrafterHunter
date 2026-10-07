#!/usr/bin/env python3
"""Regression tests for validate-world-depth.py against synthetic captures.

The bug this exposes: the validator's distance reconstruction assumed the classic
OpenGL window-depth convention (0 at near, 1 at far) and compared it against the
recorded projection matrix as if any disagreement were corruption. This game's
projection was measured to be reversed-Z (1 at near, 0 at far), where the same
depth values are all perfectly plausible and the reconstruction is backwards -
near distances come out as far ones without a single value looking wrong. With a
standard-form-only validator, a perfectly good capture blocks, and if the check
had instead been removed, a wrong answer would sail through.

Each case writes a synthetic world-capture.meta + world-depth.f32 pair and runs
the validator as a subprocess:

  * standard matrix: must detect standard and validate a known distance.
  * reversed matrix: must detect reversed-Z and validate a known distance.
    (This failed before the fix: "closed form and recorded matrix DISAGREE".)
  * a matrix with no perspective coefficients: must block.
  * no recorded matrix at all: must block, refusing to rebuild from fov.
  * the matrix Minecraft 26.2 actually records (run 20261006T172918Z):
    zero-to-one reversed, m22 = n/(f-n), m32 = nf/(f-n), window zw = ndc.
    This blocked on live data even after the convention fix, because the
    referee assumed the classic zw = (ndc+1)/2 mapping for every matrix -
    both conventions compared at ~1.0 worst-relative against a matrix they
    are both algebraically right about. (This failed before the fix:
    "BLOCKED: neither the standard nor the reversed-Z".)

Usage: test-validate-world-depth.py
"""

import struct
import subprocess
import sys
import tempfile
from pathlib import Path

VALIDATOR = Path(__file__).resolve().parent / "validate-world-depth.py"

WIDTH = 16
HEIGHT = 16
NEAR = 0.05
FAR = 1024.0
SAMPLE_X = 3
SAMPLE_Y = 5


def standard_matrix(near, far):
    """Classic GL perspective, column-major: m22 at index 10, constant at 14."""
    m = [0.0] * 16
    m[0] = 1.0        # m11 (fov/aspect, unused by the validator)
    m[5] = 1.0        # m22 of the xy block
    m[10] = -(far + near) / (far - near)
    m[11] = -1.0      # w_clip = -z_view
    m[14] = -2.0 * far * near / (far - near)
    m[15] = 0.0
    return m


def reversed_matrix(near, far):
    """Reversed-Z: same matrix with the z block's signs flipped."""
    m = [0.0] * 16
    m[0] = 1.0
    m[5] = 1.0
    m[10] = (far + near) / (far - near)
    m[11] = -1.0
    m[14] = 2.0 * far * near / (far - near)
    m[15] = 0.0
    return m


def measured_matrix(near, far):
    """The matrix Minecraft 26.2 actually recorded (archive 20261006T172918Z).

    Zero-to-one reversed-Z: m22 = n/(f-n), m32 = nf/(f-n), w_clip = -z_view,
    and the framebuffer stores zw = ndc (glClipControl ZERO_TO_ONE), not the
    classic (ndc+1)/2. Every printed digit of the live capture matches these
    expressions for n=0.05, f=1024.
    """
    m = [0.0] * 16
    m[0] = 1.0
    m[5] = 1.0
    m[10] = near / (far - near)
    m[11] = -1.0
    m[14] = near * far / (far - near)
    m[15] = 0.0
    return m


def zw_zero_to_one(distance, near, far):
    """Window depth this matrix family produces at a given view distance."""
    return near * (far - distance) / ((far - near) * distance)


def linearise(zw, near, far, convention):
    if convention == "reversed":
        return near * far / (zw * (far - near) + near)
    return near * far / (far - zw * (far - near))


def write_fixture(directory, matrix, sample_zw):
    directory.mkdir(parents=True, exist_ok=True)
    meta = directory / "world-capture.meta"
    lines = [
        "# synthetic capture for validator regression tests",
        f"width={WIDTH}",
        f"height={HEIGHT}",
        "identity=1",
        "generation=1",
        "depthFormat=D32_FLOAT",
        "perspective=FIRST_PERSON",
        "anchorSource=camera-render-state",
        "anchor=0.0000 0.0000 0.0000",
        "anchorRot=0.000 0.000",
        "fov=70.0",
        f"zNear={NEAR}",
        f"zFar={FAR}",
    ]
    if matrix is None:
        lines.append("projectionMatrixKnown=false")
    else:
        lines.append("projectionMatrixKnown=true")
        lines.append("projectionMatrix=" + " ".join(repr(v) for v in matrix))
    lines += [
        "depthRowOrder=bottom-up",
        "depthConvention=gl-window-depth",
    ]
    meta.write_text("\n".join(lines) + "\n")

    values = []
    for y in range(HEIGHT):
        for x in range(WIDTH):
            if (x, y) == (SAMPLE_X, SAMPLE_Y):
                values.append(sample_zw)
            else:
                values.append((y * WIDTH + x) / (WIDTH * HEIGHT))
    (directory / "world-depth.f32").write_bytes(
        struct.pack(f"<{WIDTH * HEIGHT}f", *values))
    return meta


def run(meta, known_dist, x, y):
    return subprocess.run(
        [sys.executable, str(VALIDATOR),
         "--meta", str(meta),
         "--depth", str(meta.parent / "world-depth.f32"),
         "--known-dist", str(known_dist),
         "--x", str(x), "--y", str(y)],
        capture_output=True, text=True)


def check(name, condition, output):
    if condition:
        print(f"PASS: {name}")
        return True
    print(f"FAIL: {name}")
    print("---- validator output ----")
    print(output.stdout)
    print(output.stderr)
    return False


def main():
    results = []

    with tempfile.TemporaryDirectory(dir="/tmp/opencode") as tmp:
        tmp = Path(tmp)

        # 1. standard window depth
        meta = write_fixture(tmp / "standard", standard_matrix(NEAR, FAR),
                             sample_zw=0.97)
        expected = linearise(0.97, NEAR, FAR, "standard")
        out = run(meta, expected, SAMPLE_X, SAMPLE_Y)
        results.append(check(
            "standard matrix detected and distance validated",
            out.returncode == 0
            and "standard (window 0=near, 1=far)" in out.stdout
            and "OK: reconstructed distance" in out.stdout,
            out))

        # 2. reversed-Z - the case that blocked before the fix
        meta = write_fixture(tmp / "reversed", reversed_matrix(NEAR, FAR),
                             sample_zw=0.01)
        expected = linearise(0.01, NEAR, FAR, "reversed")
        out = run(meta, expected, SAMPLE_X, SAMPLE_Y)
        results.append(check(
            "reversed-Z matrix detected and distance validated",
            out.returncode == 0
            and "reversed-Z (window 1=near, 0=far)" in out.stdout
            and "OK: reconstructed distance" in out.stdout,
            out))

        # 3. matrix without perspective coefficients must block
        broken = [0.0] * 16
        meta = write_fixture(tmp / "broken", broken, sample_zw=0.5)
        out = run(meta, 1.0, SAMPLE_X, SAMPLE_Y)
        results.append(check(
            "useless matrix blocks",
            out.returncode == 1 and "BLOCKED" in out.stdout and
            "neither the standard nor the reversed-Z" in out.stdout,
            out))

        # 4. missing matrix must block rather than rebuild from fov
        meta = write_fixture(tmp / "missing", None, sample_zw=0.5)
        out = run(meta, 1.0, SAMPLE_X, SAMPLE_Y)
        results.append(check(
            "missing matrix blocks instead of rebuilding from fov",
            out.returncode == 1 and
            "the projection was not recorded" in out.stdout,
            out))

        # 5. the matrix this game actually records - the live gate blocked here
        known = 4.64
        meta = write_fixture(tmp / "measured", measured_matrix(NEAR, FAR),
                             sample_zw=zw_zero_to_one(known, NEAR, FAR))
        out = run(meta, known, SAMPLE_X, SAMPLE_Y)
        results.append(check(
            "measured zero-to-one reversed matrix detected and distance validated",
            out.returncode == 0
            and "reversed-Z (window 1=near, 0=far)" in out.stdout
            and "zero-to-one" in out.stdout
            and "OK: reconstructed distance" in out.stdout,
            out))

    if all(results):
        print(f"all {len(results)} checks passed")
        return 0
    return 1


if __name__ == "__main__":
    sys.exit(main())
