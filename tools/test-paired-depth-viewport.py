#!/usr/bin/env python3
"""Execute production viewport preparation and shader addressing off target.

The fixture uses the measured 1920x1080 allocation / 1440x810 writer mismatch.
This proves coordinate conversion and refusal policy, not GPU synchronization.
"""
from pathlib import Path
import re
import subprocess
import tempfile

import numpy as np

root = Path(__file__).resolve().parents[1]
source = (root/'native/mhw-renderer/renderer.cpp').read_text()


def check(text):
    helper = text.split('bool selectedDepthMapping(float* mapping) {', 1)[1]
    helper = 'bool selectedDepthMapping(float* mapping) {' + helper.split('\nbool preparePaired(', 1)[0]
    fixture = r'''
#include <cmath>
#include <mutex>
#include <vector>
#include <limits>
#include <cassert>
#include <iostream>
struct Viewport { float TopLeftX,TopLeftY,Width,Height,MinDepth,MaxDepth; };
struct Depth { Viewport debugViewport; unsigned long long debugViewportFrame; };
std::mutex gate;
unsigned width=1920,height=1080;
unsigned long long frame=100;
std::size_t lastSelected=0;
std::vector<Depth> depths{{{0,0,1440,810,0,1},100}};
''' + helper + r'''
int main() {
    float mapping[4];
    auto emit=[&]() {
        assert(selectedDepthMapping(mapping));
        for(float v:mapping) std::cout << v << ' ';
        std::cout << '\n';
    };
    emit(); // Actual reduced viewport; not allocation-derived scale.
    depths[0].debugViewport={0,0,1920,1080,0,1}; emit();
    depths[0].debugViewport={20,10,960,540,0,1}; emit();
    auto refuse=[&]() {
        for(float& v:mapping) v=-7;
        assert(!selectedDepthMapping(mapping));
        for(float v:mapping) assert(v==-7);
    };
    depths[0].debugViewportFrame=99; refuse();
    depths[0].debugViewportFrame=101; refuse();
    depths[0].debugViewportFrame=100;
    lastSelected=1; refuse(); lastSelected=0;
    for(auto bad:std::vector<Viewport>{
        {0,0,0,810,0,1}, {0,0,1440,-1,0,1}, {-1,0,1440,810,0,1},
        {0,-1,1440,810,0,1}, {481,0,1440,810,0,1}, {0,271,1440,810,0,1},
        {0,0,std::numeric_limits<float>::infinity(),810,0,1},
        {std::numeric_limits<float>::quiet_NaN(),0,1440,810,0,1},
        {0,std::numeric_limits<float>::quiet_NaN(),1440,810,0,1},
        {0,0,1440,std::numeric_limits<float>::quiet_NaN(),0,1}}) {
        depths[0].debugViewport=bad; refuse();
    }
}
'''
    with tempfile.TemporaryDirectory() as directory:
        p = Path(directory)
        (p/'test.cpp').write_text(fixture)
        subprocess.run(['g++', '-std=c++20', '-Wall', '-Wextra', '-Werror',
                        str(p/'test.cpp'), '-o', str(p/'test')], check=True)
        result = subprocess.run([str(p/'test')], check=True, capture_output=True, text=True)
    mappings = [np.array([float(v) for v in line.split()]) for line in result.stdout.splitlines()]
    assert len(mappings) == 3
    assert np.array_equal(mappings[0], [.75, .75, 0, 0])
    assert np.array_equal(mappings[1], [1, 1, 0, 0])
    assert np.array_equal(mappings[2], [.5, .5, 20, 10])
    assert 'selectedDepthMapping(pair.depthMapping)' in text
    assert 'selectedDepthMapping(parameters.depthMapping)' in text
    assert 'float inverse[16],eye[16],sizes[4],mapping[4],depthMapping[4]' in text
    for name, entry in [('PairedShader', 'PairedPS'), ('PairedFullShader', 'PairedFullPS')]:
        shader = text.split(f'constexpr char {name}[] = R"hlsl(', 1)[1].split(')hlsl";', 1)[0]
        assert re.search(r'float4 sizes;[^\n]*\n\s*float4 mapping;[^\n]*\n\s*float4 depthMapping;', shader)
        pixel_shader = shader.split(f'float4 {entry}(', 1)[1]
        assert 'hostDepth.Load(int3(PairedDepthTexel(input.pos.xy),0))' in pixel_shader
        assert 'input.pos.z<=host+0.000001' in pixel_shader
        expression = re.search(r'int2 PairedDepthTexel\(float2 pixel\)\s*{\s*return int2\((.*?)\);', shader, re.S)[1]
        # Evaluate the actual shader return expression, rather than a duplicate
        # reference formula. Only this small arithmetic expression is permitted.
        expression = expression.replace('depthMapping.xy', 'scale').replace('depthMapping.zw', 'origin')
        assert re.fullmatch(r'[a-zA-Z0-9_.*+()\s]+', expression)

        def texel(pixel, mapping):
            return np.asarray(eval(expression, {'__builtins__': {}},
                dict(pixel=np.array(pixel), scale=mapping[:2], origin=mapping[2:], floor=np.floor)), dtype=int)

        assert tuple(texel([781.945, 371.781], mappings[0])) == (586, 278)
        assert tuple(texel([1919.5, 1079.5], mappings[0])) == (1439, 809)
        assert tuple(texel([1919.5, 1079.5], mappings[1])) == (1919, 1079)
        assert tuple(texel([1919.5, 1079.5], mappings[2])) == (979, 549)
        # Real trigger: right/bottom unused storage is clear 0. An occluder at
        # the mapped location must reject the guest; direct full-size addressing
        # instead sees clear storage and lets the guest overlap the foreground.
        depth = np.zeros((1080, 1920), dtype=np.float32)
        depth[600:810, 1200:1440] = .8
        for pixel in ([1800.5, 1000.5], [1919.5, 1079.5]):
            x, y = texel(pixel, mappings[0])
            assert .2 <= depth[y, x] + .000001
            wrong_x, wrong_y = np.asarray(pixel, dtype=int)
            assert not .2 <= depth[wrong_y, wrong_x] + .000001


check(source)
mutant = source.replace('int2(floor(pixel*depthMapping.xy+depthMapping.zw))', 'int2(pixel)')
try:
    check(mutant)
except AssertionError:
    print('PASS: original full-size depth address mutant rejected')
else:
    raise AssertionError('Original address bug escaped the regression')
print('PASS: measured viewport, identity/offset mapping, stale/invalid refusal, both production shaders and foreground occlusion')
