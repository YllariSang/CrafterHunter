#!/usr/bin/env bash
# The guest and the native reader, exercised together across the channel's lifecycle.
#
# This exists because each half had only ever been tested alone, and the fault it missed
# is invisible from either side. A deleted channel leaves the guest writing to an
# unlinked inode: the guest reports healthy, rising sequence numbers, and the reader sees
# nothing. Both statements are true and together they are a bug that neither test can
# see.
#
# So the Java side drives a native probe built from the real headers - frame_source.hpp,
# frame_transport.hpp, frame_clock.hpp - and every assertion is about what the reader
# reports at a path the guest really writes to.
set -euo pipefail
cd "$(dirname "$0")/.."

out="$(mktemp -d)"
trap 'rm -rf "$out"' EXIT

# 1. The native reader, built from the same headers the renderer uses.
g++ -std=c++20 -O1 -Wall -Wextra -Werror \
    -I native/mhw-renderer \
    native/mhw-renderer/tests/frame_reader_probe.cpp \
    -o "$out/frame_reader_probe"

# 2. The guest classes and both test programs.
classes="$out/classes"
mkdir -p "$classes"
javac --release 25 -Xlint:all -Werror -d "$classes" \
    minecraft/fabric/src/client/java/dev/crafterhunter/client/FrameLayout.java \
    minecraft/fabric/src/client/java/dev/crafterhunter/client/FrameChannel.java \
    minecraft/fabric/tests/FrameChannelRecoveryTest.java

# 3. The test uses its own temporary file and hands it to the probe, so it cannot race
#    the live game for /dev/shm/crafterhunter/frame.channel.
java -cp "$classes" dev.crafterhunter.client.FrameChannelRecoveryTest \
    "$out/frame_reader_probe"

echo "Frame recovery integration checks passed: guest and native reader agreed across"
echo "  publish, unlink, recreate, resize and restart."
