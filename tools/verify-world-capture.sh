#!/usr/bin/env bash
# Runtime evidence for the paired world colour+depth capture.
#
# This is the only check that can tell whether depth readback works at all.
#
# It runs the three request modes in order, and the order is the diagnostic:
#
#   * "colour" asks for the colour attachment alone, at the world-render hook.
#   * "depth" asks for the depth attachment alone, at the same hook.
#   * "capture" asks for both.
#
# Asking them separately is not tidiness. A refused copy raises nothing in Java, and
# the first live run produced exactly one driver error line and no publish, which is
# equally consistent with "the depth copy was refused" and "both copies were refused
# at a hook where no framebuffer is bound". Those need different fixes, and guessing
# between them is how the wrong thing gets fixed. The colour-only run settles it: if
# colour works at this hook, the hook is fine and depth is the specific problem.
#
# A mode that produces no publish is reported as "refused or stalled", with the
# driver's own GL error quoted if there is one. It is not reported as a pass.
#
# It never touches the live frame channel. This capture writes its own files, and
# deleting a channel the renderer is using would be sabotage, not a reset.
#
# Usage: tools/verify-world-capture.sh
set -uo pipefail
cd "$(dirname "$0")/.."

game="$HOME/.minecraft/crafterhunter"
out="$game/out"
request="$game/world.request"
summary="$out/world-capture.txt"
meta="$out/world-capture.meta"
log="$HOME/.minecraft/logs/latest.log"

fail() { printf 'FAIL: %s\n' "$1"; exit 1; }

[ -f "$summary" ] || fail "no summary at $summary"
grep -q 'hookSeen=true' "$summary" \
  || fail "the world-render hook never fired (hookSeen is not true in $summary)"

printf 'hook is live at the world-render boundary\n\n'

# One mode: request it, wait for a publish, and report what happened.
run_mode() {
  local verb="$1" label="$2"
  local gl_before
  gl_before=$(grep -ac "glReadPixels" "$log" 2>/dev/null || echo 0)

  rm -f "$meta"
  # Publish a complete command by same-directory rename, never partial writes.
  local temporary
  temporary=$(mktemp "$game/world.request.XXXXXX")
  printf '%s 1\n' "$verb" > "$temporary"
  mv "$temporary" "$request"

  local waited=0
  while [ ! -f "$meta" ]; do
    sleep 0.25
    waited=$((waited + 1))
    # The in-flight timeout is 3s, plus slack for the summary write.
    if [ "$waited" -gt 32 ]; then
      break
    fi
  done

  local gl_after gl_new
  gl_after=$(grep -ac "glReadPixels" "$log" 2>/dev/null || echo 0)
  gl_new=$((gl_after - gl_before))

  if [ ! -f "$meta" ]; then
    printf '%-8s : NO PUBLISH after %ss\n' "$label" "$((waited / 4))"
    printf '           status: %s\n' "$(grep '^status=' "$summary" | cut -d= -f2-)"
    printf '           colourLanded=%s depthLanded=%s\n' \
      "$(grep '^colourLanded=' "$summary" | cut -d= -f2-)" \
      "$(grep '^depthLanded=' "$summary" | cut -d= -f2-)"
    if [ "$gl_new" -gt 0 ]; then
      printf '           driver said: %s\n' \
        "$(grep -a 'glReadPixels' "$log" | tail -1 | sed 's/.*message=//' | cut -c1-90)"
    fi
    return 1
  fi

  printf '%-8s : published\n' "$label"
  printf '           %s\n' "$(grep -E '^(depthMin|depthMax|colourNonZero|depthStats)=' "$meta" | tr '\n' ' ')"
  [ "$gl_new" -gt 0 ] && printf '           driver said: %s\n' \
    "$(grep -a 'glReadPixels' "$log" | tail -1 | sed 's/.*message=//' | cut -c1-90)"
  return 0
}

run_mode colour COLOUR; colour_ok=$?
echo
run_mode depth DEPTH;   depth_ok=$?
echo
run_mode capture PAIRED; paired_ok=$?

printf '\n=== what that establishes ===\n'
if [ "$colour_ok" -ne 0 ] && [ "$depth_ok" -ne 0 ]; then
  printf 'Neither attachment publishes at the world-render hook, so the problem is the\n'
  printf 'hook rather than either format: most likely no framebuffer is bound for\n'
  printf 'reading there, and both glReadPixels calls are refused.\n'
  printf 'A refused copy raises nothing in Java, which is why this had to be measured.\n'
  exit 1
fi
if [ "$colour_ok" -ne 0 ] && [ "$depth_ok" -eq 0 ]; then
  printf 'Depth publishes but colour does not: the reverse of the expectation.\n'
  exit 1
fi
if [ "$colour_ok" -eq 0 ] && [ "$depth_ok" -ne 0 ]; then
  printf 'Colour publishes from the world-render hook and depth does not.\n'
  printf 'So the hook point is sound, the target is readable, and the failure is\n'
  printf 'specific to reading the D32_FLOAT depth attachment. The GL backend has no\n'
  printf 'format guard, so it forwards the copy to glReadPixels, and the driver\n'
  printf 'refuses it. This is the exact risk: an accessible depth texture is not a\n'
  printf 'depth texture whose format the existing readback method supports.\n'
  printf '\nPaired capture is therefore NOT achieved, and the transport extension is\n'
  printf 'not started - there is no second attachment to carry yet.\n'
  exit 1
fi

if [ "$paired_ok" -eq 0 ]; then
  printf 'Both attachments publish individually and together. Paired capture works.\n'
  printf 'Next: tools/validate-world-depth.py against a block of known distance.\n'
  exit 0
fi

printf 'Both publish alone but not together. The pairing discipline is refusing to\n'
printf 'publish an incomplete pair, which is the intended behaviour - but it means\n'
printf 'one of the two copies is being dropped when issued in the same submit.\n'
exit 1
