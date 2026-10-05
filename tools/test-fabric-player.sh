#!/usr/bin/env bash
set -euo pipefail
project_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
output_directory="$project_root/minecraft/fabric/build/headless-tests"
source_list="$project_root/tools/fabric-headless-sources.txt"
mkdir -p "$output_directory"

# Shares its dependency list with the camera check, so the two can never drift.
sources=()
while IFS= read -r line; do
    [[ -z "$line" || "$line" == \#* ]] && continue
    sources+=("$project_root/$line")
done < "$source_list"

javac --release 25 -Xlint:all -Werror -d "$output_directory" \
    "${sources[@]}" \
    "$project_root/minecraft/fabric/tests/PlayerLinkTest.java"
java -cp "$output_directory" dev.crafterhunter.client.PlayerLinkTest
