#!/usr/bin/env bash
# Off-target unit tests for the frame source's decision logic. The mapping itself
# needs a file, so what is checked is every refusal around it: an unmapped
# source, a buffer too short to hold a frame, a stale frame, and a frame
# claiming more space than the buffer has.
set -euo pipefail
cd "$(dirname "$0")/.."

out="$(mktemp -d)"
trap 'rm -rf "$out"' EXIT

g++ -std=c++20 -Wall -Wextra -Werror \
    -I native/mhw-renderer \
    -DFRAME_SOURCE_PATH="\"$PWD/native/mhw-renderer/frame_source.hpp\"" \
    native/mhw-renderer/tests/frame_source_test.cpp \
    -o "$out/frame_source_test"

"$out/frame_source_test"
