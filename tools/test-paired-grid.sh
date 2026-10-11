#!/usr/bin/env bash
# Headless checks for the paired composite's draw arithmetic.
#
# Off-target only: compiles the header and its regression tests on the build
# host. Touches no game process and installs nothing. Whether the full-resolution
# path renders is a runtime question, not answered here.
set -euo pipefail
cd "$(dirname "$0")/.."
test_dir="$(mktemp -d)"
trap 'rm -rf "$test_dir"' EXIT

g++ -std=c++20 -Wall -Wextra -Werror \
    -I native/mhw-renderer \
    native/mhw-renderer/tests/paired_grid_test.cpp \
    -o "$test_dir/paired_grid_test"

"$test_dir/paired_grid_test"