#!/usr/bin/env bash
# Build and install the opt-in MHW cube probe; never launches either game.
set -euo pipefail

project_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
printf '%s\n' 'Close MHW before replacing its plugin. This script will not launch a game.'
read -r -p 'MHW closed? Type yes: ' confirmation
[[ "$confirmation" == yes ]] || exit 1

bash "$project_root/tools/build-mhw-spl.sh"
bash "$project_root/tools/install-mhw-spl-plugin.sh" install "${1:-}"

printf '%s\n' 'To enable the cube for this test, set MHW Steam launch options to:'
printf '%s\n' 'DOTNET_ROOT= WINEDLLOVERRIDES="ucrtbase=n,b" CRAFTERHUNTER_CUBE_PROBE=1 %command%'
printf '%s\n' 'To disable it later, remove CRAFTERHUNTER_CUBE_PROBE=1 and restart MHW.'
printf '%s\n' 'Start the bridge using bash tools/prepare-play.sh, then launch both games normally.'
