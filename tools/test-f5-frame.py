#!/usr/bin/env python3
"""Headless F5 frame-mode regression; optional own-renderer observation JSON."""
from pathlib import Path
import json
import subprocess
import sys
import tempfile

root=Path(__file__).resolve().parents[1]
include=root/'native/mhw-renderer'
with tempfile.TemporaryDirectory() as directory:
    out=Path(directory)
    fixtures=[]
    for i,path in enumerate(sys.argv[1:]):
        rows=json.loads(Path(path).read_text())
        if isinstance(rows,dict):rows=rows['examples']
        lines=[]
        for row in rows:
            if not row.get('mismatched_camera_mode'):continue
            # The observation records conventional rows; production consumes
            # the managed matrix's column-vector memory layout.
            values=[row['cpu_camera_mode'],row['capture_mode']]
            for key in ('cpu_vp','expected_vp'):
                values += [row[key][r][c] for c in range(4) for r in range(4)]
            lines.append(' '.join(map(str,values)))
        assert lines,'No measured F5 mismatches in fixture'
        fixture=out/f'observed-{i}.txt';fixture.write_text('\n'.join(lines)+'\n');fixtures.append(str(fixture))
    def run(name):
        binary=out/name
        subprocess.run(['g++','-std=c++20','-Wall','-Wextra','-Werror','-I',str(out),'-I',str(include),
            str(include/'tests/f5_frame_test.cpp'),'-o',str(binary)],check=True)
        return subprocess.run([str(binary),*fixtures],capture_output=True,text=True)
    result=run('current');print(result.stdout,end='');assert result.returncode==0,result.stderr
    header=(include/'guest_camera_frame.hpp').read_text()
    original='if(mode_==selected.mode) return true;';assert header.count(original)==1
    override=out/'guest_camera_frame.hpp'
    override.write_text(header.replace(original,'if(mode_!=Unknown) return true;'))
    mutant=run('unguarded-mode')
    assert mutant.returncode and 'first-person frame drew new rear body' in mutant.stderr,mutant.stderr
    override.unlink()
    routing=(include/'composition_requests.hpp').read_text()
    old='full ? Fallback::None : Fallback::Overlay';assert routing.count(old)==1
    (out/'composition_requests.hpp').write_text(routing.replace(old,'full ? Fallback::Overlay : Fallback::Overlay'))
    mutant=run('overlay-fallback')
    assert mutant.returncode and 'refused full paired draw became overlay' in mutant.stderr,mutant.stderr
    print('PASS original premature-mode and full-refusal sky-overlay mutants rejected')

source=(include/'renderer.cpp').read_text()
assert 'guestCamera::sampleFrameCamera(viewProjection,' in source
assert source.index('guestCamera::service();',source.index('int CH_Frame(')) < source.index('guestCamera::sampleFrameCamera(viewProjection,')
assert 'if(!guestCamera::pairedFrameReady()) return false;' in source
assert 'if(!guestCamera::pairedFrameReady() || guestCamera::hideFirstPersonBody()) return false;' in source
assert 'composition::fallback(fullRequested,pairedRequested)' in source
