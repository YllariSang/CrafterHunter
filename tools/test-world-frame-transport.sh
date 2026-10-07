#!/usr/bin/env bash
# Headless only: isolated temporary files, never the running frame channels.
set -euo pipefail
project_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
test_dir="$(mktemp -d)"
trap 'rm -rf "$test_dir"' EXIT
javac --release 25 -Xlint:all -Werror -d "$test_dir" \
  "$project_root/minecraft/fabric/src/client/java/dev/crafterhunter/client/WorldFrameChannel.java" \
  "$project_root/minecraft/fabric/tests/WorldFrameChannelTest.java"
java -cp "$test_dir" dev.crafterhunter.client.WorldFrameChannelTest "$test_dir/world.frame"
g++ -std=c++20 -Wall -Wextra -Werror -I "$project_root/native/mhw-renderer" \
  "$project_root/native/mhw-renderer/tests/world_frame_test.cpp" -o "$test_dir/reader"
"$test_dir/reader" "$test_dir/world.frame" "$test_dir/resized.frame"
