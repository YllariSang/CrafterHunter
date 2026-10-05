#!/usr/bin/env bash
# Off-target unit tests for the shared-memory frame contract the guest and MHW's
# renderer will agree on. frame_transport.hpp carries no D3D types precisely so
# this can run on the host.
set -euo pipefail
cd "$(dirname "$0")/.."

out="$(mktemp -d)"
trap 'rm -rf "$out"' EXIT

g++ -std=c++20 -Wall -Wextra -Werror \
    -I native/mhw-renderer \
    native/mhw-renderer/tests/frame_transport_test.cpp \
    -o "$out/frame_transport_test"

"$out/frame_transport_test"
