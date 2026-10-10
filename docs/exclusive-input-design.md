# Exclusive input: suppression design

2026-10-11. **Design proposal, not implemented.** No suppression code exists.
Rendering, pose cadence and installed game binaries are unchanged.

This document turns the findings in `docs/exclusive-input-investigation.md`
into a concrete, scoped, reversible design. It is written for review before any
code is written.

## Goal and non-goals

**Goal.** While exclusive mode is enabled, the local hunter must not start new
movement, while native end/stop processing continues to run normally.

**Non-goals, explicitly out of scope.** Attacks and items; UI and camera input;
controller input; scripted/AI movement semantics beyond "does not break";
replacing the existing focus-handoff mechanism.

## What the evidence supports

1. Physical keyboard movement runs
   `K+0x26c8` bit 29 -> `K+0x27ac` -> `S+0xcf8/+0xcfc` -> query ->
   `C+0x960` magnitude -> cMove magnitude predicate -> `(1,4)` -> `cActRun`.
   Confirmed live this session: bit 29 (`0x20000000`) observed with magnitude 1.0.
2. The alternate timers `H+0x4af8/+0x4afc` are **not** on that path. Their only
   arming route is a state machine on `H+0x4af0`, and that field is written from
   a configuration struct field `[rdi+0x140]` by an applier that consults no
   device. Live capture: ordinary walking never left state `0x0` and never armed
   either timer.
3. The magnitude/threshold accessor `0x14122ff50` is a pure predicate
   (`[rcx+0x960] >= threshold`), but it has **22 call sites across 16
   functions**. Filtering there is too broad.
4. `cMoveEnd` inverts the magnitude result and advances its own timer with
   `P+0x68`. So zeroing the magnitude would also *enable* end processing — but
   the accessor's fan-out makes that approach unsafe anyway.

## Chosen interception point: the exact local cMove instance

The generic dispatch `0x14026ba00` resolves the concrete transition object and
calls its virtual `+0x30` predicate. Filtering **only the exact local cMove
object**, and only when exclusive mode is enabled, is the narrowest boundary
available.

Resolved live for the current session (all three carry `+0x10 = P`,
`+0x20 = C`):

| Instance | Address | cActRun target |
| --- | --- | --- |
| cMove | `0x5e23e140` | destination `(1,4)` |
| cMoveTurn | `0x5e23e770` | - |
| cMoveEnd | `0x5e23e020` | destination `(1,5)` |

### Rejected alternatives

| Candidate | Why rejected |
| --- | --- |
| Magnitude accessor `0x14122ff50` | 16 distinct consumers, including camera and UI paths. |
| Shared aggregate `S+0xcf8/+0xcfc` | Shared by every consumer; clearing it is a global input clear. |
| Keyboard writer / `WriteInput` detour | Already loader-owned; stacking detours has no safe chaining contract. |
| Patch/freeze `C+0x960` | Suppresses camera/UI reads and controller input together. |
| Whole `cMove`/`cMoveTurn`/`cMoveEnd` | Blanket filtering of all transitions; loses end processing. |
| The alternate timers | Never armed during physical movement; irrelevant to this goal. |

## Scope: exact instance, verified identity

A suppressed call must match **all** of:

- the resolved transition object's vtable equals the cMove vtable, **and**
- `[obj+0x10] == P` for the resolved local player, **and**
- exclusive mode is currently enabled.

Matching on the address alone is not acceptable: addresses change per session
and per scene load. Matching on the vtable alone is not acceptable: 216
transition objects exist in a typical scene, most of them other entities.

**Instance resolution is a runtime step, not a constant.** The procedure proven
this session is: search the writable heap for the known cMove / cMoveTurn /
cMoveEnd vtable pointers, then keep only hits whose `+0x10` equals the resolved
local player. The player itself is found by the same validated chain
(`H = [P+0x14f8]`, `[H+0xdb0] == P`, `[C+0x30] == P`, `C = H+0x10`).

## Semantics

**While enabled**

- cMove for the exact local instance: force the predicate result to `-1`
  (ineligible). No new run is selected or entered.
- cMoveEnd and cMoveTurn: **unmodified**. An already active run still receives
  its normal end processing, so the hunter stops rather than freezing mid-run.
- Everything else: unmodified.

**On enable**

The hunter may be mid-run. Enabling must not strand it. The design requires
that the first end/tick after enabling lets `cMoveEnd` run normally; this is
the one behaviour the handoff flagged as unresolved and it must be verified
before acceptance.

**On disable**

Restore the original bytes and re-arm. A key held across the transition must be
released by the user or cleared, because the game latches edge state; the
design must not synthesise that release by writing input state.

## Lifecycle and safety

The installed SPL `Hook<T>` activates on construction and disables on dispose,
but the investigation established it is **not** a documented guarantee of safe
in-flight removal, stacked-hook ownership, or unload synchronisation. Therefore:

- the filter is **disabled by default** and reaches enable state only on an
  explicit, acknowledged owner state;
- it is **idempotent** — repeated enables cannot stack a second patch;
- disable/unload must restore the original bytes and must tolerate the filter
  never having been enabled;
- ownership should expire on stale guest health, using advancing
  session/sequence evidence rather than the presence of an old file;
- the resolved instance addresses must be re-resolved on scene load and player
  replacement; a stale address must disable the filter rather than act on
  recycled memory.

## Implementation constraint: the vtable is shared

The predicate is invoked as `call QWORD PTR [rax+0x30]` with `rax = [obj]`,
i.e. through the object's vtable. That vtable is **shared**: 72 distinct
objects carry the cMove vtable `0x1431c53e0` in a typical scene, covering every
entity, not just the local hunter.

Patching the vtable slot, or the shared predicate body, would therefore affect
all of them and is **not** an acceptable implementation. Scoping is only
achievable by comparing the **object pointer** at the call site.

This also removes any option of a static, precomputed patch:

- object addresses are allocated per scene load and change between sessions;
- the local cMove object must be resolved at runtime.

So the filter must be a runtime-resolved, object-pointer-compared code hook.
Any implementation that hardcodes an address, or patches shared code, is out of
scope by this section.

## Known risks

- **Other movement consumers.** `cMoveTurn` is left untouched deliberately.
  Turning may still occur under exclusive mode; this must be measured and
  reported rather than assumed absent.
- **Controller input** shares the magnitude path. Controller coverage remains
  explicitly unsupported until tested.
- **The callback exists and is not a pass-through.** It was analysed rather than
  assumed; see below. The predicate result is respected, but the callback is a
  large routine with its own side effects on failure.

## Predicate-vs-callback analysis (resolved: the design point stands)

The dispatch `0x14026ba00` calls the predicate and then consults a callback
table on the transition owner:

```text
0x14026ba7c  call  QWORD PTR [rax+0x30]          ; cMove predicate
0x14026ba7f  mov   edi,eax                       ; edi = predicate result
0x14026ba81  mov   rax,QWORD PTR [rbx+0x268]     ; callback COUNT
0x14026ba8b  je    0x14026bb05                   ; count == 0 -> return edi unchanged
```

With no callbacks the predicate result is returned untouched. The local hunter
**does** have one: `T = H+0xa00 = 0x5e233680` has `[T+0x268] = 1`, and its entry
at `T+0x270` holds object `0x5e2338f0` whose `+0x18` is `H`, tying it to this
hunter. So the callback path is live and its return value, not the predicate's,
becomes the dispatch result:

```text
0x14026bb2e  mov   DWORD PTR [rsp+0x38],edi   ; predicate result passed BY REFERENCE (r9)
0x14026bb60  call  QWORD PTR [rax+0x10]       ; callback wrapper 0x141193720
0x14026bb63  mov   edi,eax                    ; edi = CALLBACK RETURN VALUE
```

The wrapper `0x141193720` reloads `r9d` from that out-parameter, sets
`rcx = [cb+0x18] = H`, and tail-jumps to `0x141196260`. That callback begins:

```text
0x141196278  cmp   r9d,0xffffffff        ; the predicate's result
0x14119627c  jne   0x14119629f            ; != -1 -> continue to main logic
0x14119627e  mov   DWORD PTR [rcx+0x4ef8],0xffff
0x141196288  mov   eax,r9d               ; eax = -1
0x14119628b  mov   BYTE  PTR [rcx+0x5eff],0x0
0x14119629e  ret                         ; RETURNS -1
```

**The callback propagates the failure.** A predicate result of `-1` is passed
through unchanged, together with the game's own side effects for an ineligible
move (`H+0x4ef8 = 0xffff`, `H+0x5eff = 0`). The dispatch therefore returns
`-1`, which the selector treats as ineligible.

**Consequence.** Forcing the local cMove predicate to `-1` *does* suppress the
move. This is the desirable shape: the filter does not fabricate a state the
game would never see, it drives the engine down its own native "not eligible"
path. The interception point in this document stands.

**Correction.** An earlier checkpoint withdrew this point on the belief that a
registered callback overrides the predicate. That conclusion was premature: the
callback body had not been read at the time, and the multi-entry loop's
fall-through `0x14026bb03 xor edi,edi` was also misread as forcing zero when the
single-callback case branches to `0x14026bb26` before reaching it. Both errors
are recorded rather than edited away.

**Still unverified.** The callback's success path (`0x14119629f` onward, a large
routine through `0x1411966f3`) was not analysed; it is reached only when the
predicate already returned something other than `-1`, so it does not affect this
gate, but it may matter if a future design ever needs to *allow* movement.

## Acceptance plan

1. Baseline: normal movement and release, with no filter installed.
2. Enable mid-run with W held: hunter must stop; no stuck run.
3. Enable at rest: W produces no movement in MHW while Minecraft owns input.
4. Disable: movement resumes immediately, including after a held key.
5. Repeat toggles and duplicate enables: no stacked patch, no stuck state.
6. Scene reload and player replacement: filter re-resolves or disables cleanly.
7. Plugin unload while enabled: original bytes restored, no callback into
   unloaded code.
8. Menus and UI checked separately, and what remains usable recorded.

Each requires screenshots plus fresh capture evidence. Build success is not
rendering evidence.

## Status

Design only. No code, no hook, no build, no installation, no game launch.
Exclusive input remains unimplemented and unaccepted.