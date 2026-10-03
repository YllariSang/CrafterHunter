#!/usr/bin/env bash
# Run from a normal terminal; never launches games or account/launcher tools.
set -euo pipefail

project_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
mod_directory="${1:-/home/yllaris/.minecraft/mods}"
game_directory="${2:-/home/yllaris/.local/share/Steam/steamapps/common/Monster Hunter World}"
artifact="$project_root/minecraft/fabric/build/libs/crafterhunter-fabric-0.2.0.jar"
bridge="$project_root/target/debug/crafterhunter-bridge"
plugin="$game_directory/nativePC/plugins/CSharp/CrafterHunter/CrafterHunter.MHW.dll"

for required_file in "$artifact" "$bridge" "$plugin" "$game_directory/MonsterHunterWorld.exe" "$game_directory/ucrtbase.dll"; do
    if [[ ! -f "$required_file" ]]; then
        printf 'Missing required file: %s\n' "$required_file" >&2
        exit 1
    fi
done
if [[ ! -d "$mod_directory" || ! -w "$mod_directory" ]]; then
    printf 'Mods folder is missing or not writable: %s\n' "$mod_directory" >&2
    exit 1
fi
if command -v ss >/dev/null && ss -H -lun | awk '{print $4}' | rg -q '(^|:)38470$'; then
    printf '%s\n' 'UDP port 38470 is already in use. Stop the previous bridge before running this script.' >&2
    exit 1
fi
printf '%s\n' 'Close Minecraft and MHW before continuing. No game will be launched.'
read -r -p 'Both games closed? Type yes: ' confirmation
[[ "$confirmation" == yes ]] || exit 1

# Preserve previous CrafterHunter JARs outside mods; leave every other mod alone.
backup_directory="$(mktemp -d "$project_root/minecraft/fabric/build/rollback.XXXXXX")"
shopt -s nullglob
previous_mods=("$mod_directory"/crafterhunter-fabric-*.jar)
for previous_mod in "${previous_mods[@]}"; do
    mv -- "$previous_mod" "$backup_directory/"
done
if ! install -m 0644 -- "$artifact" "$mod_directory/crafterhunter-fabric-0.2.0.jar"; then
    printf 'Install failed. Previous JARs are safe in: %s\n' "$backup_directory" >&2
    exit 1
fi
cmp -- "$artifact" "$mod_directory/crafterhunter-fabric-0.2.0.jar"
printf 'Installed v0.2; previous JARs preserved in: %s\n' "$backup_directory"
printf '%s\n' 'MHW plugin and loader left unchanged.'
printf '%s\n' 'Launch both games using your usual profiles; enter a Minecraft world and an MHW expedition.'
printf '%s\n' 'In Minecraft: F3+P disables focus pausing; F7 toggles the link; F8 re-anchors.'
printf '%s\n' 'Keep this terminal open. Ctrl+C stops the bridge.'
exec "$bridge" 127.0.0.1:38470
