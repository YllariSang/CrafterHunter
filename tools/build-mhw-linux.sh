#!/usr/bin/env bash
set -euo pipefail

project_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source_dir="$project_root/native/mhw-plugin"
build_dir="$source_dir/build-mingw64"
toolchain_file="$source_dir/toolchains/mingw64.cmake"

for required_command in cmake x86_64-w64-mingw32-g++; do
    if ! command -v "$required_command" >/dev/null 2>&1; then
        printf 'Missing required command: %s\n' "$required_command" >&2
        printf '%s\n' "Install CMake and your distribution's MinGW-w64 C++ toolchain." >&2
        exit 1
    fi
done

cmake \
    -S "$source_dir" \
    -B "$build_dir" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE="$toolchain_file"
cmake --build "$build_dir" --parallel

plugin_path="$build_dir/CrafterHunter.MHW.dll"
if [[ ! -f "$plugin_path" ]]; then
    printf '%s\n' "Build completed but the expected DLL was not found: $plugin_path" >&2
    exit 1
fi

printf 'Built: %s\n' "$plugin_path"
