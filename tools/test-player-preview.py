#!/usr/bin/env python3
"""Isolated controller tests and player draw/extraction contracts, not GPU acceptance."""
from pathlib import Path
import subprocess
import sys
import tempfile

root=Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory() as directory:
    base=Path(directory); mc=base/"mc"; mhw=base/"mhw"
    plugin=mhw/"nativePC/plugins/CSharp/CrafterHunter"; render=plugin/"render"
    render.mkdir(parents=True); (plugin/"CrafterHunter.Render.dll").touch()
    def run(action):
        subprocess.run([sys.executable,str(root/"tools/control-player-preview.py"),action,
            "--minecraft",str(mc),"--mhw",str(mhw)],check=True,capture_output=True)
    run("start")
    assert (mc/"crafterhunter/player-export.enabled").is_file()
    assert (render/"player-compose.enabled").is_file()
    assert not (render/"paired-compose.enabled").exists()
    assert (render/"world-upload.enabled").exists()
    run("align"); assert (render/"align.request").read_text()=="align\n"
    run("stop"); run("stop")
    assert not (render/"player-compose.enabled").exists()
    assert not (mc/"crafterhunter/player-export.enabled").exists()
    assert (render/"world-upload.enabled").exists() # Shared upload is never torn down by player stop.
    for mode in ('uv','net','depth-position','depth-source','real'):
        subprocess.run([sys.executable,str(root/'tools/control-player-uv.py'),mode,'--render',str(render)],check=True)
        assert (render/'player-uv-debug.enabled').exists()==(mode=='uv')
        assert (render/'player-skin-net-debug.enabled').exists()==(mode=='net')
        assert (render/'player-depth-position.enabled').exists()==(mode=='depth-position')
        assert (render/'player-depth-source.enabled').exists()==(mode=='depth-source')
        assert (render/'world-upload.enabled').exists()

source=(root/"native/mhw-renderer/renderer.cpp").read_text()
shader=source.split('constexpr char PlayerShader[] = R"hlsl(',1)[1].split(')hlsl";',1)[0]
assert "guestDepth" not in shader and "guestColour" not in shader
assert "StructuredBuffer<Vertex>" in shader and "mul(bone.pose" in shader
assert "skin.Sample" in shader and "sceneDepth.Load" in shader
assert "input.pos.z<=host+" in shader and "uiScale.xy" in shader
assert "player::matches" in source and "poseFreshness.live(now)" in source
assert "DXGI_FORMAT_R8G8B8A8_UNORM" in source and "candidate.width*4" in source
assert "context->PSSetShaderResources(0,2,material)" in source
assert "context->PSGetShaderResources(0,1" in source
assert "mapped.RowPitch" in source and "player::skinChecksum" in source
assert "o.uv=vertex.uv" in shader and "colour.a<0.5" in shader
assert "float4 checked=PlayerPS(input)" in shader and "float4(input.uv,0,checked.a)" in shader
assert 'floor(input.pos.xy)+0.5' in shader
assert 'sceneDepth.GetDimensions(w,h)' in shader
# Production coordinate boundary remains a direct integer texel Load, no offset/UV remap.
production=shader.split('float4 PlayerPS(',1)[1].split('float4 PlayerUVPS(',1)[0]
assert 'sceneDepth.Load(int3(int2(input.pos.xy),0))' in production
assert 'input.pos.z<=host+0.000001' in production
# Independently check raster pixel centres -> normalized debug Load locations at
# corners/edges across target sizes/aspects. This is math/source contract, not GPU proof.
import math
for width,height in ((1920,1080),(949,1028),(1280,720),(1,1)):
    for x,y in ((0,0),(width-1,height-1),(width//2,height//2)):
        sx,sy=x+0.5,y+0.5
        tx,ty=math.floor(sx),math.floor(sy)
        assert (tx,ty)==(x,y)
        assert ((tx+0.5)/width,(ty+0.5)/height)==(sx/width,sy/height)
        assert 0<(tx+0.5)/width<1 and 0<(ty+0.5)/height<1
export=(root/"minecraft/fabric/src/client/java/dev/crafterhunter/client/PlayerModelExport.java").read_text()
for required in ["renderer.extractRenderState", "renderer.submit(state", "model.setupAnim(state)",
        "part.translateAndRotate", "cube.polygons", "v.worldX()", "loadPose(oldPoses.get(i))", "StandardCopyOption.ATOMIC_MOVE"]:
    assert required in export,required
assert "guestDepth" not in export and "readSkin(gl" in export
print("PASS player start/align/stop, compositor flag preservation, baked extraction/pose restore, matched/fresh/depth-tested draw contracts (not GPU proof)")
