#!/usr/bin/env bash
# Explicit native opt-in. Existing managed plugin installer remains separate.
set -euo pipefail
project_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
user_home_directory="$(getent passwd "$(id -u)" | cut -d: -f6)"
game_directory="${1:-$user_home_directory/.local/share/Steam/steamapps/common/Monster Hunter World}"
expected=c2ebbbd2c49f216d484e31a5219bed419eb1e5e7d206d02cba040a3ab79d90ea
[[ -f "$game_directory/MonsterHunterWorld.exe" ]] || { echo 'MHW executable missing' >&2; exit 1; }
[[ "$(sha256sum "$game_directory/MonsterHunterWorld.exe" | cut -d' ' -f1)" == "$expected" ]] || {
    echo 'Unsupported MHW executable; refusing installation' >&2; exit 1;
}
if pgrep -f '(^|[/\\])MonsterHunterWorld\.exe($| )' >/dev/null; then
    echo 'Close MHW before installation. Use the explicit reload workflow only for development.' >&2; exit 1
fi
plugin_directory="$game_directory/nativePC/plugins/CSharp/CrafterHunter"
[[ -f "$plugin_directory/.crafterhunter-managed" ]] || { echo 'Install the managed CrafterHunter plugin first' >&2; exit 1; }
artifact="$project_root/native/mhw-renderer/build/CrafterHunter.Render.dll"
notice="$project_root/native/mhw-renderer/build/MinHook-LICENSE.txt"
[[ -f "$artifact" && -f "$notice" ]] || { echo 'Run bash tools/build-mhw-renderer.sh first' >&2; exit 1; }
backup_directory="$(mktemp -d "$project_root/native/mhw-renderer/build/rollback.XXXXXX")"
for name in CrafterHunter.Render.dll native-renderer.enabled MinHook-LICENSE.txt; do
    [[ ! -f "$plugin_directory/$name" ]] || cp -p -- "$plugin_directory/$name" "$backup_directory/"
done
install -m 0644 "$artifact" "$plugin_directory/CrafterHunter.Render.dll"
install -m 0644 "$notice" "$plugin_directory/MinHook-LICENSE.txt"
printf 'Native pre-UI renderer, pinned MHW 421810, DX11 only\n' > "$plugin_directory/native-renderer.enabled"
cmp "$artifact" "$plugin_directory/CrafterHunter.Render.dll"
printf 'Native renderer installed. Previous files preserved in: %s\n' "$backup_directory"
printf '%s\n' 'Enter a world in both games, then run: python tools/control-mhw-renderer.py place'
