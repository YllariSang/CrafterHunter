#!/usr/bin/env bash
# Build and install the opt-in MHW cube probe; never launches either game.
set -euo pipefail

project_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
game_directory="${1:-/home/yllaris/.local/share/Steam/steamapps/common/Monster Hunter World}"
installed_plugin="$game_directory/nativePC/plugins/CSharp/CrafterHunter/CrafterHunter.MHW.dll"
printf '%s\n' 'Close MHW before replacing its plugin. This script will not launch a game.'
read -r -p 'MHW closed? Type yes: ' confirmation
[[ "$confirmation" == yes ]] || exit 1

if [[ -f "$installed_plugin" ]]; then
    backup_directory="$(mktemp -d "$project_root/native/mhw-spl-plugin/build/rollback.XXXXXX")"
    cp -- "$installed_plugin" "$backup_directory/CrafterHunter.MHW.dll"
    printf 'Previous plugin backed up to: %s\n' "$backup_directory"
fi

bash "$project_root/tools/build-mhw-spl.sh"
bash "$project_root/tools/install-mhw-spl-plugin.sh" install "$game_directory"

printf '%s\n' 'To enable the cube for this test, set MHW Steam launch options to:'
printf '%s\n' 'DOTNET_ROOT= WINEDLLOVERRIDES="ucrtbase=n,b" CRAFTERHUNTER_CUBE_PROBE=1 %command%'
printf '%s\n' 'To disable it later, remove CRAFTERHUNTER_CUBE_PROBE=1 and restart MHW.'
printf '%s\n' 'Start the bridge using bash tools/prepare-play.sh, then launch both games normally.'
