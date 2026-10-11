# Exclusive-input filter: session handoff (2026-10-11)

## State

- Installed and live-tested filter (`CrafterHunter.Render.dll` SHA-256
  `981858aec9780b0114098e0c69df39f68e1a7392c63961e5f52f2e22d4598cde`),
  matched to the pinned MHW build 421810 (image base `0x140000000`). Cases 1,
  3 and 4 pass; case 2 fails on this build.
- Installed 2026-10-11 with the end-transition rewrite that targets case 2
  (installed SHA-256
  `257b9cc6514f5e81c75fdd98bfbe57f92352e1965e6c59dcd4798b0284a79a9e`, `cmp`-clean
  against the build; previous DLL kept in `build/rollback.78uMwX`). Deployed,
  **not runtime-accepted**: case 2 still needs its acceptance run. Build outputs
  are not byte-reproducible (PE timestamp), so identity comes from `cmp`, not a
  quoted build hash.
- Filter is DISARMED at rest. Nothing is patched until a request arrives.
- Repo clean apart from this work.

## What works (acceptance PASS, on the installed build)

- **Case 1 baseline:** normal movement and release, no filter.
- **Case 3 enable at rest:** with the filter armed and W held, the
  hunter does not move (0.94 units over 6s = render jitter only).
- **Case 4 disable:** with the filter off, movement resumes immediately.

## What does NOT work (case 2 FAIL, on the installed build)

- **Enable mid-run with W held:** the hunter keeps moving (195 units/s)
  and the trampoline stays silent.

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
- running `(1,4)` -> table `0x143f37fc0`, which contains `cMoveEnd` (index
  `0x94`, required result 0, destination `(1,3)`) and `cMoveTurn` (`0x9b`) and
  **not** `cMove`.

So the move predicate cannot stop a run - the run's exit is governed by the end
predicate, which was never filtered. The new build rewrites the local
`cMoveEnd` predicate's ineligible result (`-1`) to `0` while exclusive mode is
on, which asks the engine for the end transition it would have selected had the
stick been released. Full evidence in `docs/exclusive-input-design.md`.

An active run therefore should now end as soon as the filter is armed, without
a key release. **That is the claim to test, not an accepted result.**

## How to test (re-resolve every session/scene)

Addresses are NEVER persisted; they change per session and per scene
load. With MHW in a loaded offline scene:

1. Dry run: `python3 tools/resolve-exclusive-input.py --pid <MHW_PID>`
   prints local P/H/C, all cMove slots, cMoveTurn, cMoveEnd.
2. Arm: add `--publish` to write `render/exclusive-input.request`
   (atomic same-directory rename; the renderer claims then reads it).
3. Reply: `render/exclusive-input.reply` -> `armed=1 suppressed=N released=N`.
   **The reply is written ONLY when a request is processed; it is not
   refreshed live.** To read the current counters, publish a request (on or
   off) and read the reply. A stale count does NOT mean the trampoline stopped
   firing. `released` counts end-predicate rewrites, so it is the direct
   evidence that the case-2 path fired.
4. Disarm: publish request text `off`.

Case 2 run, with the new build: hold W until the hunter is running, publish the
`on` request *without releasing W*, then read the reply and the position. The
hunter should stop while W is still held and `released` should have advanced.
This is the acceptance this build needs; it has not been run.

Position readout for drift tests: `P+0x390` is the world-frame
position, `P+0x158` is the logic-frame position. The logic copy is the
authoritative freeze signal (it goes to exactly zero when stopped; the
world copy keeps ~1 unit of render jitter). At rest with the filter on
and W held, expect ~0.9 units over 6s. During a move with the filter
on, expect ~195 units/s.

## Open work

- **Case 2 acceptance:** the end-transition rewrite is built and its mechanism
  is established, but no live run has measured it. Until one does, treat case 2
  as open.
- **Turning** (`cMoveTurn`, index `0x9b`) is still never rewritten. It sits in
  the running-state table right after `cMoveEnd`, so it will be consulted while
  a run is being ended; whether the hunter can still turn under exclusive mode
  is a measurement, not an assumption.
- **Cases 5-8:** repeat toggles/duplicate enables, scene reload and
  re-resolution, plugin unload while enabled, menus/UI.
- **Cutscene/scripted movement:** the transition tables are not consulted there,
  so the filter does nothing (by design - cutscenes should not be blocked). Do
  not mistake a cutscene for a filter failure: the signature is both counters
  staying flat while the position moves.

## Cautions

- Do not claim exclusive input is done: case 2 is a stated criterion and its fix
  is unaccepted. The at-rest case works on the installed build.
- The end rewrite changes only a `-1` result. Any other result code the engine
  produces passes through untouched, so it cannot promote a state the engine was
  refusing - but that also means a cutscene whose end predicate returns `-1`
  would be rewritten. Untested; treat it as an open risk.
- `remove()` restores a slot only when it still carries our vtable, so a
  scene transition that recycles a slot is not written back (safe). The same
  guard now covers the cMoveEnd object.
- The counters are cumulative across arm cycles.
