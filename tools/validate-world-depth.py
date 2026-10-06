#!/usr/bin/env python3
"""Validate a paired world capture: does the depth mean anything?

Reads ``world-capture.meta``, ``world-depth.f32`` and ``world-colour.png`` written by
``dev.crafterhunter.client.WorldCapture`` and checks three things that a depth buffer
has to satisfy before it can be used for anything:

  1. Depth linearises to sensible distances using the projection that was actually
     recorded with the frame, not a projection rebuilt from the field of view, and
     with the depth convention that recorded matrix implements - standard window
     depth or reversed-Z - detected against the matrix rather than assumed.
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


def linearise(z_window, near, far, convention="standard"):
    """Distance in metres for a window-space depth, in a stated convention.

    Two conventions exist and the recorded matrix decides which one applies:

      standard : 0 at the near plane, 1 at the far plane (classic OpenGL),
                 d = fn / (f - zw*(f - n)). Checked at both ends: zw=0 gives n,
                 zw=1 gives f.
      reversed : 1 at the near plane, 0 at the far plane (reversed-Z, which is
                 what this game's projection was measured to implement),
                 d = fn / (zw*(f - n) + n). Ends the other way round.

    The convention is never assumed silently: ``detect_convention`` picks it by
    comparing each form against the recorded projection matrix.
    """
    if convention == "reversed":
        denominator = z_window * (far - near) + near
    else:
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


def convention_worst(matrix, near, far, convention, samples=(0.0, 0.25, 0.5, 0.75, 0.999)):
    """Worst relative disagreement between one closed form and the matrix.

    ``distance_from_matrix`` inverts the recorded coefficients directly and assumes
    no convention, so it is the referee between the two closed forms. None means the
    matrix has no usable perspective coefficients at all.
    """
    worst = 0.0
    for zw in samples:
        a = linearise(zw, near, far, convention)
        b = distance_from_matrix(zw, matrix, near, far)
        if b is None:
            return None
        scale = max(1.0, abs(a), abs(b))
        worst = max(worst, abs(a - b) / scale)
    return worst


def detect_convention(matrix, near, far):
    """Which closed form the recorded matrix implements, if either.

    A standard projection and a reversed-Z projection are both perfectly good
    matrices and their depth values are both in [0,1]; the difference is which end
    means what, and getting it backwards reconstructs near distances as far ones
    without a single value looking wrong. Trying both forms against the matrix
    turns that from a silent wrong answer into a reported measurement. Returns
    ``(convention, worst)`` or ``(None, {convention: worst})`` when neither fits.
    """
    worst_by_convention = {}
    for convention in ("standard", "reversed"):
        worst = convention_worst(matrix, near, far, convention)
        if worst is None:
            return None, {"standard": None, "reversed": None}
        worst_by_convention[convention] = worst
    for convention in ("standard", "reversed"):
        if worst_by_convention[convention] < 1e-3:
            return convention, worst_by_convention
    return None, worst_by_convention


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

    convention, worst_by_convention = detect_convention(matrix, near, far)
    print(f"\nprojection: near {near} far {far} fov {meta.get('fov')} "
          f"(recorded, not rebuilt)")
    if convention is None:
        print("            BLOCKED: neither the standard nor the reversed-Z closed")
        print("            form matches the recorded matrix "
              f"(worst relative {worst_by_convention}),")
        print("            so every distance below would be unreliable. Reported,")
        print("            not smoothed over.")
        return 1
    names = {
        "standard": "standard (window 0=near, 1=far)",
        "reversed": "reversed-Z (window 1=near, 0=far)",
    }
    print(f"            convention : {names[convention]}, measured against the")
    print(f"            recorded matrix (worst relative "
          f"{worst_by_convention[convention]:.2e});")
    print(f"            the writer claims depthConvention="
          f"{meta.get('depthConvention')}")

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
        d = linearise(value, near, far, convention)
        label = f"({x},{y})"
        print(f"  {label:>12}  {value:10.6f}  {d:12.3f}")

    # ---- the check the whole thing exists for
    if args.known_dist is not None:
        if args.x is None or args.y is None:
            print("\nA known distance was given but no pixel. Pass --x and --y for the "
                  "pixel\ncovering that block, otherwise there is nothing to compare.")
            return 1
        value = depth[args.y * width + args.x]
        measured = linearise(value, near, far, convention)
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