#!/usr/bin/env bash
set -euo pipefail

project_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
project_file="$project_root/native/mhw-spl-plugin/CrafterHunter.MHW.csproj"
output_directory="$project_root/native/mhw-spl-plugin/build/publish"

if ! command -v dotnet >/dev/null 2>&1; then
    printf '%s\n' "Missing required command: dotnet" >&2
    printf '%s\n' "Install a .NET 8 SDK, then run this script again." >&2
    exit 1
fi

if ! dotnet_version="$(dotnet --version 2>/dev/null)"; then
    printf '%s\n' "The dotnet host is installed, but no .NET SDK is available." >&2
    printf '%s\n' "Install a .NET 8 SDK, not only the runtime, then run this script again." >&2
    exit 1
fi
dotnet_major="${dotnet_version%%.*}"
if [[ ! "$dotnet_major" =~ ^[0-9]+$ ]] || (( dotnet_major < 8 )); then
    printf 'A .NET 8 or newer SDK is required; found: %s\n' "$dotnet_version" >&2
    exit 1
fi

publish_arguments=(
    "$project_file"
    --configuration Release
    --output "$output_directory"
    --no-self-contained
)

# Reuse an existing lock/assets file when building without network access. A
# fresh checkout still performs the normal restore on its first build.
if [[ -f "$project_root/native/mhw-spl-plugin/obj/project.assets.json" ]]; then
    publish_arguments+=(--no-restore)
fi

dotnet publish "${publish_arguments[@]}"

plugin_path="$output_directory/CrafterHunter.MHW.dll"
if [[ ! -f "$plugin_path" ]]; then
    printf 'Build completed but the plugin was not found: %s\n' "$plugin_path" >&2
    exit 1
fi

printf 'Built: %s\n' "$plugin_path"
