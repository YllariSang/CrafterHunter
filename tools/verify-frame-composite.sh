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
stamp="$(date +%Y%m%d-%H%M%S)"
# The per-run directory, not just its parent: created only the parent, every tee
# then failed with "No such file or directory" and the run reported no evidence
# while the capture itself had worked.
out="$evidence/$stamp"
mkdir -p "$out"

fail() { printf '%s\n' "$1" >&2; exit 1; }

game_pid() { pgrep -af "MonsterHunterWorld.exe" 2>/dev/null | awk '{for (i = 2; i <= NF; i++) if ($i ~ /^S:/) { print $1; exit }}'; }

[ -f "$log" ] || fail "no renderer log at $log - is MHW installed with the renderer?"
game="$(game_pid)"
[ -n "$game" ] || fail "MHW is not running, so there is nothing to verify"

mark="$(wc -l < "$log")"
printf 'MHW pid %s, renderer log marked at line %s\n' "$game" "$mark"

# The session start, so facts that hold once per session rather than per run - the
# pipeline being created, the depth candidates being found - are not reported missing
# merely because they happened before this run asked.
session="$(grep -n "renderer initialized" "$log" | tail -1 | cut -d: -f1)"
session="${session:-1}"

# The channel before we ask for anything, so a stale file from an earlier session
# cannot be mistaken for a live one.
#
# Deliberately NOT deleted. An earlier version removed it here, on the reasonable
# assumption that a fresh capture makes a fresh file. It does not: the guest holds
# the channel open for the life of the capture, so deleting the path leaves it
# writing to an unlinked inode that no reader can ever see. The guest then reported
# healthy publishing while the reader saw nothing, which is the worst possible
# combination to debug. The channel is recreated when the guest reopens it - on a
# size change, which is why a window nudge brings it back.
{
    echo "=== before ==="
    date
    if [ -f "$channel" ]; then
        stat -c 'channel %s bytes, modified %y' "$channel"
    else
        echo "channel absent"
    fi
} | tee "$out/before.txt"


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
# Session-wide facts, from the session start rather than from this run's mark.
sed -n "${session},\$p" "$log" > "$out/session.log"

verdict() {
    if grep -q "Minecraft frame pipeline ready" "$out/session.log"; then
        echo "  pipeline     : created"
    else
        echo "  pipeline     : NOT created (see any compile error in the session log)"
    fi
    # The upload line is guarded to fire once per session, so its absence says nothing
    # about whether uploads are happening now. Reporting "none" from it was wrong twice
    # over: it missed uploads entirely, and it could not tell a reader that never worked
    # from one working quietly. What says something is whether refusals are still arriving.
    uploaded="$(grep -m1 'First Minecraft frame uploaded' "$out/session.log" | cut -c1-110)"
    if [ -n "$uploaded" ]; then
        echo "  upload       : first upload seen this session - ${uploaded#*] }"
    else
        echo "  upload       : no upload has ever been logged this session"
    fi
    echo "  stall lines  : $(grep -c 'frame is not being published' "$out/session.log") in the whole session"
    echo "  last log tick: $(grep -oE '^\[[0-9]+\]' "$out/session.log" | tail -1 | tr -d '[]') ms since boot"
    echo "  note         : uploads after the first are silent by design. Liveness is judged by"
    echo "                 whether 'frame is not being published' has stopped, and by the"
    echo "                 screenshot - not by this count."

    if grep -q "Compositing Minecraft frame" "$out/renderer.log"; then
        echo "  composition  : $(grep -m1 'Compositing Minecraft frame' "$out/renderer.log" | cut -c1-120)"
    else
        echo "  composition  : not reached"
    fi
    local refusals
    refusals="$(grep -c "Minecraft frame refused" "$out/renderer.log")"
    echo "  refusals     : $refusals"
    grep "Minecraft frame refused" "$out/renderer.log" | sort -u | head -5 | sed 's/^/                /'
    grep -E "re-opened the Minecraft frame channel|opened via the" "$out/session.log" \
        | head -4 | sed 's/^/                /'
}
verdict | tee "$out/verdict.txt"

echo
echo "Evidence written to $out"
echo
echo "NOTE: the lines above prove the reader mapped the channel, uploaded pixels and"
echo "issued a draw. Only someone looking at the game can confirm Steve is visible,"
echo "and what 'build passes' cannot tell you is whether he looks right."
