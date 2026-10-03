#!/usr/bin/env bash
set -euo pipefail

project_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
scratch_dir="$(mktemp -d)"
bridge_pid=""
mhw_pid=""
minecraft_pid=""

cleanup() {
    for process_id in "$minecraft_pid" "$mhw_pid" "$bridge_pid"; do
        if [[ -n "$process_id" ]]; then
            kill "$process_id" 2>/dev/null || true
            wait "$process_id" 2>/dev/null || true
        fi
    done
    rm -rf -- "$scratch_dir"
}
trap cleanup EXIT

cd "$project_root"
cargo build --workspace

./target/debug/crafterhunter-bridge >"$scratch_dir/bridge.log" 2>&1 &
bridge_pid=$!
sleep 0.25

./target/debug/crafterhunter-probe mhw >"$scratch_dir/mhw.log" 2>&1 &
mhw_pid=$!
./target/debug/crafterhunter-probe minecraft >"$scratch_dir/minecraft.log" 2>&1 &
minecraft_pid=$!
sleep 2

rg -q "registered Mhw" "$scratch_dir/bridge.log"
rg -q "registered Minecraft" "$scratch_dir/bridge.log"
rg -q "CameraState from Mhw" "$scratch_dir/minecraft.log"

sed 's/^/[bridge] /' "$scratch_dir/bridge.log"
sed 's/^/[mhw] /' "$scratch_dir/mhw.log"
sed 's/^/[minecraft] /' "$scratch_dir/minecraft.log"
printf '%s\n' "CrafterHunter local transport smoke test passed."
