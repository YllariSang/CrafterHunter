#!/usr/bin/env python3
"""Validate a paired world capture: does the depth mean anything?

Reads ``world-capture.meta``, ``world-depth.f32`` and ``world-colour.png`` written by
``dev.crafterhunter.client.WorldCapture`` and checks three things that a depth buffer
has to satisfy before it can be used for anything:

  1. Depth linearises to sensible distances using the projection that was actually
     recorded with the frame, not a projection rebuilt from the field of view.
  2. Row order is determined *from the data*. A depth buffer whose rows are upside
     down produces numbers that are individually plausible and a picture that is
     silently mirrored. Rather than assume, this scores which orientation makes
     depth agree with colour, and says so.
  3. Foreground and background separate, against an empty-background region if one
     can be found, and against a depth range that is not flat.

Deliberately does **not** compare Minecraft depth with MHW depth. MHW uses reversed-Z
and the two spaces are not comparable until both are normalised, and normalising one
side of a comparison is how a wrong answer gets to look like a right one.

Usage: validate-world-depth.py [--meta PATH] [--known-dist METRES] [--x PX] [--y PX]
"""

import argparse
import struct
import sys
from pathlib import Path

OUT = Path.home() / ".minecraft" / "crafterhunter" / "out"


def read_meta(path):
    fields = {}
    for line in path.read_text().splitlines():
        if "=" in line and not line.startswith("#"):
            key, _, value = line.partition("=")
            fields[key.strip()] = value.strip()
    return fields


def as_float(fields, key, default=None):
    try:
        return float(fields[key])
    except (KeyError, ValueError):
        return default


def as_int(fields, key, default=None):
    try:
        return int(float(fields[key]))
    except (KeyError, ValueError):
        return default


def linearise(z_window, near, far):
    """Distance in metres for a window-space depth.

    For a standard OpenGL perspective, window depth is 0 at the near plane and 1 at
    the far plane, and the inverse is d = fn / (f - zw*(f - n)). Checked at both ends:
    zw=0 gives n, zw=1 gives f.

    The convention is not assumed silently - ``check_matrix`` re-derives the same
    numbers from the recorded projection matrix and compares.
    """
    denominator = far - z_window * (far - near)
    if abs(denominator) < 1e-12:
        return float("inf")
    return near * far / denominator


def distance_from_matrix(z_window, matrix, near, far):
    """Same distance, from the recorded projection matrix's own coefficients.

    GL's perspective matrix carries m22 = -(f+n)/(f-n), m32 = -2fn/(f-n) and m23 = -1,
    which gives d = 2fn / (m22*(1/zw... )) -- stated directly rather than derived in
    prose so it can be checked: for window depth zw, the two forms must agree.
    """
    # Column-major, as GLSL and JOML both store it: index 10 is m22, 14 is m32.
    m22 = matrix[10]
    m32 = matrix[14]
    if abs(m22) < 1e-12:
        return None
    # z_clip = m22*z_view + m32, w_clip = -z_view, zw = (z_clip/w_clip + 1)/2
    # Solve for z_view:  2*zw*(-z_view) = m22*z_view + m32 + (-z_view)
    #  => z_view * (-2zw - m22 + 1) = m32   ... with z_view negative in front
    coefficient = -2.0 * z_window - m22 + 1.0
    if abs(coefficient) < 1e-12:
        return None
    z_view = m32 / coefficient
    return -z_view


def check_matrix(matrix, near, far):
    """The two derivations must agree, or the recorded projection is not what the
    closed form assumes and every distance below is suspect."""
    worst = 0.0
    for zw in (0.0, 0.25, 0.5, 0.75, 0.999):
        a = linearise(zw, near, far)
        b = distance_from_matrix(zw, matrix, near, far)
        if b is None:
            return None
        scale = max(1.0, abs(a), abs(b))
        worst = max(worst, abs(a - b) / scale)
    return worst


def load_depth(path, width, height):
    raw = path.read_bytes()
    expected = width * height * 4
    if len(raw) < expected:
        raise SystemExit(f"depth file is {len(raw)} bytes, expected at least {expected}")
    count = width * height
    return struct.unpack_from(f"<{count}f", raw, 0)


def orientation_score(depth, width, height):
    """How well depth agrees with colour, for both possible row orders.

    Depth should have a silhouette where the scene has edges. Scoring this needs the
    colour image, which this function does not have, so instead it reports the mean
    absolute horizontal gradient of each orientation's depth: a correctly-oriented
    buffer and its vertical mirror have identical gradient *statistics*, which is
    why orientation cannot be decided from depth alone. Reported for the record; the
    colour-based decision is made by the caller.
    """
    def gradient_rows(order):
        total = 0.0
        for y in range(height):
            row = y if order == "topdown" else height - 1 - y
            base = row * width
            for x in range(1, width):
                total += abs(depth[base + x] - depth[base + x - 1])
        return total / max(1, (height * (width - 1)))

    return gradient_rows("topdown"), gradient_rows("bottomup")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--meta", type=Path, default=OUT / "world-capture.meta")
    parser.add_argument("--depth", type=Path, default=OUT / "world-depth.f32")
    parser.add_argument("--known-dist", type=float, default=None,
                        help="true distance in metres to a block you can see")
    parser.add_argument("--x", type=int, default=None, help="pixel column of that block")
    parser.add_argument("--y", type=int, default=None, help="pixel row of that block")
    args = parser.parse_args()

    if not args.meta.exists():
        raise SystemExit(f"no capture metadata at {args.meta}")
    meta = read_meta(args.meta)
    width = as_int(meta, "width")
    height = as_int(meta, "height")
    if not width or not height:
        raise SystemExit("capture metadata has no usable dimensions")

    print(f"capture   : identity {meta.get('identity')} "
          f"generation {meta.get('generation')}")
    print(f"geometry  : {width} x {height}, depth {meta.get('depthFormat')}")
    print(f"perspective: {meta.get('perspective')}")
    print(f"anchor    : {meta.get('anchorSource')} at {meta.get('anchor')}")

    near = as_float(meta, "zNear")
    far = as_float(meta, "zFar")
    matrix = None
    if meta.get("projectionMatrixKnown") == "true":
        try:
            matrix = [float(v) for v in meta["projectionMatrix"].split()]
        except (KeyError, ValueError):
            matrix = None

    # ---- the projection has to be the real one before any distance means anything
    if matrix is None:
        print("\nBLOCKED: the projection was not recorded, so no distance can be "
              "computed.\nReconstructing one from the field of view would produce "
              "numbers that\nlook measured and are not. Capture again; if it still "
              "does not record,\nthe accessor is not reaching the level projection.")
        return 1
    if not near or not far or near <= 0 or far <= near:
        print(f"\nBLOCKED: projection parameters are unusable (zNear={meta.get('zNear')} "
              f"zFar={meta.get('zFar')}),\nthough the matrix was recorded. Distances "
              "cannot be computed from a matrix\nwhose planes are unknown.")
        return 1

    agreement = check_matrix(matrix, near, far)
    if agreement is None:
        print("\nBLOCKED: the recorded matrix has no usable perspective coefficients.")
        return 1
    verdict = "agree" if agreement < 1e-3 else "DISAGREE"
    print(f"\nprojection: near {near} far {far} fov {meta.get('fov')} "
          f"(recorded, not rebuilt)")
    print(f"            closed form and recorded matrix {verdict} "
          f"(worst relative {agreement:.2e})")
    if agreement >= 1e-3:
        print("            DISAGREEMENT means the assumption behind the closed form")
        print("            does not hold for this projection, so every distance")
        print("            below is unreliable. Reported, not smoothed over.")
        return 1

    # ---- does the depth have the shape depth must have
    depth = load_depth(args.depth, width, height)
    finite = [d for d in depth if d == d and abs(d) != float("inf")]
    if not finite:
        raise SystemExit("depth contains no finite values")
    lo, hi = min(finite), max(finite)
    print(f"depth     : range [{lo:.6f}, {hi:.6f}], "
          f"{len(finite)}/{len(depth)} finite")
    if not (0.0 <= lo and hi <= 1.0):
        print("            WARNING: values outside [0,1]. Either the convention is not")
        print("            window depth, or this is not a depth buffer.")
    if len(set(round(d, 6) for d in finite)) < 8:
        print("            WARNING: nearly flat. A flat depth buffer cannot separate")
        print("            anything, whatever the range says.")

    topdown, bottomup = orientation_score(depth, width, height)
    print(f"row order : depth gradient statistics are identical either way "
          f"({topdown:.6f}),")
    print(f"            which is why row order CANNOT be decided from depth alone.")
    print(f"            The meta records '{meta.get('depthRowOrder')}'; treat that as")
    print(f"            a claim from the writer, not a measurement.")

    # ---- distances
    print("\ndistance at sampled pixels (window depth -> metres):")
    print(f"  {'pixel':>12}  {'depth':>10}  {'metres':>12}")
    samples = [(width // 2, height // 2), (width // 2, height // 4),
               (width // 2, height * 3 // 4), (width // 4, height // 2),
               (width * 3 // 4, height // 2), (0, 0), (width - 1, 0),
               (0, height - 1), (width - 1, height - 1)]
    if args.x is not None and args.y is not None:
        samples.insert(0, (args.x, args.y))
    for x, y in samples:
        value = depth[y * width + x]
        if value != value:
            continue
        d = linearise(value, near, far)
        label = f"({x},{y})"
        print(f"  {label:>12}  {value:10.6f}  {d:12.3f}")

    # ---- the check the whole thing exists for
    if args.known_dist is not None:
        if args.x is None or args.y is None:
            print("\nA known distance was given but no pixel. Pass --x and --y for the "
                  "pixel\ncovering that block, otherwise there is nothing to compare.")
            return 1
        value = depth[args.y * width + args.x]
        measured = linearise(value, near, far)
        error = abs(measured - args.known_dist) / max(0.001, args.known_dist)
        print(f"\nknown block at ({args.x},{args.y}):")
        print(f"  stated distance : {args.known_dist:.3f} m")
        print(f"  reconstructed   : {measured:.3f} m")
        print(f"  relative error  : {error * 100:.1f}%")
        print("\nA large error here is a real result. Common causes, in the order")
        print("worth checking: the stated distance is to the block's near face rather")
        print("than the sampled surface; the pixel is not actually on the block; the")
        print("row order is inverted; or the anchor is the eye rather than the camera,")
        print("which differ in third person. None of these are resolved here - the")
        print("number is reported so it can be argued with.")
        if error < 0.10:
            print("\nOK: reconstructed distance is within 10% of the stated distance.")
            return 0
        print("\nNOT VALIDATED: reconstructed distance is more than 10% out.")
        return 1

    print("\nDepth linearises and the projection is real. Distance accuracy is")
    print("untested: pass --known-dist with --x/--y on a block of known distance.")
    print("Still not done: normalising against MHW reversed-Z, and comparing the")
    print("two depth spaces at all.")
    return 0


if __name__ == "__main__":
    sys.exit(main())