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
