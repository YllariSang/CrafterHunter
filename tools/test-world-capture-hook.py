#!/usr/bin/env python3
"""Read-only version-pinned check of the capture boundary against Loom bytecode.

No game is launched and no request/deployment files are touched. The former
GameRenderer pre-hand-clear hook fails: LevelRenderer has already erased depth.
"""
import os
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[1]
CACHE = Path(os.environ.get("GRADLE_USER_HOME", str(Path.home() / ".gradle")))
JAR = CACHE / "caches/fabric-loom/minecraftMaven/net/minecraft/minecraft-clientonly-deobf/26.2/minecraft-clientonly-deobf-26.2.jar"
source = (ROOT / "minecraft/fabric/src/client/java/dev/crafterhunter/client/mixin/LevelWorldCaptureMixin.java").read_text()
assert "@Mixin(LevelRenderer.class)" in source, "capture after LevelRenderer returns misses its depth clear"
assert 'method = "lambda$addAlwaysOnTopPass$0"' in source
assert "require = 1" in source, "missing version-specific boundary must fail closed"
assert "shift = At.Shift.AFTER" not in source, "read must precede the clear"
code = subprocess.check_output(["javap", "-classpath", str(JAR), "-c", "-p",
    "net.minecraft.client.renderer.LevelRenderer"], text=True)
match = re.search(r"  private void lambda\$addAlwaysOnTopPass\$0\([^\n]+\);\n(.*?)(?=\n  (?:private|public|protected))", code, re.S)
assert match, "26.2 always-on-top execution method missing"
body = match.group(1)
assert body.count("CommandEncoder.clearDepthTexture:") == 1
assert body.index("CommandEncoder.clearDepthTexture:") < body.index("PreparedFrame.executeAlwaysOnTop:")
assert "RenderTarget.getDepthTexture:" in body
assert code.index("Method addAlwaysOnTopPass:") < code.index("FrameGraphBuilder.execute:")
assert "PreparedFrame.hasAnyAlwaysOnTop:" in code, "fallback assumption changed"
fallback = (ROOT / "minecraft/fabric/src/client/java/dev/crafterhunter/client/mixin/WorldCaptureMixin.java").read_text()
assert 'at = @At("HEAD")' in fallback and "beginWorldFrame()" in fallback
assert "beforeWorldDepthClear(" in fallback and "beforeWorldDepthClear(" in source
import json
config = json.loads((ROOT / "minecraft/fabric/src/client/resources/crafterhunter.client.mixins.json").read_text())
assert "LevelWorldCaptureMixin" in config["client"] and "WorldCaptureMixin" in config["client"]
print("PASS: capture precedes the always-on-top scene-depth clear, not the later hand clear")

# Pin the actual final projection upload, not the earlier camera-owned matrix.
game = subprocess.check_output(["javap", "-classpath", str(JAR), "-c", "-p",
    "net.minecraft.client.renderer.GameRenderer"], text=True)
render = re.search(r"  public void renderLevel\([^\n]+\);\n(.*?)(?=\n  (?:private|public|protected))", game, re.S).group(1)
upload = "ProjectionMatrixBuffer.getBuffer:(Lorg/joml/Matrix4f;)"
assert render.count(upload) == 1
assert render.index("Method bobHurt:") < render.index(upload) < render.index("LevelRenderer.render:")
assert render.index("Method bobView:") < render.index(upload)
assert 'target = "Lnet/minecraft/client/renderer/ProjectionMatrixBuffer;getBuffer(Lorg/joml/Matrix4f;)' in fallback
assert "recordWorldProjection(matrix)" in fallback and "return matrix;" in fallback
assert 'method = "render", at = @At("HEAD"), require = 1' in source
assert "recordWorldView(camera, view)" in source
assert "CameraRenderState;Lorg/joml/Matrix4fc;" in render
main = re.search(r"  private void lambda\$addMainPass\$0\([^\n]+\);\n(.*?)(?=\n  (?:private|public|protected))", code, re.S).group(1)
assert re.search(r"dconst_0[ \t]*\n\s*\d+: invokevirtual[^\n]*CommandEncoder.clearColorAndDepthTextures", main)
capture = (ROOT / "minecraft/fabric/src/client/java/dev/crafterhunter/client/WorldCapture.java").read_text()
assert "worldProjectionKnown = false" in capture and "worldViewKnown = false" in capture
assert "GL45C.GL_CLIP_DEPTH_MODE" in capture and "GL45C.GL_CLIP_ORIGIN" in capture
assert "GL11C.GL_DEPTH_RANGE" in capture
print("PASS: final projection after view effects, exact level view/camera argument, per-frame reset and GL clip provenance")
