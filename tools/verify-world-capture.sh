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
# Asking them separately is not tidiness. A refused copy is easy to misattribute,
# and the first live run produced exactly one driver error line and no publish,
# which is equally consistent with "the depth copy was refused" and "both copies
# were refused at a hook where no framebuffer is bound". Those need different
# fixes, and guessing between them is how the wrong thing gets fixed. The
# colour-only run settles it: if colour works at this hook, the hook is fine and
# depth is the specific problem. That is in fact what the first run showed, and
# the cause is now measured rather than argued - GlCommandEncoder attaches the
# source texture to GL_COLOR_ATTACHMENT0, a D32_FLOAT image is not colour
# renderable, and the framebuffer is incomplete. See
# tools/test-depth-readback.sh, which reproduces that refusal and the recipe
# that replaces it (DepthReadback) on this machine's drivers.
#
# A mode that produces no publish is reported as "refused or stalled", with the
# driver's own GL error quoted if there is one. It is not reported as a pass.
#
# It never touches the live frame channel. This capture writes its own files, and
# deleting a channel the renderer is using would be sabotage, not a reset.
#
# Each run archives everything it sees - the previous run's leftovers first,
# then each publish the moment it lands - into out/archive/<UTC stamp>/, because
# the live meta and summary are single files that the next request claims, and a
# later failure must be unable to overwrite an earlier success.
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

# Every artifact this run produces is copied into a fresh directory before
# anything is deleted or overwritten, so a later failure cannot erase an earlier
# success - the live files are claimed by the mod and reused on every request,
# which is exactly why they must not be the only copy of a passing result.
stamp=$(date -u +%Y%m%dT%H%M%SZ)
archive="$out/archive/$stamp"
mkdir -p "$archive"

close_archive() {
  [ -d "$archive" ] || return 0
  cp -f "$summary" "$archive/world-capture.txt" 2>/dev/null || true
  [ -f "$meta" ] && cp -f "$meta" "$archive/last-world-capture.meta" 2>/dev/null
  {
    printf 'run %s\n' "$stamp"
    grep -a -E "glReadPixels|OpenGL debug message|CrafterHunter\]" "$log" 2>/dev/null | tail -60
  } > "$archive/opengl-and-mod.log"
  printf '\nartifacts archived: %s\n' "$archive"
}
trap close_archive EXIT

# What earlier runs left behind is evidence too, and the first request below
# deletes the meta file before the mod writes a new one. Archive it now.
if [ -f "$meta" ] || [ -f "$summary" ]; then
  mkdir -p "$archive/preexisting"
  for stale in "$meta" "$summary" "$out/world-depth.f32" "$out/world-colour.rgba" \
      "$out/world-colour.png"; do
    [ -f "$stale" ] && cp -f "$stale" "$archive/preexisting/"
  done
fi

# The summary is written from the hook every 500ms, but this file may be left
# over from a PREVIOUS session. Grepping hookSeen=true here once printed
# "hook is live" from a summary the current session never wrote (run
# 2026-10-07, where the game had loaded a jar with no hook at all). Whether the
# hook fired *since the request* is decided inside run_mode, against the
# moment the request was published; here, only report whether a summary
# exists at all.
if [ ! -f "$summary" ]; then
  printf 'note: no summary at %s yet - the hook has never written\n\n' "$summary"
fi

# One mode: request it, wait for a publish, and report what happened.
# grep -c prints the count and still exits 1 when the count is 0, so `|| echo 0`
# would append a second line and make the arithmetic below read "0\n0" - which
# aborts the whole ;-list in bash and takes the status assignments with it.
gl_count() {
  local n
  n=$(grep -ac "$1" "$log" 2>/dev/null || true)
  printf '%s' "${n:-0}"
}

run_mode() {
  local verb="$1" label="$2"
  local gl_before
  gl_before=$(gl_count glReadPixels)

  rm -f "$meta"
  # Publish a complete command by same-directory rename, never partial writes.
  local temporary publish_epoch
  publish_epoch=$(date +%s)
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
  gl_after=$(gl_count glReadPixels)
  gl_new=$((gl_after - gl_before))

  if [ ! -f "$meta" ]; then
    printf '%-8s : NO PUBLISH after %ss\n' "$label" "$((waited / 4))"
    printf '           status: %s\n' "$(grep '^status=' "$summary" | cut -d= -f2-)"
    printf '           colourLanded=%s depthLanded=%s\n' \
      "$(grep '^colourLanded=' "$summary" | cut -d= -f2-)" \
      "$(grep '^depthLanded=' "$summary" | cut -d= -f2-)"
    # The summary refreshes from the hook every 500ms, so a summary written
    # after the request proves the hook ran and still did not publish; one
    # older than the request means this session's hook has not fired at all.
    local summary_epoch
    summary_epoch=$(stat -c %Y "$summary" 2>/dev/null || echo 0)
    if [ "$summary_epoch" -ge "$publish_epoch" ]; then
      printf '           hook    : fired since the request (summary refreshed),\n'
      printf '                       so the capture ran and did not publish\n'
    else
      printf '           hook    : NOT fired since the request - summary last written '
      date -d "@$summary_epoch" '+%F %T' 2>/dev/null || printf 'at epoch %s' "$summary_epoch"
      printf '\n                       (enter a world, or the loaded jar has no hook)\n'
    fi
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

  # Copy this publish out of the live files immediately: the next mode's first
  # request deletes the meta, and a mode that fails must not take this result
  # with it. The files copied are the ones this mode actually wrote - filing a
  # stale depth buffer under a colour run would be evidence about nothing.
  mkdir -p "$archive/$label"
  cp -f "$meta" "$archive/$label/"
  cp -f "$summary" "$archive/$label/world-capture.txt"
  local written artifact
  case "$verb" in
    colour)  written="$out/world-colour.rgba $out/world-colour.png" ;;
    depth)   written="$out/world-depth.f32" ;;
    capture) written="$out/world-depth.f32 $out/world-colour.rgba $out/world-colour.png" ;;
    *)       written="" ;;
  esac
  for artifact in $written; do
    [ -f "$artifact" ] && cp -f "$artifact" "$archive/$label/"
  done
  printf '           archived to %s\n' "$archive/$label"
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
  printf 'hook rather than either format: most likely no framebuffer is complete for\n'
  printf 'reading there, and both glReadPixels calls are refused.\n'
  printf 'The backend does throw on a refused copy (IllegalStateException after queueing\n'
  printf 'the fence task); that refusal is what disables capture fail-closed above.\n'
  exit 1
fi
if [ "$colour_ok" -ne 0 ] && [ "$depth_ok" -eq 0 ]; then
  printf 'Depth publishes but colour does not: the reverse of the expectation.\n'
  exit 1
fi
if [ "$colour_ok" -eq 0 ] && [ "$depth_ok" -ne 0 ]; then
  printf 'Colour publishes from the world-render hook and depth does not.\n'
  printf 'So the hook point is sound, the target is readable, and the failure is\n'
  printf 'specific to the depth half. Depth is no longer handed to the game copy path\n'
  printf '(which attaches a depth image to GL_COLOR_ATTACHMENT0 and gets refused); it\n'
  printf 'goes through DepthReadback, which attaches it to GL_DEPTH_ATTACHMENT and\n'
  printf 'checks framebuffer completeness before reading. So one of three things is\n'
  printf 'being reported above: the framebuffer was not complete, glReadPixels returned\n'
  printf 'an error, or the half never landed. The status line quotes which, and\n'
  printf 'tools/test-depth-readback.sh says whether this driver accepts the recipe.\n'
  printf '\nPaired capture is therefore NOT achieved, and the transport extension is\n'
  printf 'not started - there is no second attachment to carry yet.\n'
  exit 1
fi

if [ "$paired_ok" -eq 0 ]; then
  printf 'Both attachments published. This proves completion, NOT valid scene depth.\n'
  printf 'Next: tools/validate-world-depth.py --acceptance against a known block surface.\n'
  exit 0
fi

printf 'Both publish alone but not together. The pairing discipline is refusing to\n'
printf 'publish an incomplete pair, which is the intended behaviour - but it means\n'
printf 'one of the two copies is being dropped when issued in the same submit.\n'
exit 1
