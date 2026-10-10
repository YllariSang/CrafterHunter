# Exclusive-input filter: session handoff (2026-10-11)

## State

- Filter BUILT and INSTALLED (`CrafterHunter.Render.dll` SHA-256
  `981858aec9780b0114098e0c69df39f68e1a7392c63961e5f52f2e22d4598cde`),
  matched to the pinned MHW build 421810 (image base `0x140000000`).
- Filter is currently DISARMED. Nothing is patched at rest.
- Games at handoff: MHW running (offline scene), Minecraft running
  (Singleplayer, fabric). Both on the active workspace.
- Repo clean; latest commits are the resolver multi-slot fix, this
  handoff, and the design-doc acceptance record.

## What works (acceptance PASS)

- **Case 1 baseline:** normal movement and release, no filter.
- **Case 3 enable at rest:** with the filter armed and W held, the
  hunter does not move (0.94 units over 6s = render jitter only).
  This is the case that matters for exclusive input.
- **Case 4 disable:** with the filter off, movement resumes immediately.

## What does NOT work (case 2 FAIL)

- **Enable mid-run with W held:** the hunter keeps moving (195 units/s)
  and the trampoline stays silent. Movement is not stopped until W is
  released and re-pressed.

## Mechanism (proven live)

The filter gives each local cMove object a private vtable copy whose
`+0x30` slot (function `0x141934220`) points at a trampoline that runs
the real predicate and rewrites the result to `-1`. That predicate gates
the TRANSITION into a move state:

- At rest it is consulted every frame (~85/s), so the trampoline fires
  and `-1` blocks the transition -> no movement starts.
- During a move the move state's own update applies movement WITHOUT
  re-consulting the predicate, so the trampoline is silent and the run
  continues.

An active run ends only via `cMoveEnd`, which fires on key release; the
following re-press is then blocked by the predicate. So the filter
prevents STARTING movement, not CONTINUING it.

## How to test (re-resolve every session/scene)

Addresses are NEVER persisted; they change per session and per scene
load. With MHW in a loaded offline scene:

1. Dry run: `python3 tools/resolve-exclusive-input.py --pid <MHW_PID>`
   prints local P/H/C, all cMove slots, cMoveTurn, cMoveEnd.
2. Arm: add `--publish` to write `render/exclusive-input.request`
   (atomic same-directory rename; the renderer claims then reads it).
3. Reply: `render/exclusive-input.reply` -> `armed=1 suppressed=N`.
   **The reply is written ONLY when a request is processed; it is not
   refreshed live.** To read the current suppressed count, publish a
   request (on or off) and read the reply. A stale count does NOT mean
   the trampoline stopped firing.
4. Disarm: publish request text `off`.

Position readout for drift tests: `P+0x390` is the world-frame
position, `P+0x158` is the logic-frame position. The logic copy is the
authoritative freeze signal (it goes to exactly zero when stopped; the
world copy keeps ~1 unit of render jitter). At rest with the filter on
and W held, expect ~0.9 units over 6s. During a move with the filter
on, expect ~195 units/s.

## Open work

- **Case 2 (stop an in-progress run):** would require ending the move on
  arm - either invoke `cMoveEnd` when the filter arms, or gate the
  move-state update itself. Both are larger than the transition
  predicate and are NOT attempted. This is the prerequisite for a hunter
  that is already running when Minecraft takes ownership.
- **Cases 5-8:** repeat toggles/duplicate enables, scene reload and
  re-resolution, plugin unload while enabled, menus/UI.
- **Cutscene/scripted movement:** the cMove predicate is not consulted
  at all, so the filter correctly does nothing there (by design -
  cutscenes should not be blocked). Do not mistake a cutscene for a
  filter failure: the signature is the trampoline staying silent while
  the position moves.

## Cautions

- Do not claim exclusive input is done: case 2 is a stated criterion and
  fails. The at-rest case works; a running hunter does not stop.
- `remove()` restores a slot only when it still carries our vtable, so a
  scene transition that recycles a slot is not written back (safe).
- The suppression counter is cumulative across arm cycles.
