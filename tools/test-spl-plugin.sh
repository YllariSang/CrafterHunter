#!/usr/bin/env bash
# Headless checks for the MHW plugin's pure logic: the render probe's rays, the
# Minecraft block packet, the placement lifecycle, and the terrain adapter's
# signature table, down-segment, agreement rule, and state machine.
#
# Runs outside the game. Nothing here resolves a signature or casts a ray; the
# point is that a typo in a byte pattern or in the state machine fails here
# instead of quietly disabling terrain in the next session.
set -euo pipefail
project_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
test_project="$project_root/native/mhw-spl-plugin/tests/CrafterHunter.MHW.Tests.csproj"

if ! command -v dotnet >/dev/null 2>&1; then
    printf '%s\n' "Missing required command: dotnet" >&2
    printf '%s\n' "Install a .NET 8 SDK, then run this script again." >&2
    exit 1
fi

# Reuse the restored assets from a previous build when the machine is offline.
run_arguments=(--project "$test_project" --configuration Release)
if [[ -f "$project_root/native/mhw-spl-plugin/tests/obj/project.assets.json" ]]; then
    run_arguments+=(--no-restore)
fi

dotnet run "${run_arguments[@]}"