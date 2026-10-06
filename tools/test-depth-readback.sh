#!/usr/bin/env bash
# Reproduces Minecraft's refused D32_FLOAT depth readback on this machine's own
# GL driver, and measures the recipe that reads depth correctly.
#
# The game is not involved and no request file is touched: the probe creates its
# own EGL context, writes a known depth pattern, and then asks both questions
# the capture path has to answer - does the backend's own recipe come back with
# GL_INVALID_FRAMEBUFFER_OPERATION, and what does a correct read look like?
#
# It runs twice: once on whatever GL vendor the machine hands out, and once with
# the vendor Minecraft itself logs, because a readback diagnosis measured on the
# wrong card is a diagnosis about the wrong driver.
#
# A build that compiles proves nothing here; only a driver's answer counts, so
# this script runs the probe instead of checking its source.
set -euo pipefail
cd "$(dirname "$0")/.."

out="$(mktemp -d)"
trap 'rm -rf "$out"' EXIT

if ! pkg-config --exists egl gl; then
    printf 'SKIP: pkg-config does not know egl and gl, so the GL probe cannot be built here.\n'
    exit 1
fi

read -r -a compile_flags <<< "$(pkg-config --cflags egl gl)"
read -r -a link_flags <<< "$(pkg-config --libs egl gl)"

gcc -std=c11 -Wall -Wextra -Werror "${compile_flags[@]}" \
    tools/gl-depth-readback-probe.c -o "$out/gl-depth-readback-probe" "${link_flags[@]}"

# The default vendor first: whatever this machine hands a plain GL context.
status=0
"$out/gl-depth-readback-probe" || status=$?

# Then the vendor the game actually runs on. Minecraft's own log says
# 'AMD Radeon 660M ... Mesa 26.2.4-arch1.1', and a diagnosis measured only on
# the other card would be a diagnosis about the wrong driver. Forcing the vendor
# through glvnd is the difference between a probe that answers for the game and
# one that answers for the machine.
mesa_vendor=/usr/share/glvnd/egl_vendor.d/50_mesa.json
if [ -f "$mesa_vendor" ]; then
    printf '\n== Mesa (the driver Minecraft runs on) ==\n'
    __EGL_VENDOR_LIBRARY_FILENAMES="$mesa_vendor" "$out/gl-depth-readback-probe" || status=1
else
    printf '\nno Mesa vendor at %s; the game-driver run is skipped\n' "$mesa_vendor"
fi

exit "$status"
