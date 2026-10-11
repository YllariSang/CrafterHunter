#!/usr/bin/env python3
"""Execute the shader's local-plane validator off-target, plus an old-flat mutant."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / "native/mhw-renderer/renderer.cpp").read_text()
shader = source.split('constexpr char PairedFullShader[] = R"hlsl(', 1)[1].split(')hlsl";', 1)[0]
body = shader.split("float2 PairedPlaneSlope(", 1)[1].split("Splat PairedFullVS", 1)[0]
body = "float2 PairedPlaneSlope(" + body
# The same production function executes with test-only HLSL vector/texture shims.
# Only the compile-time unroll hint and size swizzle need syntactic adaptation.
body = body.replace("[unroll] ", "").replace("uint2(sizes.xy)", "uint2(width,height)")
assert "cornerDepth=depth+dot(PairedPlaneSlope(xy,depth)" in shader
assert "cornerZ=mapping.x==1 ? cornerDepth : cornerDepth*2-1" in shader
assert "cell/sizes.xy*2-1,cornerZ,1" in shader
assert "input.pos.z<=host+0.000001" in shader  # no occlusion relaxation
assert "guestColour.Load(int3(xy,0))" in shader
assert "depth-extent<=mapping.y || depth+extent>1" in body
with tempfile.TemporaryDirectory(prefix="ch-paired-plane-") as temporary:
    out = Path(temporary)
    include = out / "paired-plane-shader-body.inc"
    test = root / "native/mhw-renderer/tests/paired_plane_test.cpp"
    binary = out / "test"

    def build(text):
        include.write_text(text)
        subprocess.run(["g++", "-std=c++20", "-Wall", "-Wextra", "-Werror", "-O2",
                        "-I", str(out), str(test), "-o", str(binary)], check=True)

    build(body)
    subprocess.run([str(binary)], check=True)
    # Restore the old flat footprint while retaining all validity checks. The
    # numerical plane/occlusion assertions must then fail, not just text checks.
    mutant = re.sub(r"return slope;", "return 0;", body)
    assert mutant != body
    build(mutant)
    result = subprocess.run([str(binary)], capture_output=True, text=True)
    assert result.returncode != 0 and "tilted footprint" in result.stdout
    print("PASS: old flat-depth mutant fails the numerical coverage/occlusion regression (not GPU acceptance)")
