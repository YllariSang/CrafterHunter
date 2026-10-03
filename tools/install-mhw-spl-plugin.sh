#!/usr/bin/env bash
set -euo pipefail

usage() {
    printf '%s\n' "Usage: $0 install|remove [MHW game directory]" >&2
}

action="${1:-}"
game_directory="${2:-}"
if [[ "$action" != "install" && "$action" != "remove" ]]; then
    usage
    exit 2
fi

project_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source_plugin="$project_root/native/mhw-spl-plugin/build/publish/CrafterHunter.MHW.dll"

if [[ -z "$game_directory" ]]; then
    user_home_directory="$(getent passwd "$(id -u)" | cut -d: -f6)"
    candidates=(
        "$user_home_directory/.local/share/Steam/steamapps/common/Monster Hunter World"
        "$user_home_directory/.steam/steam/steamapps/common/Monster Hunter World"
    )
    for candidate in "${candidates[@]}"; do
        if [[ -f "$candidate/MonsterHunterWorld.exe" ]]; then
            game_directory="$candidate"
            break
        fi
    done
fi

if [[ -z "$game_directory" ]]; then
    printf '%s\n' "MHW was not found in a default Steam library." >&2
    usage
    exit 2
fi

game_directory="$(realpath "$game_directory")"
if [[ ! -f "$game_directory/MonsterHunterWorld.exe" ]]; then
    printf 'MonsterHunterWorld.exe was not found in: %s\n' "$game_directory" >&2
    exit 2
fi

plugin_directory="$game_directory/nativePC/plugins/CSharp/CrafterHunter"
plugin_path="$plugin_directory/CrafterHunter.MHW.dll"
marker_path="$plugin_directory/.crafterhunter-managed"

if [[ "$action" == "remove" ]]; then
    if [[ ! -f "$marker_path" ]]; then
        printf '%s\n' "Nothing was removed: the managed-install marker is absent." >&2
        exit 1
    fi

    recorded_hash="$(awk -F= '$1 == "sha256" { print $2; exit }' "$marker_path")"
    if [[ -z "$recorded_hash" || ! -f "$plugin_path" ]]; then
        printf '%s\n' "Refusing removal: the managed installation is incomplete." >&2
        exit 1
    fi
    installed_hash="$(sha256sum "$plugin_path" | cut -d' ' -f1)"
    if [[ "$installed_hash" != "$recorded_hash" ]]; then
        printf '%s\n' "Refusing removal: the installed plugin changed after installation." >&2
        exit 1
    fi

    rm -- "$plugin_path" "$marker_path"
    rmdir -- "$plugin_directory" 2>/dev/null || true
    printf '%s\n' "Removed the CrafterHunter plugin. SharpPluginLoader was left untouched."
    exit 0
fi

if [[ ! -f "$source_plugin" ]]; then
    printf 'Plugin build not found: %s\n' "$source_plugin" >&2
    printf '%s\n' "Run ./tools/build-mhw-spl.sh first." >&2
    exit 1
fi
if [[ ! -f "$game_directory/ucrtbase.dll" ]]; then
    printf '%s\n' "SharpPluginLoader's Linux bootstrap (ucrtbase.dll) was not found." >&2
    printf '%s\n' "Install its official Linux release before installing CrafterHunter." >&2
    exit 1
fi
if [[ -e "$plugin_directory" && ! -f "$marker_path" ]]; then
    printf 'Refusing to use an unmanaged directory: %s\n' "$plugin_directory" >&2
    exit 1
fi
if [[ -f "$marker_path" && -f "$plugin_path" ]]; then
    recorded_hash="$(awk -F= '$1 == "sha256" { print $2; exit }' "$marker_path")"
    installed_hash="$(sha256sum "$plugin_path" | cut -d' ' -f1)"
    if [[ -z "$recorded_hash" || "$installed_hash" != "$recorded_hash" ]]; then
        printf '%s\n' "Refusing upgrade: the installed plugin changed after installation." >&2
        exit 1
    fi
fi

mkdir -p -- "$plugin_directory"
install -m 0644 -- "$source_plugin" "$plugin_path"
installed_hash="$(sha256sum "$plugin_path" | cut -d' ' -f1)"
marker_temporary="$plugin_directory/.crafterhunter-managed.tmp.$$"
printf 'format=1\nsha256=%s\n' "$installed_hash" >"$marker_temporary"
mv -- "$marker_temporary" "$marker_path"

printf 'Installed: %s\n' "$plugin_path"
printf '%s\n' 'Steam launch options required by SharpPluginLoader on Proton:'
printf '%s\n' 'DOTNET_ROOT= WINEDLLOVERRIDES="ucrtbase=n,b" %command%'
printf '%s\n' "Keep MHW offline/private while CrafterHunter is installed."
