#!/usr/bin/env bash
# Off-target unit tests for the depth-candidate selection policy.
# selection.hpp carries no D3D types precisely so this can run on the host.
set -euo pipefail
cd "$(dirname "$0")/.."

out="$(mktemp -d)"
trap 'rm -rf "$out"' EXIT

g++ -std=c++20 -Wall -Wextra -Werror \
    -I native/mhw-renderer \
    native/mhw-renderer/tests/selection_test.cpp \
    -o "$out/selection_test"

"$out/selection_test"
echo "Depth selection checks passed: freshness, reversed-Z clear, target match, measured content, threshold boundary, discovery order, fail-closed."
