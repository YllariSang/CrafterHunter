#!/usr/bin/env bash
set -u

strict=0
game_directory=""
for argument in "$@"; do
    case "$argument" in
        --strict) strict=1 ;;
        -*)
            printf 'Unknown option: %s\n' "$argument" >&2
            printf 'Usage: %s [--strict] [MHW game directory]\n' "$0" >&2
            exit 2
            ;;
        *)
            if [[ -n "$game_directory" ]]; then
                printf '%s\n' "Only one MHW game directory may be supplied." >&2
                exit 2
            fi
            game_directory="$argument"
            ;;
    esac
done

failures=0

ok() {
    printf '[ok]   %s\n' "$1"
}

note() {
    printf '[info] %s\n' "$1"
}

missing() {
    printf '[miss] %s\n' "$1"
    failures=$((failures + 1))
}

command_status() {
    local command_name="$1"
    local purpose="$2"
    if command -v "$command_name" >/dev/null 2>&1; then
        ok "$command_name — $purpose"
    else
        missing "$command_name — $purpose"
    fi
}

printf '%s\n' "CrafterHunter Linux/Proton environment"
command_status cargo "Rust bridge and probes"
command_status dotnet ".NET command host for the MHW plugin build"
command_status java "Minecraft/Fabric runtime"
command_status sha256sum "safe build fingerprinting"

if command -v dotnet >/dev/null 2>&1; then
    if dotnet_version="$(dotnet --version 2>/dev/null)"; then
        dotnet_major="${dotnet_version%%.*}"
        if [[ "$dotnet_major" =~ ^[0-9]+$ ]] && (( dotnet_major >= 8 )); then
            ok ".NET SDK $dotnet_version"
        else
            missing ".NET 8+ SDK required; found $dotnet_version"
        fi
    else
        missing "dotnet host found, but no SDK is installed"
    fi
fi

if command -v java >/dev/null 2>&1; then
    java_line="$(java -version 2>&1 | sed -n '1p')"
    note "Java: $java_line"
fi

if command -v protontricks >/dev/null 2>&1; then
    ok "protontricks — installs SharpPluginLoader's prefix dependencies"
else
    missing "protontricks — required for dotnetdesktop8 and d3dcompiler_47 in the MHW prefix"
fi

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

if [[ -z "$game_directory" || ! -f "$game_directory/MonsterHunterWorld.exe" ]]; then
    missing "completed MHW installation (pass Steam's Installed Files directory if it is in another library)"
else
    game_directory="$(realpath "$game_directory")"
    ok "MHW executable: $game_directory/MonsterHunterWorld.exe"

    steamapps_directory="$(dirname "$(dirname "$game_directory")")"
    prefix_directory="$steamapps_directory/compatdata/582010/pfx"
    if [[ -d "$prefix_directory" ]]; then
        ok "Proton prefix: $prefix_directory"
    else
        missing "Proton prefix not created yet; launch MHW once through Steam"
    fi

    if [[ -f "$game_directory/ucrtbase.dll" ]]; then
        ok "SharpPluginLoader Linux bootstrap detected"
    else
        missing "SharpPluginLoader Linux release is not installed in the MHW directory"
    fi

    plugin_path="$game_directory/nativePC/plugins/CSharp/CrafterHunter/CrafterHunter.MHW.dll"
    if [[ -f "$plugin_path" ]]; then
        ok "CrafterHunter MHW plugin installed"
    else
        note "CrafterHunter MHW plugin is not installed yet"
    fi
fi

note "A Minecraft Java license cannot be detected locally; authentication remains the launcher's responsibility."

if (( failures > 0 )); then
    printf '%s\n' "Doctor found $failures missing prerequisite(s)."
    if (( strict )); then
        exit 1
    fi
else
    printf '%s\n' "Doctor found no missing prerequisites."
fi
