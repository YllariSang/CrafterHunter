#!/usr/bin/env bash
set -euo pipefail
project_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cmake -S "$project_root/native/mhw-renderer" -B "$project_root/native/mhw-renderer/build" -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE="$project_root/native/mhw-plugin/toolchains/mingw64.cmake" \
    -DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc -DCMAKE_BUILD_TYPE=Release
cmake --build "$project_root/native/mhw-renderer/build"
# MinHook is statically linked; retain its redistribution notice with the DLL.
install -m 0644 "$project_root/native/mhw-renderer/build/_deps/minhook-src/LICENSE.txt" \
    "$project_root/native/mhw-renderer/build/MinHook-LICENSE.txt"
