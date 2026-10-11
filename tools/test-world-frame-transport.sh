#!/usr/bin/env bash
# Headless only: isolated temporary files, never the running frame channels.
set -euo pipefail
project_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
test_dir="$(mktemp -d)"
trap 'rm -rf "$test_dir"' EXIT
javac --release 25 -Xlint:all -Werror -d "$test_dir" \
  "$project_root/minecraft/fabric/src/client/java/dev/crafterhunter/client/WorldFrameChannel.java" \
  "$project_root/minecraft/fabric/src/client/java/dev/crafterhunter/client/PlayerModelChannel.java" \
  "$project_root/minecraft/fabric/tests/WorldFrameChannelTest.java"
java -cp "$test_dir" dev.crafterhunter.client.WorldFrameChannelTest "$test_dir/world.frame"
g++ -std=c++20 -Wall -Wextra -Werror -I "$project_root/native/mhw-renderer" \
  "$project_root/native/mhw-renderer/tests/world_frame_test.cpp" -o "$test_dir/reader"
"$test_dir/reader" "$test_dir/world.frame" "$test_dir/resized.frame"
python3 - "$project_root/native/mhw-renderer/player_model.hpp" "$test_dir/player_model.hpp" <<'PY'
from pathlib import Path
import sys
source=Path(sys.argv[1]).read_text()
old='if(paired) return capturedPose(snapshot,out);'
assert source.count(old)==1
Path(sys.argv[2]).write_text(source.replace(old,'(void)paired; // old newest-live-pose selection'))
PY
g++ -std=c++20 -Wall -Wextra -Werror -I "$test_dir" -I "$project_root/native/mhw-renderer" \
  "$project_root/native/mhw-renderer/tests/world_frame_test.cpp" -o "$test_dir/newest-mutant"
python3 - "$test_dir/newest-mutant" "$test_dir/world.frame" "$test_dir/resized.frame" <<'PY'
import resource, subprocess, sys
resource.setrlimit(resource.RLIMIT_CORE,(0,0))
result=subprocess.run(sys.argv[1:],capture_output=True)
assert result.returncode!=0, 'newest-pose mutant survived attachment timing regression'
print('PASS: newest-live-pose mutant rejected against older captured attachments')
PY
