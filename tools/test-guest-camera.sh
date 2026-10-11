#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
test_dir=$(mktemp -d)
trap 'rm -rf "$test_dir"' EXIT
javac --release 25 -Xlint:all -Werror -d "$test_dir" \
  minecraft/fabric/src/client/java/dev/crafterhunter/client/GuestCameraChannel.java \
  minecraft/fabric/tests/GuestCameraChannelTest.java
java -cp "$test_dir" dev.crafterhunter.client.GuestCameraChannelTest "$test_dir/camera.pose"
g++ -std=c++20 -Wall -Wextra -Werror -I native/mhw-renderer \
  native/mhw-renderer/tests/guest_camera_test.cpp -o "$test_dir/test"
"$test_dir/test" "$test_dir/camera.pose"
python3 tools/test-guest-camera-controller.py
