#!/usr/bin/env bash
# Headless checks for the guest's frame contract: the row order a GPU readback
# arrives in, the byte count of a frame, the shared-memory meta line, and the
# request parsing the control tool and the mod both have to agree on.
set -euo pipefail
project_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
output_directory="$project_root/minecraft/fabric/build/headless-tests"
mkdir -p "$output_directory"

sources=(
    "$project_root/minecraft/fabric/src/client/java/dev/crafterhunter/client/WorldCopyState.java"
    "$project_root/minecraft/fabric/tests/WorldCopyStateTest.java"
    "$project_root/minecraft/fabric/src/client/java/dev/crafterhunter/client/FrameLayout.java"
    "$project_root/minecraft/fabric/src/client/java/dev/crafterhunter/client/FrameRequest.java"
    "$project_root/minecraft/fabric/src/client/java/dev/crafterhunter/client/FrameCopyState.java"
    "$project_root/minecraft/fabric/src/client/java/dev/crafterhunter/client/FrameChannel.java"
    "$project_root/minecraft/fabric/tests/FrameLayoutTest.java"
    "$project_root/minecraft/fabric/tests/FrameChannelTest.java"
)

javac --release 25 -Xlint:all -Werror -d "$output_directory" "${sources[@]}"
java -cp "$output_directory" dev.crafterhunter.client.FrameLayoutTest
java -cp "$output_directory" dev.crafterhunter.client.FrameChannelTest
java -cp "$output_directory" dev.crafterhunter.client.WorldCopyStateTest
