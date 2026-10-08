#!/usr/bin/env python3
"""Headless controller lifecycle + shader contract checks, NOT GPU acceptance."""
import pathlib
import subprocess
import sys
import tempfile

root = pathlib.Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory() as temporary:
    base = pathlib.Path(temporary)
    mc, mhw = base/"mc", base/"mhw"
    plugin = mhw/"nativePC/plugins/CSharp/CrafterHunter"
    render = plugin/"render"
    render.mkdir(parents=True)
    (plugin/"CrafterHunter.Render.dll").touch()
    channel = mc/"crafterhunter/world.frame"
    channel.parent.mkdir(parents=True)
    channel.write_bytes(b"existing frame must survive")
    def action(verb):
        subprocess.run([sys.executable, str(root/"tools/control-paired-preview.py"), verb,
                        "--minecraft", str(mc), "--mhw", str(mhw)], check=True, capture_output=True)
    action("start")
    assert (render/"paired-compose.enabled").exists()
    assert (render/"world-upload.enabled").exists()
    assert (mc/"crafterhunter/world-stream.enabled").exists()
    action("align")
    assert (render/"align.request").read_text()=="align\n"
    action("stop")
    action("stop") # idempotent; never deletes frame channel
    assert channel.read_bytes()==b"existing frame must survive"
    assert not (render/"paired-compose.enabled").exists()
    assert not (mc/"crafterhunter/world-stream.enabled").exists()

source=(root/"native/mhw-renderer/renderer.cpp").read_text()
shader=source.split('constexpr char PairedShader[] = R"hlsl(',1)[1].split(')hlsl";',1)[0]
assert "guestDepth.Load" in shader and "guestColour.Load" in shader
assert "mul(guestInverseProjection" in shader and "mul(eyeToHost,eye)" in shader
assert "hostDepth.Load" in shader and "input.pos.z<=host+" in shader
assert "depth<=mapping.y" in shader and "SV_ClipDistance0" in shader
assert "worldMetadata.clipOrigin!=1" in source # unsupported origin fails closed
assert "D3D11_COMPARISON_GREATER" in source # guest self-occlusion
assert "row<3 ? 100 : 1" in source # host GPU centimetres, including translation
# Source-grid draw counts cover awkward resolutions without out-of-bounds loads.
for width,height in [(1,1),(949,1028),(1920,1080),(4096,4096),(7,5)]:
    columns=(width+3)//4
    rows=(height+3)//4
    assert (columns-1)*4<width and (rows-1)*4<height
    assert columns*rows*6<=2**32-1
print("PASS: preview start/align/stop, channel preservation, shader safety contracts and grid bounds (not GPU proof)")
