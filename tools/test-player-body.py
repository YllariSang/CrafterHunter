#!/usr/bin/env python3
"""Headless single-body regressions; no live control files or game processes."""
from pathlib import Path
import json
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
CLIENT = ROOT / 'minecraft/fabric/src/client/java/dev/crafterhunter/client'
JAR = Path.home() / '.gradle/caches/fabric-loom/minecraftMaven/net/minecraft/minecraft-clientonly-deobf/26.2/minecraft-clientonly-deobf-26.2.jar'

def code(name):
    return subprocess.check_output(['javap', '-p', '-c', '-classpath', str(JAR), name], text=True)

def method(text, name):
    parts = re.split(r'(?=^  (?:public|private|protected|static))', text, flags=re.M)
    return next(part for part in parts if ' ' + name + '(' in part.splitlines()[0])

living = method(code('net.minecraft.client.renderer.entity.LivingEntityRenderer'), 'submit')
assert living.count('SubmitNodeCollector.submitModel:') == 1
assert living.index('SubmitNodeCollector.submitModel:') < living.index('RenderLayer.submit:')
avatar = method(code('net.minecraft.client.renderer.entity.player.AvatarRenderer'), 'extractRenderState')
assert 'AvatarRenderState.id:I' in avatar and 'Avatar.getId:()I' in avatar
feature = method(code('net.minecraft.client.renderer.feature.RenderTypeFeatureRenderer'), 'executeGroup')
assert 'PreparedRenderType.drawFromBuffer:(Lnet/minecraft/client/renderer/StagedVertexBuffer$ExecuteInfo;)V' in feature
assert 'List.clear:' not in feature and 'endDraw:' not in feature
prepared = code('net.minecraft.client.renderer.feature.FeatureRenderDispatcher$PreparedFrame')
phase = method(prepared, 'executePhase')
assert 'List.clear:' not in phase and 'finishExecute:' not in phase
assert 'finishExecute:' in method(prepared, 'close')
staged = method(code('net.minecraft.client.renderer.StagedVertexBuffer'), 'getExecuteInfo')
assert 'putfield' not in staged and 'endDraw:' not in staged
level = method(code('net.minecraft.client.renderer.LevelRenderer'), 'render')
assert level.count('FrameGraphBuilder.execute:') == 1
assert level.index('FrameGraphBuilder.execute:') < level.index('PreparedFrame.close:')
graph = code('com.mojang.blaze3d.framegraph.FrameGraphBuilder')
assert 'List.add:' in method(graph, 'assignResourceLifetimes')

# Require the exact narrow redirect descriptors to occur in installed bytecode.
for file, target_code in [('LocalBodySubmitMixin.java', living), ('LevelWorldCaptureMixin.java', level)]:
    source = (CLIENT/'mixin'/file).read_text()
    for target in re.findall(r'target = "([^"]+)"', source):
        if 'submitModel' not in target and 'FrameGraphBuilder;execute' not in target: continue
        owner, signature = target[1:].split(';', 1)
        name, descriptor = signature.split('(', 1)
        assert owner + '.' + name + ':(' + descriptor in target_code, target
config = json.loads((ROOT/'minecraft/fabric/src/client/resources/crafterhunter.client.mixins.json').read_text())
for mixin in ('LocalBodySubmitMixin','RenderTypeAccessor','BodyRenderTypeMixin','PreparedBodyDrawMixin'):
    assert mixin in config['client']
body = (CLIENT/'PlayerBodyCapture.java').read_text()
assert 'model instanceof PlayerModel' in body and 'avatar.id != mc.player.getId()' in body
assert 'BodyCapturePass.capturing() && bodyDraws.contains(prepared)' in body
capture = (CLIENT/'WorldCapture.java').read_text()
assert 'if (BodyCapturePass.restoring()) return;' in capture
assert 'if (issue && remaining > 0 && !inFlight)' in capture
assert 'inFlightBodyExcluded = PlayerBodyCapture.excludedAtIssue();' in capture
renderer = (ROOT/'native/mhw-renderer/renderer.cpp').read_text()
player = renderer.split('bool drawPlayer(ID3D11Texture2D* back,ID3D11Buffer* camera,ID3D11Buffer* ui) {',1)[1].split('// The Minecraft frame, drawn',1)[0]
assert 'worldMetadata.bodyExcluded' in player and 'pairedDepthStamp.matches(requestedDepth)' in player
assert 'if(bodyDepth==player::BodyDepth::Hidden) return false;' in player
assert 'if(bodyDepth==player::BodyDepth::SharedGuest) modelDepth=pairedDSV.Get();' in player
assert 'else context->ClearDepthStencilView(privateDSV.Get()' in player
assert 'OMSetRenderTargets(1,target.GetAddressOf(),modelDepth)' in player

with tempfile.TemporaryDirectory() as directory:
    out = Path(directory)
    subprocess.run(['g++','-std=c++20','-Wall','-Wextra','-Werror','-I',str(ROOT/'native/mhw-renderer'),
        str(ROOT/'native/mhw-renderer/tests/player_body_test.cpp'),'-o',str(out/'body')],check=True)
    subprocess.run([str(out/'body')],check=True)
    # The old policy draws a second model with its own cleared depth target.
    header = (ROOT/'native/mhw-renderer/player_body.hpp').read_text()
    mutant = re.sub(r'inline BodyDepth bodyDepth\([^}]+\}',
        'inline BodyDepth bodyDepth(bool, bool, bool) { return BodyDepth::Private; }', header)
    (out/'player_body.hpp').write_text(mutant)
    subprocess.run(['g++','-std=c++20','-I',str(out),str(ROOT/'native/mhw-renderer/tests/player_body_test.cpp'),
        '-o',str(out/'old-depth')],check=True)
    result = subprocess.run([str(out/'old-depth')],cwd=out,capture_output=True)
    assert result.returncode != 0, 'old duplicate/private-depth policy survived'

    # Run testBodyCapture first. Recompile only the production helper into a
    # private override directory; use the same actual Minecraft test classpath.
    cp = (ROOT/'minecraft/fabric/build/body-test/classpath.txt').read_text()
    original = (CLIENT/'BodyCapturePass.java').read_text()
    for name, broken, expected in [
        ('no-schedule-reset', original.replace('Schedule.reset(graph);','/* old unsafe replay */'), 'already assigned'),
        ('no-body-pass', original.replace('if (!filterBody)', 'if (true)'), 'body duplicated'),
        ('no-display-restore', original.replace('try { graph.execute(allocator, inspector); }','try { /* lost normal display */ }'), 'normal display differs')]:
        assert broken != original
        folder = out/name; folder.mkdir(); source = folder/'BodyCapturePass.java'; source.write_text(broken)
        subprocess.run(['javac','--release','25','-cp',cp,'-d',str(folder),str(source)],check=True)
        result = subprocess.run(['java','-cp',str(folder)+':'+cp,'BodyCapturePassTest'],capture_output=True,text=True)
        assert result.returncode != 0 and expected in result.stderr, (name,result.stdout,result.stderr)
        print('PASS mutant rejected:',name)
print('PASS pinned local-body batching/replay hooks, preserved buffers/capture policy and native single-body integration')
