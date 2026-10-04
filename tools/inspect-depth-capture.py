#!/usr/bin/env python3
"""Visualize the native renderer's read-only color/depth dumps (local artifacts)."""
import argparse
import json
from pathlib import Path
import struct
import numpy as np
from PIL import Image

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("files", nargs="+", type=Path)
args = parser.parse_args()
for path in args.files:
    raw = path.read_bytes()
    w, h, fmt, pitch = struct.unpack_from("<4I", raw)
    if w > 16384 or h > 16384 or len(raw) != 16 + pitch * h:
        raise SystemExit(f"Invalid capture: {path}")
    rows = np.frombuffer(raw, dtype=np.uint8, offset=16).reshape(h, pitch)
    if fmt in (39, 40, 41):
        depth = rows[:, :w * 4].copy().view("<f4").reshape(h, w)
        finite = np.isfinite(depth)
        valid = finite & (depth > 0) & (depth <= 1)
        values = depth[valid]
        stats = {"path": str(path), "size": [w, h], "format": fmt,
                 "covered": float(valid.mean()), "finite": bool(finite.all())}
        if values.size:
            stats["percentiles"] = np.percentile(values, [0, 1, 50, 99, 100]).tolist()
        print(json.dumps(stats))
        # Log depth exposes near and distant geometry in a single diagnostic image.
        display = np.zeros((h, w), dtype=np.uint8)
        display[valid] = np.clip((np.log10(values) + 6) * 42.5, 0, 255).astype(np.uint8)
        image = Image.fromarray(display)
    elif fmt == 26:  # DXGI_FORMAT_R11G11B10_FLOAT: unsigned mini-floats.
        packed = rows[:, :w * 4].copy().view("<u4").reshape(h, w)
        channels = []
        for shift, bits in ((0, 6), (11, 6), (22, 5)):
            value = packed >> shift
            mantissa = value & ((1 << bits) - 1)
            exponent = (value >> bits) & 31
            channel = np.where(exponent == 0, mantissa / (1 << bits) * 2.0 ** -14,
                               (1 + mantissa / (1 << bits)) * 2.0 ** (exponent.astype(float) - 15))
            channels.append(channel)
        rgb = np.stack(channels, axis=-1)
        image = Image.fromarray((np.clip(rgb, 0, 1) ** (1 / 2.2) * 255).astype(np.uint8))
    elif fmt in (27, 28, 87):
        pixels = rows[:, :w * 4].copy().reshape(h, w, 4)
        if fmt == 87:
            pixels = pixels[:, :, [2, 1, 0, 3]]
        image = Image.fromarray(pixels[:, :, :3])
    else:
        print(f"Unsupported diagnostic format {fmt}: {path}")
        continue
    image.thumbnail((960, 540))
    output = path.with_suffix(".png")
    image.save(output)
    print(output)
