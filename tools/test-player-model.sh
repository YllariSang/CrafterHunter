#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
test_dir=$(mktemp -d)
trap 'rm -rf "$test_dir"' EXIT
g++ -std=c++20 -Wall -Wextra -Werror -I native/mhw-renderer native/mhw-renderer/tests/player_model_test.cpp -o "$test_dir/test"
"$test_dir/test" "$test_dir/player.asset" "$test_dir/player.pose"
cd minecraft/fabric
./gradlew testBakedPlayer -PplayerEvidence="$test_dir"
"$test_dir/test" "$test_dir/player.asset" "$test_dir/player.pose" actual
