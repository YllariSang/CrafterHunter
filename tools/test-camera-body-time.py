#!/usr/bin/env python3
"""Headless paired-camera timing regression and original newer-camera mutant."""
from pathlib import Path
import subprocess
import sys
import tempfile

root = Path(__file__).resolve().parents[1]
include = root/'native/mhw-renderer'
fixture = include/'tests/camera_body_time_test.cpp'
with tempfile.TemporaryDirectory() as directory:
    out = Path(directory)
    def build(binary):
        subprocess.run(['g++', '-std=c++20', '-Wall', '-Wextra', '-Werror',
            '-I', str(out), '-I', str(include), str(fixture), '-o', str(binary)], check=True)
    build(out/'test')
    subprocess.run([str(out/'test'), *sys.argv[1:]], check=True)
    header = (include/'guest_camera.hpp').read_text()
    old = 'render.position=captured.cameraPosition;'
    assert header.count(old) == 1
    (out/'guest_camera.hpp').write_text(header.replace(old, 'render.position=control.position;'))
    build(out/'newer-camera')
    result = subprocess.run([str(out/'newer-camera')], capture_output=True, text=True)
    assert result.returncode != 0 and 'flying body/camera projection times disagree' in result.stderr, result.stderr
    print('PASS: newer independent camera position mutant rejected by flying projection oracle')

source = (include/'renderer.cpp').read_text()
assert 'gc::makePairedPlan(worldMetadata,sample' in source
assert ': gc::makePlan(sample,' in source  # Standalone camera/fast-body behavior retained.
assert 'candidate.expires=lease.advancedAt+500' in source
assert 'worldFreshness.live(now)' in source and 'applied-capture=%llu' in source
