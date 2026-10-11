#!/usr/bin/env bash
# Headless only; no running game channels or requests.
set -euo pipefail
project_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
test_dir="$(mktemp -d)"
trap 'rm -rf "$test_dir"' EXIT
javac --release 25 -Xlint:all -Werror -d "$test_dir" \
  "$project_root/minecraft/fabric/src/client/java/dev/crafterhunter/client/WorldCopyState.java" \
  "$project_root/minecraft/fabric/src/client/java/dev/crafterhunter/client/CaptureTiming.java" \
  "$project_root/minecraft/fabric/tests/CaptureTimingTest.java"
java -cp "$test_dir" dev.crafterhunter.client.CaptureTimingTest
g++ -std=c++20 -Wall -Wextra -Werror -I "$project_root/native/mhw-renderer" \
  "$project_root/native/mhw-renderer/tests/stage_timing_test.cpp" -o "$test_dir/host"
"$test_dir/host"
python3 - "$project_root" "$test_dir" <<'PY'
from pathlib import Path
import subprocess, sys
root, out = map(Path, sys.argv[1:])
state = root/'minecraft/fabric/src/client/java/dev/crafterhunter/client/WorldCopyState.java'
mutant = out/'WorldCopyState.java'
mutant.write_text(state.read_text().replace('if (!depth) { depthNanos', 'if (true) { depthNanos'))
subprocess.run(['javac','--release','25','-d',str(out),str(mutant)],check=True)
result = subprocess.run(['java','-cp',str(out),'dev.crafterhunter.client.CaptureTimingTest'],capture_output=True)
assert result.returncode != 0, 'duplicate callback timestamp mutant survived'
subprocess.run(['javac','--release','25','-d',str(out),str(state)],check=True)
timing = root/'minecraft/fabric/src/client/java/dev/crafterhunter/client/CaptureTiming.java'
folded = out/'CaptureTiming.java'
folded.write_text(timing.read_text().replace('long[] spans = {callback - issue,', 'long[] spans = {service - issue,'))
subprocess.run(['javac','--release','25','-d',str(out),str(folded)],check=True)
result = subprocess.run(['java','-cp',str(out),'dev.crafterhunter.client.CaptureTimingTest'],capture_output=True)
assert result.returncode != 0, 'service-delay-as-copy-delay mutant survived'
source = (root/'minecraft/fabric/src/client/java/dev/crafterhunter/client/WorldCapture.java').read_text()
assert source.index('WorldFrameChannel.publish(') < source.index('captureTiming.record(')
assert 'copyState.completedNanos()' in source
host = (root/'native/mhw-renderer/renderer.cpp').read_text()
assert 'if(worldUploadEnabled && uploadTiming.due(GetTickCount64()))' in host
assert 'timing::DuplicateRead].record(readStart,readEnd)' in host
print('PASS: duplicate timestamp and folded service-delay mutants rejected; production timing boundaries pinned')
PY
