#!/usr/bin/env bash
# Headless checks for the exclusive-input suppression policy.
#
# Off-target only: compiles the policy header and its regression tests on the
# build host. It touches no game process, installs nothing and reads no shared
# state. Runtime acceptance is a separate, manual gate.
set -euo pipefail
cd "$(dirname "$0")/.."
test_dir="$(mktemp -d)"
trap 'rm -rf "$test_dir"' EXIT

g++ -std=c++20 -Wall -Wextra -Werror \
    -I native/mhw-renderer \
    native/mhw-renderer/tests/exclusive_input_test.cpp \
    -o "$test_dir/exclusive_input_test"

"$test_dir/exclusive_input_test"