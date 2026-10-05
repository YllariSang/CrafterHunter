#!/usr/bin/env bash
# Off-target unit tests for placing the guest's clock on ours. Both faults covered
# here were silent: a milliseconds-to-nanoseconds conversion short by a factor of
# ten thousand, and an offset that could only be learned after a frame had already
# passed the freshness gate, so disagreeing clocks deadlocked.
set -euo pipefail
cd "$(dirname "$0")/.."

out="$(mktemp -d)"
trap 'rm -rf "$out"' EXIT

g++ -std=c++20 -Wall -Wextra -Werror \
    -I native/mhw-renderer \
    native/mhw-renderer/tests/frame_clock_test.cpp \
    -o "$out/frame_clock_test"

"$out/frame_clock_test"
