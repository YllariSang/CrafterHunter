#!/usr/bin/env python3
"""Perspective footprint regression; headless reference, not GPU acceptance."""
from pathlib import Path
import re
import numpy as np

root = Path(__file__).resolve().parents[1]
source = (root / "native/mhw-renderer/renderer.cpp").read_text()
shader = source.split('constexpr char PairedFullShader[] = R"hlsl(', 1)[1].split(')hlsl";', 1)[0]
draw = source.rsplit("bool drawPairedFull(", 1)[1].split("bool drawPlayer(", 1)[0]
# Tie the numerical reference to production topology, corner reconstruction and
# rejection. Compilation is checked separately; neither check runs a real GPU.
assert "uint sample=id/6;" in shader
assert "sample>=uint(sizes.x*sizes.y)" in shader
assert "corners[id%6]*0.5" in shader
assert "cell/sizes.xy*2-1,z,1" in shader
assert "mul(eyeToHost,cornerEye)" in shader
assert shader.count("guestDepth.Load") == 1  # no neighbour mesh across edges
assert "depth<=mapping.y" in shader and "!isfinite(depth)" in shader
assert "clip.z<=0 || clip.z>clip.w" in shader
assert "input.pos.z<=host+0.000001" in shader
assert "input.colour.a<0.5" in shader and "SV_ClipDistance0" in shader
assert "D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST" in draw
assert re.search(r"grid::splatVertices\(.*?worldMetadata.height\),1\)", draw, re.S)
assert "context->Draw(vertices,0)" in draw

# Real baseline scale: source 949x509, host 1920x1080 with controlled FOV.
# Use a smaller plane patch to test every target pixel and fractional origins.
scale = 1080 / 509
projection = np.array([[.8,0,0,0], [0,1.4,0,0],
                       [0,0,.00001,.16], [0,0,-1,0]], dtype=float)
inverse = np.linalg.inv(projection)
width, height = 31, 19

def projected(x, y, distance, offset):
    eye = inverse @ np.array([x / width * 2 - 1, y / height * 2 - 1,
                              .16 / distance - .00001, 1])
    eye /= eye[3]
    clip = projection @ eye
    return (clip[:2] / clip[3] + 1) * [width / 2, height / 2] * scale + offset

def bounds(x, y, distance, offset):
    return projected(x, y, distance, offset), projected(x+1, y+1, distance, offset)

def raster(footprints, offset, depths):
    target = np.zeros((48, 74), dtype=bool)
    for y in range(height):
        for x in range(width):
            d = depths[y, x]
            if not np.isfinite(d) or d <= 0:
                continue
            if footprints:
                low, high = bounds(x, y, d, offset)
                # Sample target centres; half-open edges implement adjacent
                # rectangle coverage without overlaps or artificial expansion.
                xx = np.arange(target.shape[1]) + .5
                yy = np.arange(target.shape[0]) + .5
                target |= ((yy[:,None] >= low[1]-1e-10) &
                           (yy[:,None] < high[1]-1e-10) &
                           (xx[None,:] >= low[0]-1e-10) &
                           (xx[None,:] < high[0]-1e-10))
            else:
                pixel = np.floor(projected(x+.5, y+.5, d, offset)).astype(int)
                target[pixel[1], pixel[0]] = True
    return target

for offset in ([0,0], [.27,1.33], [2.8,3.9]):
    depths = np.full((height, width), 7.)
    solid = raster(True, offset, depths)
    points = raster(False, offset, depths)
    xx = np.arange(solid.shape[1]) + .5
    yy = np.arange(solid.shape[0]) + .5
    expected = ((xx[None,:] >= offset[0]) &
                (xx[None,:] < width*scale+offset[0]) &
                (yy[:,None] >= offset[1]) &
                (yy[:,None] < height*scale+offset[1]))
    assert np.array_equal(solid, expected), "projected plane has holes or spills"
    assert points.sum() < .25 * expected.sum(), "old point baseline no longer reproduces gaps"
    # No extrapolation between neighbours: an invalid column remains absent,
    # even between nearby and distant surfaces at radically different depths.
    depths[:, :15] = 2.
    depths[:, 15] = np.nan
    depths[:, 16:] = 80.
    gap = raster(True, offset, depths)
    missing = ((xx[None,:] >= 15*scale+offset[0]) &
               (xx[None,:] < 16*scale+offset[0])) & expected
    assert not gap[missing].any(), "joined or stretched across invalid depth"
    assert np.array_equal(gap, expected & ~missing)

# Reversed-Z rejection still happens per covered host pixel; drawing a footprint
# must not admit the fragments hidden by the foreground host.
guest = np.array([.04, .04, .04, .04])
host = np.array([0., .02, .08, np.nan])
assert np.array_equal(np.isfinite(host) & (guest > host + .000001),
                      [True, True, False, False])
print("PASS: enlarged/translated perspective footprints cover planes; original points leave >75% gaps; invalid depth/host occlusion retained (not GPU proof)")
