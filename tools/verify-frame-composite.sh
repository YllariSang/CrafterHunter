#!/usr/bin/env bash
# Runtime evidence for the Minecraft frame composite.
#
# This is the only check that can tell whether Steve appears in MHW. Everything
# else in the suite is a statement about code; this one reads what the renderer
# logged while the game was actually running, and it is deliberately explicit
# about the difference:
#
#   * "channel mapped and a frame uploaded" means the reader, the mapping, the
#     clock rule and the upload all worked. It is necessary and not sufficient.
#   * "composited" means drawFrameComposite returned true, so a draw call was
#     issued with a bound scene depth.
#   * Neither means the player saw anything. Only the person looking at the game
#     can confirm that, and this script says so rather than implying otherwise.
#
# Usage: tools/verify-frame-composite.sh [frames] [settle-seconds]
set -uo pipefail
cd "$(dirname "$0")/.."

frames="${1:-120}"
settle="${2:-12}"
plugin="$HOME/.local/share/Steam/steamapps/common/Monster Hunter World/nativePC/plugins/CSharp/CrafterHunter"
log="$plugin/render/renderer.log"
channel="/dev/shm/crafterhunter/frame.channel"
evidence="native/mhw-renderer/build/evidence/frame-composite"
mkdir -p "$evidence"
stamp="$(date +%Y%m%d-%H%M%S)"
out="$evidence/$stamp"

fail() { printf '%s\n' "$1" >&2; exit 1; }

game_pid() { pgrep -af "MonsterHunterWorld.exe" 2>/dev/null | awk '{for (i = 2; i <= NF; i++) if ($i ~ /^S:/) { print $1; exit }}'; }

[ -f "$log" ] || fail "no renderer log at $log - is MHW installed with the renderer?"
game="$(game_pid)"
[ -n "$game" ] || fail "MHW is not running, so there is nothing to verify"

mark="$(wc -l < "$log")"
printf 'MHW pid %s, renderer log marked at line %s\n' "$game" "$mark"

# The channel before we ask for anything, so a stale file from an earlier session
# cannot be mistaken for a live one.
{
    echo "=== before ==="
    date
    if [ -f "$channel" ]; then
        stat -c 'channel %s bytes, modified %y' "$channel"
    else
        echo "channel absent"
    fi
} | tee "$out/before.txt"

rm -f /dev/shm/crafterhunter/frame.channel

printf 'requesting %s frames\n' "$frames"
python3 tools/control-minecraft-frame.py capture --frames "$frames" >/dev/null 2>&1 || fail "capture request failed"
sleep "$settle"

{
    echo "=== after ==="
    date
    if [ -f "$channel" ]; then
        stat -c 'channel %s bytes, modified %y' "$channel"
        python3 - "$channel" <<'PY'
import struct, sys, time
path = sys.argv[1]
with open(path, "rb") as handle:
    head = handle.read(256)
magic, version = struct.unpack_from("<II", head, 0)
print(f"magic=0x{magic:08x} version={version}")
for slot in (0, 1):
    base = 128 + slot * 64
    sequence, captured, published = struct.unpack_from("<QQQ", head, base + 16)
    w, h = struct.unpack_from("<II", head, base + 40)
    print(f"  slot {slot}: seq={sequence} published={published} {w}x{h}")
    if published == 1:
        now = time.clock_gettime_ns(time.CLOCK_MONOTONIC)
        print(f"          guest stamp is {(now - captured) / 1e6:.0f} ms behind CLOCK_MONOTONIC")
PY
    else
        echo "channel absent - the guest did not publish"
    fi
    echo
    echo "capture summary:"
    cat "$HOME/.minecraft/crafterhunter/out/capture.txt" 2>/dev/null || echo "(none)"
} | tee "$out/after.txt"

# Only this run's lines, so an old success cannot be read as a new one.
tail -n "+$((mark + 1))" "$log" > "$out/renderer.log"
{
    echo "=== new renderer log lines ==="
    cat "$out/renderer.log"
} | tee -a "$out/after.txt"

echo
echo "--- verdict ---"
verdict() {
    if grep -q "Minecraft frame pipeline ready" "$out/renderer.log"; then
        echo "  pipeline     : created"
    else
        echo "  pipeline     : NOT created (see the compile error above, if any)"
    fi
    if grep -q "First Minecraft frame uploaded" "$out/renderer.log"; then
        echo "  upload       : $(grep -m1 'First Minecraft frame uploaded' "$out/renderer.log" | cut -c1-120)"
    else
        echo "  upload       : none"
    fi
    if grep -q "Compositing Minecraft frame" "$out/renderer.log"; then
        echo "  composition  : $(grep -m1 'Compositing Minecraft frame' "$out/renderer.log" | cut -c1-120)"
    else
        echo "  composition  : not reached"
    fi
    local refusals
    refusals="$(grep -c "Minecraft frame refused" "$out/renderer.log")"
    echo "  refusals     : $refusals"
    grep "Minecraft frame refused" "$out/renderer.log" | sort -u | head -5 | sed 's/^/                /'
    grep "re-opened the Minecraft frame channel" "$out/renderer.log" | head -2 | sed 's/^/                /'
}
verdict | tee "$out/verdict.txt"

echo
echo "Evidence written to $out"
echo
echo "NOTE: the lines above prove the reader mapped the channel, uploaded pixels and"
echo "issued a draw. Only someone looking at the game can confirm Steve is visible,"
echo "and what 'build passes' cannot tell you is whether he looks right."
