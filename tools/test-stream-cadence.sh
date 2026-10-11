#!/usr/bin/env bash
# Headless checks for the host's half of the stream pacing contract.
#
# Off-target only: compiles the header and its regression tests on the build
# host. Touches no game process and installs nothing.
set -euo pipefail
cd "$(dirname "$0")/.."
test_dir="$(mktemp -d)"
trap 'rm -rf "$test_dir"' EXIT

g++ -std=c++20 -Wall -Wextra -Werror \
    -I native/mhw-renderer \
    native/mhw-renderer/tests/stream_cadence_test.cpp \
    -o "$test_dir/stream_cadence_test"

"$test_dir/stream_cadence_test"