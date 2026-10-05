#!/usr/bin/env bash
# Runtime evidence for the paired world colour+depth capture.
#
# This is the only check that can tell whether depth readback works at all.
# Having a D32_FLOAT depth attachment and an OpenGL copyTextureToBuffer with no
# format guard is an argument that it should; it is not a result. This reads what
# the capture actually recorded and is explicit about the difference between:
#
#   * "a pair was published" means both asynchronous readbacks of one capture
#     identity landed and were written together.
#   * "depth looks like depth" means the numbers have the shape depth must have:
#     in [0,1], varying across the frame, no non-finite values, and near/far
#     distinguishable. A buffer of zeros, of colour, or of one repeated value all
#     copy without complaint and would pass a weaker check than this one.
#   * Neither means distance is correct. Reconstructing it needs the projection
#     and a normalisation against MHW's reversed-Z, neither of which is done.
#
# It never touches the live frame channel. This capture writes its own files and
# deleting a channel the renderer is using would be sabotage, not a reset.
#
# Usage: tools/verify-world-capture.sh [frames]
set -uo pipefail
cd "$(dirname "$0")/.."

frames="${1:-2}"
game="$HOME/.minecraft/crafterhunter"
out="$game/out"
request="$game/world.request"
meta="$out/world-capture.meta"
summary="$out/world-capture.txt"

fail() { printf 'FAIL: %s\n' "$1"; exit 1; }

[ -f "$summary" ] || fail "no summary at $summary - is the game running with the new jar?"
grep -q 'hookSeen=true' "$summary" || fail "the world-render hook never fired (hookSeen is not true)"

printf 'capture requested: %s frame(s)\n' "$frames"
printf 'capture %s\n' "$frames" > "$request"

# The capture is queued from a render hook and lands a frame or two later.
waited=0
while [ ! -f "$meta" ]; do
  sleep 0.5
  waited=$((waited + 1))
  [ "$waited" -gt 60 ] && fail "no metadata after 30s - request was seen but nothing published"
done
sleep 0.5

printf '\n=== recorded metadata ===\n'
cat "$meta"

value() { grep "^$1=" "$meta" | head -1 | cut -d= -f2-; }

printf '\n=== what that has to satisfy ===\n'

identity="$(value identity)"
[ -n "$identity" ] || fail "no capture identity recorded - nothing can be claimed about pairing"
printf 'pairing identity present: %s\n' "$identity"

[ "$(value depthFormat)" = "D32_FLOAT" ] \
  || fail "depth format is $(value depthFormat), not the D32_FLOAT the target allocates"

width="$(value width)"; height="$(value height)"
[ "$width" -gt 0 ] 2>/dev/null && [ "$height" -gt 0 ] 2>/dev/null \
  || fail "dimensions are ${width}x${height}"

# The row-layout question. If the driver padded rows, a tight-stride read shears
# every row but the first, and the numbers below would still look plausible.
if [ "$(value depthRowsPadded)" = "true" ]; then
  fail "depth rows are padded ($((width * 4)) tight vs $(value depthBufferBytes) actual) - a tight-stride read would be wrong"
fi
printf 'row layout tight (depth %s bytes = %s x %s x 4)\n' \
  "$(value depthBufferBytes)" "$width" "$height"

nonfinite="$(value depthNonFinite)"
[ "$nonfinite" = "0" ] || fail "$nonfinite non-finite depth values - this is not a depth buffer"

min="$(value depthMin)"; max="$(value depthMax)"
python3 - "$min" "$max" <<'PY' || exit 1
import sys
lo, hi = float(sys.argv[1]), float(sys.argv[2])
if not (0.0 <= lo <= hi <= 1.0):
    print(f"FAIL: depth range [{lo}, {hi}] is not a window-space depth range")
    raise SystemExit(1)
PY
printf 'depth range [%s, %s] is inside window depth\n' "$min" "$max"

distinct="$(value depthDistinctApprox)"
[ "$distinct" -gt 4 ] 2>/dev/null || fail "only $distinct distinct depth values - this may be a flat buffer, not a scene"
printf 'depth varies: %s distinct values across the frame\n' "$distinct"

printf '\nOK: a colour+depth pair was published, and the depth has the shape depth must have.\n'
printf 'Still unknown: whether these depths linearise to the right distances, and\n'
printf 'whether they are comparable with MHW depth at all. Both need the projection\n'
printf 'and a normalisation against MHW reversed-Z, and neither is done.\n'