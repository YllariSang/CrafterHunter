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

source=(root/"native/mhw-renderer/renderer.cpp").read_text()
shader=source.split('constexpr char PlayerShader[] = R"hlsl(',1)[1].split(')hlsl";',1)[0]
assert "guestDepth" not in shader and "guestColour" not in shader
assert "StructuredBuffer<Vertex>" in shader and "mul(bone.pose" in shader
assert "skin.Sample" in shader and "sceneDepth.Load" in shader
assert "input.pos.z<=host+" in shader and "uiScale.xy" in shader
assert "player::matches" in source and "poseFreshness.live(now)" in source
export=(root/"minecraft/fabric/src/client/java/dev/crafterhunter/client/PlayerModelExport.java").read_text()
for required in ["renderer.extractRenderState", "renderer.submit(state", "model.setupAnim(state)",
        "part.translateAndRotate", "cube.polygons", "v.worldX()", "loadPose(oldPoses.get(i))", "StandardCopyOption.ATOMIC_MOVE"]:
    assert required in export,required
assert "guestDepth" not in export and "readSkin(gl" in export
print("PASS player start/align/stop, compositor flag preservation, baked extraction/pose restore, matched/fresh/depth-tested draw contracts (not GPU proof)")
