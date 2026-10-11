# Exclusive-input filter: session handoff (2026-10-11)

## State

- Filter is DISARMED at rest. Nothing is patched until a request arrives, and
  every acceptance run ends with a disarm.
- Installed and live-tested against the pinned MHW build 421810 (image base
  `0x140000000`). The end-transition rewrite is the current build (installed
  SHA-256
  `257b9cc6514f5e81c75fdd98bfbe57f92352e1965e6c59dcd4798b0284a79a9e`, `cmp`-clean
  against the build; previous DLL kept in `build/rollback.78uMwX`). **All four
  cases 1-4 pass.** The earlier build that only rewrote `cMove` is preserved
  there and is what case 2 failed on.

## Acceptance (all four cases)

`tools/accept-exclusive-input.py --pid <MHW_PID>` runs three phases with the
key held across all of them and reports units/s for every player field that
moved, so the numbers come from the game's own position.

| Case | Result | Evidence |
|---|---|---|
| 1. Baseline, no filter | PASS | 307-375 units/s; release stops |
| 2. Enable mid-run, W held | PASS | 373 -> **0.58** -> 291 units/s across arm/disarm |
| 3. Enable at rest, W held | PASS | 0.94 units over 6 s (render jitter only) |
| 4. Disable, W held | PASS | resumes immediately |

Case 2, two consecutive runs, filter armed at t=7 s with W never released:
`P+0x390` moved 373.35 then 333.46 units/s, fell to **0.58** and **0.67**
while armed, and returned to 290.75 and 290.42 after the disarm. The
transition state agrees: `(1,2)` -> `(1,3)`, which is the end transition's own
destination, 0.6 s after the arm -> `(1,0)` idle -> back to `(1,2)` 0.6 s after
the disarm. `released` advanced by exactly one per arm cycle. Two runs, two
different scenes: in the later one the run occupies state `(1,2)` rather than
`(1,4)`, and that table carries `cMoveEnd` and no `cMove` just the same.

## What does NOT work

Nothing in cases 1-4. The earlier build that rewrote only `cMove` failed case
2 (195 units/s, trampoline silent) - that failure is what identified the wrong
object, and it is kept in the design doc rather than edited away.

## Mechanism (proven live, then explained statically)

The filter gives each local cMove object a private vtable copy whose
`+0x30` slot (function `0x141934220`) points at a trampoline that runs
the real predicate and rewrites the result to `-1`. That predicate gates
the TRANSITION into a move state:

- At rest it is consulted every frame (~85/s), so the trampoline fires
  and `-1` blocks the transition -> no movement starts.
- During a move the engine consults a different object, so the trampoline
  is silent and the run continues.

The "different object" is now identified. The local hunter's transition set is
`T = H+0xa00`, scanned per frame by `0x14026bd10` (via the thunk at
`0x14118e480`), with a per-state fallback at `0x14026bf70`. Each current state
selects its own table of transition records:

- idle `(1,0)` -> table `0x143f374b0`, which contains `cMove` (index `0x93`)
  and **not** `cMoveEnd`;
- every locomotion state -> `cMoveEnd` (index `0x94`, required result 0) and
  `cMoveTurn` (`0x9b`) and **not** `cMove`. Observed as `(1,4)` in one scene
  (table `0x143f37fc0`) and as `(1,2)` in another (table `0x143f37940`).

So the move predicate cannot stop a run - the run's exit is governed by the end
predicate, which was never filtered. The current build rewrites the local
`cMoveEnd` predicate's ineligible result (`-1`) to `0` while exclusive mode is
on, which asks the engine for the end transition it would have selected had the
stick been released. Full evidence in `docs/exclusive-input-design.md`.

An active run ends as soon as the filter is armed, without a key release -
measured, not assumed.

## How to test (re-resolve every session/scene)

Addresses are NEVER persisted; they change per session and per scene
load. With MHW in a loaded offline scene:

1. Dry run: `python3 tools/resolve-exclusive-input.py --pid <MHW_PID>`
   prints local P/H/C, all cMove slots, cMoveTurn, cMoveEnd.
2. Acceptance, hands off: `python3 tools/accept-exclusive-input.py --pid <MHW_PID>`.
   Tell the user to hold the key for the whole run first; it arms mid-run,
   disarms, and prints units/s per moving field.
3. Manual equivalent: `--publish` on the resolver writes
   `render/exclusive-input.request` (atomic same-directory rename; the renderer
   claims then reads it); `off` disarms.
4. Reply: `render/exclusive-input.reply` -> `armed=1 suppressed=N released=N`.
   **The reply is written ONLY when a request is processed; it is not
   refreshed live.** To read the current counters, publish a request (on or
   off) and read the reply. A stale count does NOT mean the trampoline stopped
   firing. `released` counts end-predicate rewrites, so it is the direct
   evidence that the case-2 path fired.

Position readout: the world-frame copy is at `P+0x390` and the logic-frame copy
at `P+0x160` (the handoff previously said `P+0x158`, which is the field *after*
the triple; both copies track the same motion and the acceptance tool reports
every field that moved rather than trusting one). Expect ~300-375 units/s while
running with the filter off, and under 1 unit/s with it on.

## Open work

- **Cases 5-8:** repeat toggles/duplicate enables, scene reload and
  re-resolution, plugin unload while enabled, menus/UI.
- **Turning** (`cMoveTurn`, index `0x9b`) is still never rewritten. It sits in
  the locomotion-state table right after `cMoveEnd`, so it will be consulted
  while a run is being ended; whether the hunter can still turn under exclusive
  mode is a measurement, not an assumption.
- **Cutscene/scripted movement:** the transition tables are not consulted there,
  so the filter does nothing (by design - cutscenes should not be blocked). Do
  not mistake a cutscene for a filter failure: the signature is both counters
  staying flat while the position moves.

## Cautions

- Cases 1-4 pass. Do not read that as exclusive input being finished: cases 5-8
  and turning are open, and the cutscene risk below is untested.
- The end rewrite changes only a `-1` result. Any other result code the engine
  produces passes through untouched, so it cannot promote a state the engine was
  refusing - but that also means a cutscene whose end predicate returns `-1`
  would be rewritten. Untested; treat it as an open risk.
- `remove()` restores a slot only when it still carries our vtable, so a
  scene transition that recycles a slot is not written back (safe). The same
  guard now covers the cMoveEnd object.
- The counters are cumulative across arm cycles.
