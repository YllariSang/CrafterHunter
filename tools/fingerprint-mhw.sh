#!/usr/bin/env bash
set -euo pipefail

game_directory="${1:-}"

if [[ -z "$game_directory" ]]; then
    user_home_directory="$(getent passwd "$(id -u)" | cut -d: -f6)"
    candidate_directories=(
        "$user_home_directory/.local/share/Steam/steamapps/common/Monster Hunter World"
        "$user_home_directory/.steam/steam/steamapps/common/Monster Hunter World"
    )
    for candidate_directory in "${candidate_directories[@]}"; do
        if [[ -f "$candidate_directory/MonsterHunterWorld.exe" ]]; then
            game_directory="$candidate_directory"
            break
        fi
    done
fi

if [[ -z "$game_directory" ]]; then
    printf '%s\n' "MHW was not found in a default Steam library." >&2
    printf '%s\n' "In Steam, open Properties > Installed Files > Browse, then pass that directory:" >&2
    printf '%s\n' './tools/fingerprint-mhw.sh "/path/to/Monster Hunter World"' >&2
    exit 2
fi

game_directory="$(realpath "$game_directory")"
executable_path="$game_directory/MonsterHunterWorld.exe"
if [[ ! -f "$executable_path" ]]; then
    printf 'MonsterHunterWorld.exe was not found in: %s\n' "$game_directory" >&2
    exit 2
fi

steamapps_directory="$(dirname "$(dirname "$game_directory")")"
manifest_path="$steamapps_directory/appmanifest_582010.acf"
compatibility_prefix="$steamapps_directory/compatdata/582010/pfx"
build_id="unknown"
if [[ -f "$manifest_path" ]]; then
    detected_build_id="$(awk -F'"' '/"buildid"/ { print $4; exit }' "$manifest_path")"
    if [[ -n "$detected_build_id" ]]; then
        build_id="$detected_build_id"
    fi
fi

printf 'Path: %s\n' "$executable_path"
printf 'SizeBytes: %s\n' "$(stat -c '%s' "$executable_path")"
printf 'LastWriteUtc: %s\n' "$(date -u -r "$executable_path" '+%Y-%m-%dT%H:%M:%SZ')"
printf 'SHA256: %s\n' "$(sha256sum "$executable_path" | cut -d' ' -f1)"
printf 'SteamBuildId: %s\n' "$build_id"
printf 'ProtonPrefix: %s\n' "$compatibility_prefix"
printf 'ProtonPrefixExists: %s\n' "$([[ -d "$compatibility_prefix" ]] && printf yes || printf no)"
printf 'Kernel: %s\n' "$(uname -srmo)"
printf '%s\n' "Share this text output only. Do not upload the executable."
