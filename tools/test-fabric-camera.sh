#!/usr/bin/env bash
set -euo pipefail
project_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
output_directory="$project_root/minecraft/fabric/build/headless-tests"
source_directory="$project_root/minecraft/fabric/src/client/java/dev/crafterhunter/client"
mkdir -p "$output_directory"
javac --release 25 -Xlint:all -Werror -d "$output_directory" \
    "$source_directory/CameraState.java" \
    "$source_directory/CameraFeed.java" \
    "$source_directory/CameraLink.java" \
    "$project_root/minecraft/fabric/tests/CameraLinkTest.java"
java -cp "$output_directory" dev.crafterhunter.client.CameraLinkTest
