#!/usr/bin/env bash
# Headless allocation/update policy; no game requests or installations.
set -euo pipefail
project_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
test_dir="$(mktemp -d)"
trap 'rm -rf "$test_dir"' EXIT
g++ -std=c++20 -Wall -Wextra -Werror -I "$project_root/native/mhw-renderer" \
  "$project_root/native/mhw-renderer/tests/world_upload_test.cpp" -o "$test_dir/upload"
"$test_dir/upload"
