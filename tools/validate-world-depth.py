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

Usage: validate-world-depth.py [--meta PATH] [--depth PATH] [--colour PATH]
                                [--known-dist METRES] [--x PX] [--y PX]
"""

import argparse
import hashlib
import math
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


def distance_from_matrix(z_window, matrix, near, far, remap="classic"):
    """Same distance, from the recorded projection matrix's own coefficients.

    Two framebuffers can differ in the window mapping - the step from NDC to
    the depth values actually stored - and the mapping is not written inside
    the matrix, so both candidates are tried and whichever agrees with a
    closed form is the one reported:

      ``classic``     : OpenGL's default, ``zw = (ndc + 1)/2``.
      ``zero-to-one`` : ``glClipControl(ZERO_TO_ONE)``, ``zw = ndc`` - which
                        is what this game's matrix and its measured window
                        values together implement: m22 = n/(f-n),
                        m32 = nf/(f-n), and depthMax = n/d at the hand.

    In both branches z_clip = m22*z_view + m32 and w_clip = -z_view, so only
    the inversion differs. Column-major, as GLSL and JOML both store it:
    index 10 is m22, 14 is m32.
    """
    m22 = matrix[10]
    m32 = matrix[14]
    if abs(m22) < 1e-12 and abs(m32) < 1e-12:
        return None
    if remap == "zero-to-one":
        # zw = z_clip/w_clip = (m22*z_view + m32)/(-z_view)
        #  => z_view * (zw + m22) = -m32, and d = -z_view = m32/(zw + m22)
        denominator = z_window + m22
        if abs(denominator) < 1e-12:
            return None
        return m32 / denominator
    # z_clip = m22*z_view + m32, w_clip = -z_view, zw = (z_clip/w_clip + 1)/2
    # Solve for z_view:  2*zw*(-z_view) = m22*z_view + m32 + (-z_view)
    #  => z_view * (-2zw - m22 + 1) = m32   ... with z_view negative in front
    coefficient = -2.0 * z_window - m22 + 1.0
    if abs(coefficient) < 1e-12:
        return None
    z_view = m32 / coefficient
    return -z_view


def convention_worst(matrix, near, far, convention, remap,
                     samples=(0.0, 0.25, 0.5, 0.75, 0.999)):
    """Worst relative disagreement between one closed form and the matrix.

    ``distance_from_matrix`` inverts the recorded coefficients directly and assumes
    no convention, so it is the referee between the two closed forms; the window
    mapping is varied alongside the convention because the matrix fixes which pair
    is self-consistent. None means the matrix has no usable perspective
    coefficients at all.
    """
    worst = 0.0
    for zw in samples:
        a = linearise(zw, near, far, convention)
        b = distance_from_matrix(zw, matrix, near, far, remap)
        if b is None:
            return None
        scale = max(1.0, abs(a), abs(b))
        worst = max(worst, abs(a - b) / scale)
    return worst


def detect_convention(matrix, near, far):
    """Which closed form and window mapping the recorded matrix implements.

    A standard projection and a reversed-Z projection are both perfectly good
    matrices and their depth values are both in [0,1]; the same is true of the two
    window mappings (classic GL ``(ndc+1)/2`` and glClipControl zero-to-one). Each
    matrix family measured here - and both families a driver can produce - is
    self-consistent with exactly one (convention, mapping) pair. Getting it
    backwards reconstructs near distances as far ones without a single value
    looking wrong; trying all four against the matrix turns that from a silent
    wrong answer into a reported measurement. Returns ``((convention, remap),
    worst)``, or ``(None, {label: worst})`` when none fits.
    """
    labels = [(c, r) for c in ("standard", "reversed")
              for r in ("classic", "zero-to-one")]
    worst_by_combo = {}
    for convention, remap in labels:
        worst_by_combo[(convention, remap)] = convention_worst(
            matrix, near, far, convention, remap)
    for key in labels:
        worst = worst_by_combo[key]
        if worst is not None and worst < 1e-3:
            return key, worst
    return None, worst_by_combo


def load_depth(path, width, height):
    raw = path.read_bytes()
    expected = width * height * 4
    if len(raw) < expected:
        raise SystemExit(f"depth file is {len(raw)} bytes, expected at least {expected}")
    count = width * height
    return struct.unpack_from(f"<{count}f", raw, 0)


def load_colour(path, width, height):
    """Raw RGBA bytes in GL order (row 0 is the framebuffer's bottom row).

    The PNG is written flipped to display order for humans; this file keeps
    the order it was copied in, which is the same order the depth read uses,
    so comparing the two raw files compares like with like.
    """
    raw = path.read_bytes()
    expected = width * height * 4
    if len(raw) < expected:
        raise SystemExit(
            f"colour file is {len(raw)} bytes, expected at least {expected}")
    return raw[:expected]


def colour_depth_agreement(depth, colour, width, height, near, far, convention):
    """Score both depth row orders against the colour image, in metres.

    Colour is the referee depth cannot be for itself: in a real scene the sky
    sits far away and the ground under it does not, and exactly one row order
    puts far depth under the sky pixels. Rows are classified from colour
    (bright, blue-dominant = sky; the brightness floor keeps dark water out),
    each depth row is averaged in window depth and linearised with the
    detected convention, and each order is scored as mean(sky row distance)
    minus mean(ground row distance). The correct order gives a large positive
    gap, the mirror flips it negative.

    Returns ``(verdict, detail)``: verdict is ``"aligned"`` (depth rows line
    up with colour rows), ``"mirrored"`` (they are flipped relative to
    colour), or ``None`` when the evidence cannot decide - no usable
    sky/ground split, or no depth structure for a split to land on. The
    detail carries the numbers either way, so an undecidable result reports
    its evidence instead of a verdict.
    """
    sky_fraction = []
    for y in range(height):
        base = y * width * 4
        sky = 0
        for x in range(width):
            i = base + 4 * x
            r, g, b = colour[i], colour[i + 1], colour[i + 2]
            if b >= r + 8 and b >= g + 8 and min(r, g, b) >= 60:
                sky += 1
        sky_fraction.append(sky / width)

    sky_rows = [y for y in range(height) if sky_fraction[y] >= 0.5]
    ground_rows = [y for y in range(height) if sky_fraction[y] <= 0.1]
    if len(sky_rows) < 3 or len(ground_rows) < 3:
        return None, (f"colour offers {len(sky_rows)} sky and {len(ground_rows)} "
                      f"ground rows, and the split needs at least 3 of each")

    row_distance = []
    for y in range(height):
        base = y * width
        mean_zw = sum(depth[base:base + width]) / width
        row_distance.append(linearise(mean_zw, near, far, convention))

    def gap(depth_row_for_colour_row):
        sky_d = (sum(row_distance[depth_row_for_colour_row(y)] for y in sky_rows)
                 / len(sky_rows))
        ground_d = (sum(row_distance[depth_row_for_colour_row(y)]
                        for y in ground_rows) / len(ground_rows))
        return sky_d - ground_d

    aligned = gap(lambda y: y)
    mirrored = gap(lambda y: height - 1 - y)
    detail = (f"sky-ground gap {aligned:+.1f} m with depth rows as stored, "
              f"{mirrored:+.1f} m with them flipped")
    verdict, best = (("aligned", aligned) if aligned > mirrored
                     else ("mirrored", mirrored))
    if best - min(aligned, mirrored) < 1.0 or best <= 0:
        return None, detail + ", which does not separate the two orders"
    return verdict, detail


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
    parser.add_argument("--colour", type=Path, default=OUT / "world-colour.rgba",
                        help="raw RGBA colour from the same capture, the referee "
                             "for the depth row order")
    parser.add_argument("--known-dist", type=float, default=None,
                        help="camera-forward distance to the sampled block surface, NOT Euclidean range")
    parser.add_argument("--acceptance", action="store_true",
                        help="require a matched pair, measured orientation and known block distance")
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
    if width < 1 or height < 1 or width > 8192 or height > 8192:
        parser.error("capture dimensions outside supported bounds")
    if (args.x is None) != (args.y is None):
        parser.error("--x and --y must be supplied together")
    if args.x is not None and not (0 <= args.x < width and 0 <= args.y < height):
        parser.error("sample pixel outside capture dimensions")
    if args.known_dist is not None and (not math.isfinite(args.known_dist) or args.known_dist <= 0):
        parser.error("known distance must be positive and finite")
    if args.acceptance:
        if args.known_dist is None or args.x is None:
            print("BLOCKED: acceptance requires --known-dist, --x and --y")
            return 1
        if meta.get("mode") != "BOTH" or not meta.get("captureBoundary"):
            print("BLOCKED: acceptance requires a paired capture with issue-time boundary")
            return 1
        for key in ("identity", "generation"):
            if not meta.get(key) or as_int(meta, key, 0) <= 0:
                print(f"BLOCKED: missing capture {key}")
                return 1
        for path, key in ((args.depth, "depthSha256"), (args.colour, "colourSha256")):
            if not path.exists() or hashlib.sha256(path.read_bytes()).hexdigest() != meta.get(key):
                print(f"BLOCKED: {key} missing or mismatched; files are not the recorded pair")
                return 1

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
    if len(matrix) != 16 or not all(math.isfinite(value) for value in matrix):
        print("BLOCKED: projection matrix must contain 16 finite coefficients")
        return 1
    if not near or not far or not math.isfinite(near) or not math.isfinite(far) or near <= 0 or far <= near:
        print(f"\nBLOCKED: projection parameters are unusable (zNear={meta.get('zNear')} "
              f"zFar={meta.get('zFar')}),\nthough the matrix was recorded. Distances "
              "cannot be computed from a matrix\nwhose planes are unknown.")
        return 1

    # Success returns ((convention, remap), worst-float); failure returns
    # (None, {label: worst}). The shape carries which one happened.
    detected, detection_detail = detect_convention(matrix, near, far)
    print(f"\nprojection: near {near} far {far} fov {meta.get('fov')} "
          f"(recorded, not rebuilt)")
    if detected is None:
        print("            BLOCKED: neither the standard nor the reversed-Z closed")
        print("            form matches the recorded matrix under either window")
        print(f"            mapping (worst relative {detection_detail}),")
        print("            so every distance below would be unreliable. Reported,")
        print("            not smoothed over.")
        return 1
    convention, remap = detected
    names = {
        "standard": "standard (window 0=near, 1=far)",
        "reversed": "reversed-Z (window 1=near, 0=far)",
    }
    mappings = {
        "classic": "classic GL (zw = (ndc+1)/2)",
        "zero-to-one": "zero-to-one (zw = ndc)",
    }
    print(f"            convention : {names[convention]}, window mapping")
    print(f"            {mappings[remap]}, both measured against")
    print(f"            the recorded matrix (worst relative "
          f"{detection_detail:.2e});")
    print(f"            the writer claims depthConvention="
          f"{meta.get('depthConvention')}")

    # ---- does the depth have the shape depth must have
    depth = load_depth(args.depth, width, height)
    finite = [d for d in depth if d == d and abs(d) != float("inf")]
    if not finite:
        raise SystemExit("depth contains no finite values")
    lo, hi = min(finite), max(finite)
    if args.acceptance and (len(finite) != len(depth) or lo < 0 or hi > 1 or lo == hi):
        print("BLOCKED: acceptance requires finite, in-range, non-flat scene depth")
        return 1
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
    print("            which is why row order CANNOT be decided from depth alone.")
    # The colour file is the referee; its absence leaves the claim a claim,
    # and a claim contradicted by the referee fails the capture outright.
    orientation_ok = True
    orientation_confirmed = False
    claim = meta.get("depthRowOrder")
    colour_claim = meta.get("colourRowOrder")
    expected = None
    if claim and colour_claim:
        expected = "aligned" if claim == colour_claim else "mirrored"
    if not args.colour.exists():
        print(f"            No colour file at {args.colour} to decide against, so")
        print(f"            '{claim}' stays a claim from the writer, not a measurement.")
    else:
        colour = load_colour(args.colour, width, height)
        verdict, detail = colour_depth_agreement(
            depth, colour, width, height, near, far, convention)
        if verdict is None:
            print(f"            Row order UNDECIDABLE against colour: {detail}.")
            print(f"            '{claim}' stays a claim from the writer, unverified.")
        elif expected is None:
            print(f"            MEASURED {verdict} against colour: {detail}; the row-order")
            print("            claims are incomplete, so there is nothing to confirm.")
        elif verdict == expected:
            orientation_confirmed = True
            print(f"            MEASURED {verdict} against colour: {detail};")
            print(f"            claims depth '{claim}' / colour '{colour_claim}' - CONFIRMED.")
        else:
            print(f"            MEASURED {verdict} against colour: {detail};")
            print(f"            claims depth '{claim}' / colour '{colour_claim}' - CONTRADICTED.")
            orientation_ok = False

    if args.acceptance and not orientation_confirmed:
        print("BLOCKED: acceptance requires colour/depth orientation CONFIRMED, not UNDECIDABLE")
        return 1

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
        if not orientation_ok:
            print("\nNOT VALIDATED: colour says the depth rows are mirrored relative to")
            print("the recorded row order, so (x, y) may address a different row than")
            print("the one named. The writer's row order has to be fixed first.")
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

    if not orientation_ok:
        print("\nNOT VALIDATED: the row order measured against colour contradicts the")
        print("recorded one, so distances computed from these rows are not trustworthy.")
        return 1

    print("\nDepth linearises and the projection is real. Distance accuracy is")
    print("untested: pass --known-dist with --x/--y on a block of known distance.")
    print("Still not done: normalising against MHW reversed-Z, and comparing the")
    print("two depth spaces at all.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
