#!/usr/bin/env bash
# Off-target unit tests for the per-pixel compositing rule.
# frame_composite.hpp carries no D3D types precisely so this can run on the host,
# and the reversed-Z comparison is pinned here rather than trusted to a visual
# check: a backwards depth test still produces a plausible-looking image.
set -euo pipefail
cd "$(dirname "$0")/.."

out="$(mktemp -d)"
trap 'rm -rf "$out"' EXIT

g++ -std=c++20 -Wall -Wextra -Werror \
    -I native/mhw-renderer \
    native/mhw-renderer/tests/frame_composite_test.cpp \
    -o "$out/frame_composite_test"

"$out/frame_composite_test"
