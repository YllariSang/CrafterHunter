# Exclusive input: reference investigation

2026-10-09. Status: investigation only; exclusive input is NOT implemented.
Rendering, pose cadence and installed game binaries are unchanged.

## Reference and history

Inspected MIT reference at commit
`d9cac4690682131140f736f9ec60302b877e258d` (copyright 2026 justbustin).
GitHub path history for both game.cpp and Overlay.java returned only that initial
publication commit; no earlier implementation/restoration history was exposed.
This document describes observed source, not a runtime test of the reference.
No reference implementation code was copied.

- [MHW README](https://github.com/justbustin/minecraft-crossover-bridge/blob/d9cac4690682131140f736f9ec60302b877e258d/monster-hunter-world/README.md)
- [Design](https://github.com/justbustin/minecraft-crossover-bridge/blob/d9cac4690682131140f736f9ec60302b877e258d/docs/how-it-works.md)
- [Host handle_switching](https://github.com/justbustin/minecraft-crossover-bridge/blob/d9cac4690682131140f736f9ec60302b877e258d/monster-hunter-world/mhw-bridge/src/game.cpp)
- [Guest Overlay](https://github.com/justbustin/minecraft-crossover-bridge/blob/d9cac4690682131140f736f9ec60302b877e258d/monster-hunter-world/mc-bridge/src/client/java/dev/mhwmc/bridge/client/Overlay.java)
- [Guest key bindings](https://github.com/justbustin/minecraft-crossover-bridge/blob/d9cac4690682131140f736f9ec60302b877e258d/monster-hunter-world/mc-bridge/src/client/java/dev/mhwmc/bridge/client/MhwBridgeClient.java)
- [DirectInput forwarding proxy](https://github.com/justbustin/minecraft-crossover-bridge/blob/d9cac4690682131140f736f9ec60302b877e258d/monster-hunter-world/mhw-bridge/src/proxy.cpp)

## Actual mechanism

Minecraft registers F8 as a Fabric key binding. switchToMhw is idempotent:
it sets mhwMode, opens the pause screen when appropriate, releases Minecraft's
mouse, hides its GLFW window and increments a shared-memory MHW-focus request.
The host sees a changed request counter and calls ShowWindow(SW_RESTORE),
SetForegroundWindow, SetActiveWindow and SetFocus. It logs whether foreground
focus actually succeeded, rather than treating the request as guaranteed.

Host F8 is GetAsyncKeyState(VK_F8), gated by the MHW foreground window and a
rising-edge latch. It increments the shared-memory Minecraft-switch counter.
The guest polls that counter, shows/focuses its GLFW window and dismisses its
pause screen if present. No native input filter is enabled/disabled in these
handoff functions, so restoration here is focus restoration, not undoing a
hunter-input patch.

While Minecraft owns focus, its transparent borderless floating window lies over
MHW. macOS NSWindow setIgnoresMouseEvents(false) prevents clicks falling through.
GLFW handles Minecraft's own mouse capture. The proxy forwards DirectInput8Create
to the real library; it is not itself an input-suppression device wrapper.

Menu access is achieved by handing focus back to MHW, not by keeping MHW UI
inputs selectively available while hunter inputs are filtered. The inspected
handoff has no controller polling/filtering or controller ownership handshake.
Do not infer that background MHW gamepad input is suppressed.

## Transferability and missing prerequisite

GLFW window calls and edge-triggered request/acknowledgment concepts can inform a
Linux implementation. Cocoa mouse-event policy and CrossOver foreground behavior
are platform-specific. Wine foreground requests and native Linux Minecraft focus
must not be assumed to work identically under Wayland. No desktop configuration
or focus automation is introduced here. DX11/DXVK rendering is unrelated to this
input boundary and requires no changes.

Installed SPL Core 1.0.0 XML documents IO.Input state queries, not hunter-only
suppression. Upstream Input.cs reads sMhKeyboard/sMhSteamController. Upstream
CoreModule.cpp invokes plugins before the original main update; this does not
establish whether native device polling occurs before or after the callback, or
which hunter consumer handles keyboard, mouse and controller actions.

Required prerequisite: verify the pinned build's hunter gameplay input consumer
or a supported suppression API, including its calling convention, identity,
thread/order and separation from menu/camera input. A focus handoff is not proof
of that boundary. No global input clearing or speculative hook is installed.

Once established, suppression must be scoped to the local hunter, disabled by
default, with a reachable disable path, idempotent transitions and restoration
on unload/error. Ownership should expire on stale guest health if its safety
depends on a live guest; a timeout must use advancing session/sequence evidence,
not merely the presence of an old file. Held-button transitions, player
replacement/loading, hook lifetime and release-before-reenable require tests.
Controller coverage must remain explicitly unsupported until tested.

## Acceptance plan (not yet executable)

1. Startup: normal MHW keyboard/mouse movement/actions; no suppression.
2. Enable: acknowledged owner state; Minecraft moves Steve, hunter stays still
   and does not attack/use items. Do not confuse lack of focus with suppression.
3. Disable: immediately regain MHW input, including after a held key/button.
4. Repeat toggles and duplicate commands: no stacked hooks/stuck buttons.
5. Test menus/UI separately; record which remain usable in each mode.
6. Repeat keyboard/mouse checks with controller only if support is implemented.
7. Disconnect guest/bridge, then unload plugin while enabled: verify restoration
   for each supported path and no callback into unloaded code.
8. Check player/skin/pose/orbit/occlusion against the accepted renderer. Measure
   frame times in the same scene/focus state before and after, not just Steve's
   intentionally 4 Hz animation cadence.

No implementation/lifecycle tests or live ownership acceptance are claimed.

## Pinned-build input trace — 2026-10-09, review required

Investigation baseline: `b421dc0`. No suppression, observer hook, installation,
game launch, input write, commit or push was performed. The already-running MHW
process (PID 110141) permitted read-only `/proc/110141/mem` access. Small loaded
code excerpts and analysis tools remain under `/tmp`, outside this repository;
no game code or binary data is included here. Addresses below are observations
on this build/session, **not signatures or authorization to hook them**.

### Identity and evidence provenance

Rechecked executable: 84,225,952 bytes, SHA-256
`c2ebbbd2c49f216d484e31a5219bed419eb1e5e7d206d02cba040a3ab79d90ea`.
Steam appmanifest reports build ID `15539686`; loader log and both address caches
report revision `421810`. Project records identify Ver. 15.23.00. The on-disk
`.text` remains opaque (entropy 8.00); offline instruction decoding is invalid.
Loaded code, rather than encrypted file bytes, supplied the observations below.

Installed SPL package is `SharpPluginLoader-2.0.0-linux.zip`. Its Core and native
loader bytes match the corresponding archive entries exactly:

| Component | SHA-256 |
| --- | --- |
| Installed SharpPluginLoader.Core.dll | `de764f893832e0e7b35e239ee20f3bcf14e1f8c9fd3e516a653a9d6ff20bb3c8` |
| Installed loader ucrtbase.dll | `bdc7be03efe2540fc4bddf24b8d7077baa82bd01c596fc086bf37c1f1d68a934` |
| Installed Default.bin | `4397f9c020e49c5921cc4f39948589327adf1d54ca4c1c94a77893b596992f08` |
| steam_api64.dll | `1db3fd414039d3e5815a5721925dd2e0a3a9f2549603c6cab7c49b84966a1af3` |
| Prefix D3D11.dll | `a1e70f1df91df181632a9b70c3d2eaa1ad0c8550cbbee07a8b3abaf6678f042a` |
| Prefix DXGI.dll | `533389eb68a0af2c7e9d047de4c66e52830c5c0772d06a08632ec11ecefb787d` |

Proton version file: `1790839227 experimental-11.0-20261001-x86_64`.
Process mappings include Wine X11 driver, user32, dinput8, xinput9_1_0,
xinput1_4 and the prefix D3D11/DXGI modules. Module presence alone does not prove
device use. Exact DXVK semantic version was not established; the hashes above
identify the installed files without guessing one. Package version 2.0.0 and
the project's Core NuGet reference 1.0.0 are different identifiers.

Installed managed Core was decompiled with scratch ILSpy 9.1.0.7988, outside the
game. Upstream was separately inspected at
`7be239303f90af7f7356a3695adc07cb6b0707ef`; its native source is corroboration,
not proof that this release was built from that commit.

### Observed input paths

```text
Keyboard:
  Wine user32 foreground/key APIs + DirectInput keyboard device
    -> Keyboard:WriteInput (loaded 0x1422E7AD0, already detoured by SPL)
    -> SPL renderer keyboard filter, after its Original call
    -> native keyboard update 0x1422E74F0 computes edge/repeat state
    -> shared keyboard state / SPL read APIs
    -> UNKNOWN action mapping and hunter/UI/camera consumers

Mouse:
  OS/device acquisition NOT traced
    -> sMhMouse virtual function 6 (installed SPL renderer hook)
    -> Original, then shared mouse-state filtering for ImGui
    -> UNKNOWN hunter/UI/camera consumers

Controller:
  Wine xinput9_1_0!XInputGetState
    -> game thunk 0x14273DCC9, slot 0x144C979E0
    -> native pad routines, including 0x1422A59D0
       [this routine also has a DirectInput Poll/Acquire/GetDeviceState branch]
    -> UNKNOWN relation to sMhSteamController aggregation/action mapping
    -> UNKNOWN hunter/UI/camera consumers
  SPL Input(Button) independently reads sMhSteamController shared button bits.
```

Keyboard evidence is stronger than a name in the address cache:

- The sole direct-call candidate to WriteInput is decoded at `0x1422E75E1`,
  within the unwind-described function `0x1422E74F0..0x1422E7821`.
- That caller copies On to Old, clears On, passes `keyboard + 0x138` as the
  second argument, calls WriteInput, then derives Trg/Rel/Chg. This establishes
  the writer's timing relative to **keyboard-state production**, not relative
  to the hunter consumer or plugin OnUpdate.
- Writer code uses device virtual slots 25/7/9 (Poll/Acquire/GetDeviceState in
  IDirectInputDevice8), requesting 256 bytes, then maps device keys through the
  table at `keyboard + 0x38` into output bits.
- Its resolved user32 slot `0x144C97678` points to the mapped export
  GetAsyncKeyState; `0x144C977A8` points to GetForegroundWindow. These names were
  matched against the installed Wine DLL export RVAs, not inferred from code.
- Installed SPL Input(Key) reads this shared state/table. It offers queries,
  not a local-hunter suppression API.

Controller evidence is independent: the loaded pointer at `0x144C979E0` equals
the installed xinput9_1_0 export XInputGetState (`0x6FFFF7B81620` this session).
The thunk has six direct-call candidates, within unwind-described functions
starting `0x14229FBC0`, `0x14229FDC0`, `0x1422A0650`, `0x1422A5735`,
`0x1422A59D0`, `0x1422A5E70`. Some are destruction/reset paths; they must not all
be called gameplay updates. The decoded routine at `0x1422A59D0` calls the
thunk and has a separate DirectInput branch requesting 0x110 bytes. A related
vtable's DTI getter identifies `sPad::Pad`. No connected-controller input,
Steam Input remapping, analog axes, or hunter-consumer behavior was tested.

### Installed SPL behavior and lifecycle

Installed Renderer.Initialize already installs two shared-input hooks:

- Keyboard: Original first, menu-key release latch, then clearing the supplied
  KeyboardState when ImGui WantCaptureKeyboard or NavActive is true.
- Mouse: sMhMouse vfunc 6; ImGui focus/hover checks around Original, then
  modifications to shared mouse wheel/button fields, including menu/dialogue
  fields. This is not a verified local-hunter-only gate.

These describe **existing loader behavior**; CrafterHunter did not clear or
write any of those fields. Forcing ImGui capture or copying those operations
would affect shared input and conflict with the requested selective scope.

Installed NativeInterface.OnUpdate invokes PluginManager.InvokeOnUpdate; that
iterates registered plugins under the contexts lock, without a catch around
each callback. Loaded Main:Update `0x141AF2B60` is detoured. Upstream CoreModule
calls plugin callbacks before Original; the exact installed native callback
ordering and full path from main update to device polling/hunter consumption
were not traced. No supported post-poll/pre-hunter callback was found.

Installed Hook<T> activates on creation, exposes Original/Enable/Disable, and
Dispose disables if enabled; its finalizer also attempts disable. This is not a
documented guarantee of safe in-flight detour removal, stacked-hook ownership,
or unload synchronization. Keyboard's existing detour makes stacking especially
unsafe without an explicit chaining contract.

Installed PluginManager.UnloadPlugin calls OnUnload then disposes the context.
Context disposal calls IPlugin.Dispose, which disposes IDisposable-valued plugin
fields, then unloads native/context resources. UnloadAllPlugins directly disposes
contexts; do not assume it invokes OnUnload. Future restoration must therefore
cover Dispose too and must not depend on a finalizer or callbacks after unload.
Watcher-driven reload and exceptions during cleanup also need explicit handling.
Installed Renderer.Shutdown destroys ImGui context; it does not explicitly
dispose its input hooks in that method. Do not repurpose loader-owned hooks.

### Ranked interception candidates

Rank is suitability for this task, not confidence that suppression is safe.

| Rank / candidate | Coverage and timing | Effects / confidence / risks |
| --- | --- | --- |
| 1. Local hunter action/movement consumer | Device coverage and consumption timing unknown | Correct semantic target, but **not identified**. Must distinguish human input from scripted movement/actions and separate UI/camera callers; no signature/ABI/cleanup evidence yet. Not implementable now. |
| 2. Existing keyboard writer / mouse update boundary | Keyboard production boundary verified; mouse Original-then-filter verified in installed SPL | High confidence as shared filters, low suitability for hunter-only suppression. Menu/camera effects not isolated, controller uncovered, already loader-owned. Native thread identity/in-flight lifetime still unmeasured; reversible hook API alone is insufficient. |
| 3. Native pad acquisition boundary | XInput call target and DirectInput alternative verified | Shared controller acquisition, not hunter-specific. Would risk UI/camera, connection state and held-button transitions. Steam-controller aggregation/analog coverage and thread/lifetime unverified; no production signature proposed. |
| 4. Plugin OnUpdate state modification | Callback exists, but post-poll/pre-consumer order unverified | Likely overwritten or observed too late; shared-memory clearing is forbidden. No supported scoped suppression method. Reject. |
| 5. Focus handoff / OS filtering | Application-level keyboard/mouse routing | Not hunter-only; background controller behavior unproven and Linux focus routing differs. No current implementation recommendation. |

### Missing prerequisite and recommendation

Do **not** implement a suppression hook yet. Available extension points establish
shared device input, not the required hunter-only boundary. This is not proof
that the game lacks such a boundary or that the goal is impossible.

Needed next evidence: follow reads of the shared keyboard/mouse/pad state into
the local hunter's semantic movement/action consumer; identify its exact-build
function and ABI, all relevant UI/camera/scripted callers, and the source of
analog input. Then obtain a bounded controlled trace of polling, mapping,
consumer execution and OnUpdate order/thread, including menus, loading/player
replacement and held inputs. Verify coexistence with SPL's existing detours and
safe in-flight cleanup before any interception. A passive hook cannot be safely
proposed as hunter telemetry until the consumer itself is identified.

Existing CrafterHunter logs cover camera/player position, bridge health,
calibration, rendering and terrain diagnostics, not input-consumer timing.
Installed loader logs registration of OnUpdate, not polling/consumer order.
No existing input trace can substitute for the missing evidence. No new runtime
instrumentation was added and no GUI automation was attempted.

Validation: `python3 tools/verify-host-build.py` PASS; archive/installed SHA-256
equality PASS; loaded-code and Wine export checks described above completed;
`git diff --check` required before handoff. No production build or gameplay test
is appropriate for documentation-only changes. No live input-suppression or
restoration acceptance is claimed. This report and MODLOG.md are left
**uncommitted for review**.

### Semantic-consumer follow-up — 2026-10-09

Read-only loaded-code inspection on the same fingerprinted build advances the
trace to mapped commands, **not to a verified hunter consumer**. Addresses below
are evidence locations, not proposed hook signatures. Windows x64 register
arguments are observed; complete function types and thread identity are unknown.

* Live `sMhKeyboard` instance slot `0x1451c53b0` has vtable
  `0x142f86d08`, verified against its DTI getter. Its update `0x140485d90`
  calls base keyboard update `0x1422e74f0`, then mapping helpers
  `0x140485470` and `0x1404848a0`. This establishes polling/edge production
  before derived mapping within this call, not global frame/callback order.
* `0x140485aa0` takes keyboard in RCX and mapped index in EDX, returning
  an AL boolean from indexed bits at `+0x13e0`, subject to controller-context
  gates. `0x140485af0` similarly reads `+0x1400`; the precise down/edge
  meaning has not been proven from all writers. Do not confuse these with
  raw key codes or assign action names to their indices.
* Shared query `0x1418bcc60` takes a command index in ECX, then ORs
  controller `0x141b16640`, keyboard `0x140485aa0`, and mouse
  `0x1404890b0` results. Its mouse singleton slot `0x1451c53f0` resolves
  to the DTI-verified `sMhMouse` vtable `0x142f86e48`. The query has no
  hunter argument and at least 29 confirmed direct-call sites. This is a
  shared command boundary, not a safe hunter-only interception point.
  Direct-call searches are nonexhaustive (indirect calls/inlining remain).
* Related query `0x1418bcd20` ORs controller `0x141b167a0`, keyboard
  `0x140485af0`, and mouse `0x1404891d0`. Additional shared variants
  exist at `0x1418bccb0`, `0x1418bcd70`, and `0x1418bcdc0`; their full
  semantics and consumer classification remain unestablished.
* Live `sMhSteamController` slot `0x1451c4558` is DTI/vtable verified
  (`0x142f87138`). Derived update `0x141b168c0` calls base pad update
  `0x1422a5040`, selected-pad processing `0x1418d7720`, then further
  mapping/filter helpers. Query `0x141b16640` takes RCX=this,
  EDX=command index, R8D=context index, testing mapped bits at `+0xda0`
  through a command-mask table at `+0x1000` (context strides observed).
  `0x141b167a0` tests the corresponding `+0xda8` bits. These are shared
  aggregation structures; no controller-to-hunter end-to-end proof exists.
* Scalar query `0x141b15e70` takes the same three register arguments and
  returns XMM0. Index 1 first checks keyboard/mouse mapped indices 14/15,
  otherwise reads a context/index-selected float at `+0xcfc` with strides
  `0x8d0`/`0xb8`. Thus even this controller-named layer can incorporate
  keyboard/mouse; the axis/action meaning is not established.

Some shared-query callers also access the master-player accessor and sound
names `snd_tab_2_simple` / `snd_cancel_light`. These are contextual clues only,
not proof of UI ownership or hunter consumption. They cannot justify a hook.
The live `sPlayer` manager slot `0x14500eca0` had zero master identity; an
entry matching zero failed the accessor's required object-flag test. No valid
local hunter or its consumer was inferred from that entry.

Unwind records include split/chained ranges: a short range is not necessarily
a complete function. Raw windows were labeled accordingly. No guessed entry,
ABI, vtable slot or detour was introduced. Inspection used read-only process
memory and bounded disassembly; scratch tools stayed outside the repository.

Ranking update: a **hunter-local caller of mapped queries** remains the preferred
but unidentified target. Hooking the shared mapped queries ranks below it and
is rejected: device coverage is broader, but UI/camera/scripted scope is unknown.
Device writers and acquisition remain unsuitable for the same reasons already
recorded. No small suppression implementation is justified yet.

Single next action: statically identify and analyze the local `uPlayer` update
path and its mapped-query call sites on this exact loaded build, proving object
identity and separating human-command consumption from camera/UI/scripted
processing before considering passive timing instrumentation. Afterwards, runtime
order/thread, SPL hook coexistence and in-flight unload safety still need proof.
Future ownership must separately handle duplicate toggles, held-button release,
Dispose/unload/reload, guest disconnect, exceptions and restoration; none is
implemented or accepted here. Controller support remains unsupported.

### uPlayer dispatch and local-object gate — 2026-10-09

Same loaded-image session; read-only inspection, no game launch or hooks.
Class identity now has structural evidence: the `uPlayer` name at
`0x143258780` is referenced by DTI `0x14502df90`; getter `0x142038d80`
returns that DTI. Vtable `0x1434a6d18` contains this getter at byte offset
`0x20`. Constructor `0x142031b40`, called from allocation routine
`0x142031860`, installs that same vtable after its base constructor. This
identifies the class, not the active local hunter or a scheduler update entry.

Verified dispatch edges (vtable byte offsets observed, not guessed hook slots):

* `0x141fbd010` conditionally dispatches `this` through `+0x250`, resolving
  to `0x142037ac0` in the identified uPlayer vtable. That implementation
  calls `0x141f5f340` first, then `0x1419a5610`; if nonnull, it calls the
  returned object's virtual `+0xa8`. It subsequently processes another
  object and flags. Skipping this entire routine would skip more than input.
* `0x141fbd220` conditionally dispatches `+0x248` to `0x142038330`.
  That routine calls `0x1419a5610` and, if nonnull, tail-dispatches the
  returned object's virtual `+0xb0`.
* The broader `0x141fbccd0` path contains collision/position updates,
  component callbacks, and conditional virtual dispatches including
  `+0x238` (`0x1420388d0`), `+0x110` (`0x141f79830`) and `+0x118`
  (`0x142047a70`). These are not established human-input gates. Split
  unwind ranges required continuation decoding from verified instruction
  boundaries; padding after return/tail-jump is not treated as a callee.

The strongest new local-scope evidence is `0x1419a5610`: RCX is the supplied
player, it calls `0x141b42da0` with the sPlayer singleton and that player,
and returns null unless the check succeeds. `0x141b42da0` explicitly compares
the supplied pointer against `0x141b42010` (FindMasterPlayer), returning AL
equality. On success `0x1419a5610` returns `[player+0x14f8]` in RAX.
This proves a local-player-gated object accessor, **not** that the object's
`+0xa8/+0xb0` methods consume human gameplay input. Its concrete type,
virtual targets, device/action coverage, camera/UI/scripted responsibilities
and allocation/replacement lifetime are not yet established. Neither accessor
should be hooked as a substitute for that missing semantic evidence.

| Boundary | Classification supported now | Suppression suitability |
| --- | --- | --- |
| uPlayer `+0x250` / `0x142037ac0` | General player processing; action category unknown | Reject: maintenance and indirect work are mixed |
| uPlayer `+0x248` / `0x142038330` | Local-object dispatch; category unknown | Not verified: callee identity/semantics missing |
| `0x1419a5610` / `0x141b42da0` | Local-player identity/access gate | Not an input consumer |
| Local object's `+0xa8/+0xb0` | Unknown | Best next trace target, not a hook candidate |
| Shared mapped query `0x1418bcc60` and previously listed callers | Shared input queries; individual movement/action/camera/UI/AI categories unresolved | Reject as a global suppression boundary |

No verified edge yet joins these local-object dispatches to `0x1418bcc60`.
One versus multiple movement/attack/item consumers therefore remains unknown.
Only the conditional intra-function order above is established; scheduler
entry, polling-relative frame order, thread context and SPL callback order
require additional evidence. The observed Windows x64 register usage does not
establish complete callable signatures or safe detour/unload behavior.

Next action: resolve the concrete type and virtual targets of the local-gated
`+0x14f8` object (from its allocation/assignment path, or a read-only snapshot
with a valid master player), then trace those specific two methods into mapped
queries. This is narrower than repeating whole-image caller searches. Stop
before instrumentation until their identity and semantics are established.
No safe suppression implementation is justified. Validation: bounded static
dispatch/accessor decoding; `git diff --check`. No executable behavior or
installed files changed; documentation remains uncommitted for review.

### Resolved local object — 2026-10-10

Read-only inspection of user-started PID 14133 resolves the object as
`cHumanControllerPlMaster`: player `0x69e30080` has the previously identified
uPlayer vtable; `[player+0x14f8] = 0x5e1ac1c0`, whose vtable is
`0x14322bbb0`. Its DTI getter `0x1411971d0` returns DTI `0x144f9f158`,
with that concrete name. These heap addresses are session observations only.

Correction to prior invalid-player conclusion: the previous reader used the
wrong entry-relative offsets. Re-decoded FindMasterPlayer compares `[r8-8]`
against `[manager+0xae40]`, then returns `[r8+8]` subject to flag bits `0x0e`,
where r8 starts at manager+0x50 and advances by 0x740. Zero identity alone is
not an invalidity test. Using those actual offsets returns the valid player
above (flags 0x08). The earlier claim of an invalid local entry is superseded.

Resolved dispatches, with observed Windows x64 RCX=this (full ABI not proven):

| Slot | Target | Proven work / classification |
| --- | --- | --- |
| +0xa8 | 0x1411a4570 | Calls 0x1411a71e0 first; then state-gated mapped queries. Mixed processing; movement/attack/item/UI meaning unresolved. |
| +0xb0 | 0x1411a2660 | Processes pending fields +0x4fbc/+0x4fc0, iterates other players, checks squared positional distance, invokes operations on those players, clears pending fields. Cross-player processing/maintenance; precise action meaning unknown. |

The +0xa8 target calls mapped variant `0x1418bcdc0` with command 0x15,
then `0x1418bcc60` with indices 4 and 5 at `0x1411a45fb` and
`0x1411a4609`. Query results gate tail-call `0x141ae5700` with a global
object, not directly a movement-vector write. Jump-table branches and state
gates precede this block. No action labels are inferred from command indices.
The initial helper also handles a referenced object and its position before
these queries; skipping the whole method is not established input-only.

The +0xb0 body through return `0x1411a2854` has no direct mapped-query
call. It excludes its associated player from loops, calls `0x141f75920`
on qualifying nearby players and `0x141f60560` in another pending branch.
Indirect/transitive input consumption is not ruled out. This method is unsafe
to skip speculatively: it has pending-state cleanup and cross-player effects.

Thus a verified local-dispatch-to-mapped-query path now exists, but a safe
movement/action suppression boundary still does not. Class name and local
accessor are insufficient to prove every caller is local or every operation
is human-input-only. Thread/order, held-input behavior, object replacement,
SPL chaining and in-flight cleanup remain unverified. No instrumentation added.
Next focused action: establish what `0x141ae5700` does with its global object
and these command results, to classify the proven +0xa8 command branch before
considering any interception. No code, installed files or input state changed.

### Classified command branch — 2026-10-10

Read-only PID 14133 trace resolves the global receiver of `0x141ae5700`:
slot `0x1451c4640` points to `sMhGUI`, verified by vtable `0x1433f9190`
and DTI getter `0x141ade710`. Its `+0x13d60` receiver has vtable
`0x143465270`, getter `0x141d8e9c0`, and concrete DTI name
`uGUICommonSelectWindow`. Classification is supported by request handling,
not the names alone.

`0x141ae5700` gates on external checks (`0x1405dc320`, `0x1418bc710`),
GUI bytes `+0x146c0/+0x146c4`, and another GUI flag `+0x146e6`. It builds
a stack request containing position values, a value obtained through
`0x1418bbdb0(0xb,0xd6)`, and a copied callback. These numeric arguments
are not assigned user-visible meanings. It forwards the request via
`0x141d8ee40` only if the GUI's `+0x13d60` object passes native validity
checks. It performs callback copy/destruction work and returns at
`0x141ae595f`; no hunter movement vector or direct attack/item execution
was observed in this body. The mapped results select this control-flow
branch; they are not passed as action parameters.

`0x141d8ee40` copies the request through `0x140554600` into a bounded
four-entry structure at receiver `+0x29f0`, updates ring fields
`+0x2c30/+0x2c38`, and sets receiver `+0x2298` bit `0x80000000`.
The copy helper's initial field copies corroborate request storage. This
is concrete queued UI work; queue consumption and visible text were not
runtime-observed.

Callback table `0x1433f94b0` resolves its operation target to
`0x141ad4f70`. That target checks `[R8] == 0`; only that result tail-calls
`0x141b463f0` with sPlayer singleton, EDX=0x2a, R8D=-1.
`0x141b463f0` reads pending field `+0xaff4`; under its guard it writes
EDX to `+0xaff4` and R8D to `+0xb000`. For EDX=0x2a the guard permits
those writes. This is deferred player-manager state, not evidence of a
particular hunter action. The callback body itself does not inspect a
local-hunter pointer. The eventual interpretation of pending code 0x2a
is still unknown; do not call it an attack, item or menu transition.

Proven path: local-player-gated controller +0xa8 -> mapped-command decision
-> sMhGUI request construction -> common selection-window queue -> stored
callback -> conditional sPlayer pending-state write. Callback invocation
timing/ABI and queue lifetime still require proof; inspection did not execute it.
Observed register roles are not complete hook prototypes.

A bounded E8/E9 search, with instruction confirmation from unwind-described
ranges, found only the known tail-call at `0x1411a461e`. This is incomplete
caller evidence: indirect pointers, split ranges, inlining and other builds
are not covered. The observed caller is local-gated, but the GUI queue and
player manager are shared subsystems. No global local-hunter-only claim follows.

No safe suppression candidate emerged: intercepting this branch would suppress
a UI/deferred operation, not established movement/attack/item inputs; skipping
the entire +0xa8 still skips other work. Highest-value next static trace:
follow reads/consumption of sPlayer pending fields +0xaff4/+0xb000 to resolve
code 0x2a and its effects. Runtime observation is not yet necessary merely to
classify that consumer; dynamic order and safe hook lifetime remain later gates.
Validation: bounded entry/branch/callback/queue decoding and caller confirmation;
`git diff --check`. No hooks, memory/input writes, installed changes or commits.

### Pending code 0x2a consumption — 2026-10-10

Read-only PID 14133, bounded displacement-reference search plus instruction
confirmation. Search results are not exhaustive: split unwind ranges can omit
accesses (including the already proven small setter), indirect addressing and
alternate encodings remain uncovered. Immediate constants 0xb000 in unrelated
functions were rejected as field references.

Verified field reader/consumer: `0x141b42fd0`, present at byte +0x30 of
the previously identified sPlayer vtable `0x1433fe5b0`, preserves RCX=this
in RSI. At `0x141b435e5` it reads +0xaff4, compares against -1, and for
a pending value performs this transfer:

* old +0xafec -> +0xaff0; old +0xaff8 -> +0xaffc;
* pending +0xb000 -> active +0xaff8, then pending +0xb000 = -1;
* pending +0xaff4 -> active +0xafec, then pending +0xaff4 = -1;
* word +0xb004 = 1 (transition indication).

Other branches clear transition byte +0xb004, and when +0xb005 requests it,
clear that byte and reset active +0xafec/+0xaff8 to -1. Constructor path
`0x141b3ff10` initializes these fields to -1. These are generic state
promotion/reset operations, not movement or attack execution. Their surrounding
update also processes player entries; skipping it would affect unrelated work.

Downstream dispatcher `0x141299b50` loads the verified sPlayer singleton
slot 0x14500eca0, checks +0xb004, then reads +0xafec. It range-checks
code+1 and dispatches through a byte map at 0x14129dda0 and RVA table
0x14129dd3c. For code 0x2a, index 43 maps to case 24, whose target
0x14129dd07 is register restoration/return. Thus **this dispatcher does
no case-specific action work for 0x2a**. Its receiver type has not been
established; no subsystem label is inferred from the address alone.

Verified additional active-state readers `0x140ef5de7` and `0x140ef6081`
load the same sPlayer singleton and compare +0xafec directly with 0x2a,
branching to 0x140ef5f97 / 0x140ef61a1 instead of their normal processing.
Their surrounding code accesses the local controller through 0x1419a5610.
This establishes state-gated early-outs, not the precise movement/attack/item
meaning of those routines. Other active-state matches remain unclassified;
no all-reader/all-caller completeness is claimed. The known GUI callback
establishes one human-input origin, not exclusive provenance of this mechanism.

Conclusion: 0x2a is proven to become active shared player-manager state and
gate downstream work. Its user-visible purpose and all consequences remain
unknown; there is no proven dedicated hunter action or safe suppression hook.
Do not inject this code or hook the generic state promotion/dispatcher.
Highest-value next action: resolve the concrete receiver/type and processing
of 0x140ef5db0 and its 0x2a early-out, to identify exactly which behavior this
state gates. Static classification can still advance; runtime timing/lifecycle
remain unverified. `git diff --check` passed; no executable/installed changes,
hooks, input writes, launches, commits or pushes.

### Classification of 0x140ef5db0 early-out — 2026-10-10

Read-only inspection of the existing PID 14133 loaded image, same pinned-build
provenance as above; no instrumentation. Entry 0x140ef5db0 has its own prologue,
preserves RCX in RBX, and has two verified returns: success at 0x140ef5f96
(EAX=0) and failure at 0x140ef5fa6 (EAX=-1). The comparison at 0x140ef5de7
branches at 0x140ef5dee to the failure epilogue 0x140ef5f97. This conclusion
comes from decoding the entry and all intervening branches, not equating an
unwind fragment with a complete function.

Receiver identity is supported by construction AND metadata: metadata
0x144f7c168 names nHmTransition::cPopGimmickButton and references factory
0x140ef37c0. The factory allocates 0xb0 bytes and installs vtable 0x1431c0c18;
placement initializer 0x140ef3890 installs the same table. Its +0x20 getter
0x140ef5880 returns that metadata; +0x30 is 0x140ef5db0. Initializers set
command selectors +0x9c/+0xa0/+0xa4/+0xa8 to 2/3/4/5. These numbers are not
assigned gameplay meanings. Eight table references to the processing function
were observed, including tables with metadata getters naming PopAquariumButton,
PopWallInteractiveButton, PopVillageWarpButton, PopSpaButton, PopFootSpaButton,
PopChairButton and YokuryuWarp. This is shared transition-family processing,
not evidence that every invocation is a PopGimmickButton instance.

Before the state gate, [self+0x28] is passed to 0x1419a5610. With a nonnull
local controller, 0x1411937d0 registers self in the controller's pointer array
when its cached value differs from player+0x8930, and returns that value into
self+0x30. Only then is sPlayer+0xafec compared with 0x2a. A null controller
bypasses this comparison and continues normally: local gating of this branch
does NOT prove local-only scope of the whole routine.

Normal processing checks eligibility via 0x140ef6670, associated-object
[self+0x10] fields +0x7d20, +0x7b78 and +0x7ac8, and selects one of the four
command indices. It queries [self+0x20] through 0x14122fdb0, 0x14122fe60 or
0x14122fd50. The latter reads a command bit at +0x780 while setting bookkeeping
bits at +0x860/+0x8a0; these are not pure read-only queries. On a true result,
0x141f604b0 sets associated-object bits indexed 0x15d; the routine sets bit 22
in [self+0x28]+0x12624/+0x12638. If associated-object+0x7a58 equals 0x34,
it also sets bits indexed 0x9b/0x9c through that helper. The helper updates
+0x8938/+0x8968 bit arrays and has an additional flag side effect for 0x9b.
No movement-vector update, attack execution, item execution or camera/UI
operation is established by these flag effects alone.

Precisely, the 0x2a early-out retains controller registration/cache maintenance,
skips all subsequent eligibility checks, selector/query bookkeeping and success
flag writes, and returns -1 instead of allowing success 0. It is a
state-dependent command/transition check, not a verified general input blocker.
The interpretation that this is an interaction-related transition predicate is
consistent with metadata and boolean/flag data flow, but the actual transition
chosen and user-visible action remain unproven. No safe suppression candidate.

Best next action: trace the caller consuming this +0x30 virtual result (0/-1)
into transition selection and action execution, to identify what these flags
actually cause. Static evidence still cannot establish runtime thread/order or
reversible hook safety. Validation: bounded branch, factory/getter and callee
decoding; git diff --check. No executable or installed-file changes, hooks,
memory/input writes, launches, commits or pushes.

### Virtual result consumption and destination selection — 2026-10-10

Same PID 14133, read-only loaded-build inspection. A dispatch candidate was
confirmed from instruction boundaries, table indexing and actual objects:
0x14026ba00 loads a transition pointer from receiver+0xc0+16*group,
indexed by a record's second integer, sets transition byte +8 bit 0 from
record+8, then calls vtable+0x30 at 0x14026ba7c. The live local controller's
embedded table owner at 0x5e1acbc0 (controller 0x5e1ac1c0 +0xa00) contains
39 objects whose +0x30 target is 0x140ef5db0, including index 464 at
0x5e1c26f0 with constructed PopGimmickButton vtable 0x1431c0c18. Example
objects' +0x10/+0x28 are the verified local uPlayer 0x69e30080, and +0x20
is controller+0x10. This proves table compatibility and ownership, not that
any particular record executed during observation.

The generic table-owner structure itself has no proven independent concrete
DTI type. Its construction is established: cHumanControllerPad constructor
0x141189b00 installs vtable 0x14322b910 (getter 0x14118d500 -> metadata
0x144f9f0c8), then initializes its embedded +0xa00 structure through
0x14026aaf0. Verified metadata ancestry of the actual owner is
cHumanControllerPlMaster -> cHumanControllerPl -> cHumanControllerPad.
Do not relabel this structure as a named FSM class without further evidence.

Result flow: 0x14026ba00 saves EAX in EDI, then may send that result plus
record/state pointers through optional callback objects at owner+0x270
(64-byte entries). With no callbacks it returns the original result; the
last callback can replace it. No guarantee of unmodified results is claimed.
0x14026b830 calls it at 0x14026b8e9. A returned -1 skips this record and
continues scanning, NOT necessarily all action selection. Other integers
are compared with record+0xc, with -2 accepted as a wildcard. Matching records
with both destination integers at +0x10/+0x14 valid are selected; special
records can require additional checks through 0x14026bb80. Thus 0 is a
table-matched outcome, not an unconditional execute command.

Concrete recorded example: the owner's group-1 table has 777 state entries;
state-table entries 0/3/4/5 and others include transition record
(1,464,0,0,1,495). This selects PopGimmickButton index 464, tests outcome 0,
and supplies destination pair (1,495). Another record using index 484 has
destination (-10,-10), demonstrating that the consumer handles other
transition instances and special outcomes. These are read-only snapshots,
not event traces or semantic labels for those constants.

Selection consumer 0x14026b590 calls 0x14026b830, rejects a null selection,
invokes selected transitions' +0x38 methods through 0x14026b380, resolves
the destination pair (optionally through owner+0x38 callback), and calls
0x140269ce0 with the state object and pair. On a true result it calls
0x14026b160, which applies transition byte flags and invokes +0x28 methods.
The parallel consumer 0x14026b670 also selects records, calls 0x140269ce0,
and records the selected transition pointer at table-owner+0x150.
0x14118e4a0 supplies controller+0xa00 and [controller+0xd90]+0x61c8 to
0x14026c0f0 -> 0x14026b670. These are shared table-processing routines,
not PopGimmickButton-only or proven local-only routines.

Execution boundary caveat: 0x140269ce0 already starts with an absolute
indirect detour to 0x6fff9a300060. Its hook provenance/original trampoline was
not established here. The still-visible native continuation validates a
destination entry, snapshots prior state, writes pair +0xac/+0xb0 and mode
+0xa8=1 when idle, otherwise writes pending pair +0xbc/+0xc0, and returns
true. This is evidence of intended state-selection effects, NOT proof that
the live detour always performs them. In the local state object's group-1
table, entry 495 has vtable 0x143357058; its getter 0x1417497a0 returns
metadata 0x144fe1fb8 naming nHmAction::cActEnvBashKick. No kick, interaction,
movement or attack execution is inferred from that name alone; trace stops
at the verified selection/state-update boundary.

No direct consumption of the prior +0x12624/+0x12638 bit-22 writes, or
associated +0x8938/+0x8968 bits, was established by this result trace.
Those remain separate effects. Closest verified command-consuming predicate
is still PopGimmickButton's selector -> 0x14122fdb0/0x14122fe60/0x14122fd50
on controller+0x10. Its connection to physical devices and the earlier shared
0x1418bcc60 mapped-query path, coverage of movement/attacks/items, and runtime
order remain unproven. This advances transition/action selection, not a safe
human-input-only suppression boundary.

Coverage: one loaded-text search for disp8 vtable+0x30 call encodings, filtered
by integer-result usage and confirmed against unwind-described instruction
ranges; then bounded direct-reference searches for the identified consumers.
Other encodings, split-range omissions, indirect callers and all runtime
callbacks are unresolved. No all-caller claim. Best next action: establish
the existing 0x140269ce0 detour's provenance and original-call behavior before
following how a selected destination pair becomes action execution.
git diff --check passed; documentation only, no hooks/instrumentation,
memory/input writes, installed changes, launches, commits or pushes.

### Existing DoAction detour provenance and native entry — 2026-10-10

Read-only PID 14133. Installed Core hash remains
de764f893832e0e7b35e239ee20f3bcf14e1f8c9fd3e516a653a9d6ff20bb3c8.
Scratch ILSpy 9.1.0.7988 inspected that installed DLL, not the NuGet assembly.
Its ActionController.Initialize creates a Hook<DoActionDelegate> at
AddressRepository.Get("ActionController:DoAction"); the installed cache maps
that key to 0x140269ce0. NativeInterface.Initialize schedules that initializer.
Hook<T>'s constructor uses ReloadedHooks.CreateHook(...).Activate(); Original
returns the underlying IHook.OriginalFunction. This is loader-owned, not a
CrafterHunter hook. No reusable ABI/signature or safe chaining contract follows.

Loaded entry bytes ff24255800a302 jump through pointer slot 0x2a30058 to
0x6fff9a300060 -> 0x6fff9a3a37cc -> CLR delegate-marshalling body
0x6fff9aa2d810. Its delegate handle 0x951d68 references object 0x14a470810;
the delegate's method precode resolves to body 0x6fff9aa2d8c0. That body's
owner-field read +0x100, entity callback loop, MainPlayer comparison via +0x61c8,
local-player callback loop and two Original delegate calls match the installed
ActionController.DoActionHookFunc. Ownership is supported by installation,
cache, decoded wrapper and actual delegate chain, not anonymous-code ranges.

Original-call behavior on normal completion: OnEntityAction callbacks run when
Owner is nonnull; OnPlayerAction callbacks additionally run when the receiver
matches MainPlayer.ActionController. Both receive ActionInfo by reference.
Both local/nonlocal branches then call Original with the same receiver and
possibly callback-modified pair, returning its Boolean result. There is no
normal suppression branch in this wrapper. Exceptions before Original can
interrupt execution: no local catch/finally guarantees an original call.
The current loader log explicitly says CrafterHunter does not implement either
action callback; this does not prove the absence of every other observer or
future runtime change.

Loaded OriginalFunction is also corroborated independently: IHook object
0x14a470868 (method table matching the wrapper's getter dispatch) has delegate
0x14a515a60 at +8; getter body 0x6fff9aa24420 returns that field. Delegate
native target is 0xfffc0060. Its trampoline executes displaced instructions
movsxd r8,[rdx+4]; mov r9,rcx; then jumps to 0x140269ce7. Thus R8 is the
sign-extended destination ID and R9 retains the native receiver before the
visible implementation reads the destination set through RDX. Managed delegate
invocation code calls its native target and normalizes the return to Boolean.
No trampoline was invoked by the investigation. The library's internal
allocation/relocation bookkeeping was not reconstructed; these observations
establish this installed hook's original pointer, not a general hook safety
guarantee.

For record destination (1,495), selector 0x14026b590 supplies state receiver
and pair pointer to this wrapper. Its local receiver 0x69e36248 is verified
by vtable 0x142f11cf0 -> getter 0x14026a4d0 -> DTI 0x144c98be0 naming
cActionController, and is local uPlayer+0x61c8. Native continuation validates
set/ID and a nonnull action-table entry. If mode +0xa8 is zero it stores the
pair at +0xac/+0xb0 and mode 1; otherwise it queues +0xbc/+0xc0. A true
DoAction result does not therefore prove entry execution.

Nearest verified native entry consumer: 0x14026a2d0 resolves the current
+0xac/+0xb0 table entry, requires nonnull entry AND mode +0xa8==1, calls
0x14026a660 (prior-action processing), then 0x14026a1e0 and sets mode 2.
0x14026a1e0 runs optional callbacks, resets action fields +8/+0x18 and
controller bookkeeping, then tail-dispatches the action's vtable+0x30.
For the previously verified table entry 495, that virtual target is
0x140eef2f0. This establishes a conditional static entry path to the object
whose metadata names nHmAction::cActEnvBashKick, not proof that (1,495) was
requested, became current, or executed during this session. Callback mutation,
queued-state promotion and scheduling remain relevant; no runtime event trace
or user-visible action semantics were established. Stop at this verified
action-entry dispatch rather than assigning meaning from its name.

Conclusion: a shared action-request notification hook and native action-entry
path are now explained, but neither distinguishes physical hunter input from
scripted/UI/other requests. Do not suppress DoAction globally or nominate it
as a hunter-input boundary. Best next investigation: trace producers of the
controller+0x10 command bits read by 0x14122fd50, to establish human-input
versus other command provenance before this transition predicate. Missing
evidence is that provenance and selective action coverage, not detour ownership.
Validation: installed assembly hash/decompilation, loaded delegate/trampoline
chain and bounded native consumer decoding; git diff --check. Documentation
only; no game writes, hooks, instrumentation, launches, installation changes,
commits or pushes. Scratch tools installed only under /tmp/ch-input-ilspy.

### Command-bit owner and derived producers (2026-10-10)

Read-only loaded-code inspection in the same pinned image/PID 14133 resolves
transition+0x20 to 0x5e1ac1d0 (local controller+0x10). Its vtable
0x143241a80, getter 0x14122ff10 and DTI 0x144fa0e00 identify
cPlayerCommandController; constructor 0x14122e8c0 installs that table.
These are session/build observations, not signatures or callable prototypes.

The confirmed call at 0x140ef5f2a passes this receiver and the selector from
transition+0x9c/+0xa0/+0xa4/+0xa8, initialized to 2/3/4/5. 0x14122fd50
sets the selected bit in +0x860 and +0x8a0, then returns the bit from
+0x780+(index>>5)*4. Thus these four selectors query the first dword's
masks 0x4/0x8/0x10/0x20; no action names are assigned. The first two
arrays are bookkeeping, not the returned command value. 0x14122fd90
queries +0x780 without those bookkeeping writes.

Verified producer chain: 0x14118e350 passes controller+0x10 at its call
0x14118e3db to 0x141230bb0, under manager/player-state gates. The latter
preserves its receiver in R15, expands bits at receiver+0x18 into byte +8
of 0x28-byte records beginning +0x48 (store +0x50+index*0x28), then
updates those records via 0x1411edc90 at 0x141231007. That helper maintains
previous/current bytes, edge-related bytes and countdown/timer state;
the command output is therefore derived/cached state, not a raw device array.
The +0x780 array is cleared at 0x14123187f/886/88d before reconstruction.

For the four selected outputs, the bounded reconstruction shows:

| Output bit | Verified writer/condition |
| --- | --- |
| 2 | 0x141232854 -> 0x141232fc0 with source record index 0; +0x52/+0x6b gates helpers 0x141230990/0x1412308d0 |
| 3 | 0x141232884 writes the bit according to receiver+0x68/+0x6a (record 0 derived bytes) |
| 4 | 0x141232892 -> 0x141232fc0 with source record index 1; corresponding bytes +0x7a/+0x93 |
| 5 | 0x1412328a3 -> 0x1412330b0 with source record index 1; sets/clears according to +0x90/+0x92 |

0x1412308d0 positively writes +0x780 at 0x1412308f8; 0x141230990
prepares related +0x7a0/+0x7c0/+0x7e0/+0x800/+0x820/+0x840 state,
not itself +0x780. The alternate branch 0x1412328aa..8d9 clears all four
through 0x14122f460, which also clears five related arrays. Later call
0x141232ecc conditionally applies 0x14122f4f0: derived clearing of +0x780
using bookkeeping and other state arrays. Finally +0x860 is copied to
+0x880 and cleared at 0x141232ed1..eff. Constructor initialization and
these verified clears are not a claim of exhaustive writer coverage.

Upstream limit: writes that populate this exact receiver's +0x18 source
bitfield have not been resolved. Nearby calls to shared mapped query
0x1418bcc60 (commands 7..10 here) feed a separate +0x978 object; they do
not prove the source of bits 2..5. No end-to-end keyboard, mouse or pad
provenance, physical-versus-scripted discriminator, or runtime scheduling
order is established. Direct-call search for 0x141230bb0 yielded the
verified call above; indirect callers and other +0x780 writers remain open.

Consumer scope remains the previously verified shared interaction-transition
predicate family and generic transition selector, not a complete movement,
attack or item consumer. Local ownership of this instance does not prove
all instances/callers are local or physical-input-only. No suppression
boundary is justified; hook chaining, in-flight teardown and action coverage
also remain unverified. Best next action: trace only assignments to the
cPlayerCommandController +0x18 input bitfield in its owning controller's
preparation path, including any non-device producers, before considering hooks.
Validation: bounded instruction decoding from entry/unwind-fragment boundaries,
vtable/DTI and caller receiver data flow; git diff --check. Documentation only;
no executable, installed binary, input state, hook or instrumentation changes.

### Source-bitfield initialization and ownership limit (2026-10-10)

Same pinned loaded image, read-only static instruction inspection. Constructor
0x141189b00 builds cPlayerCommandController at owning controller+0x10;
0x14122e8c0 then calls 0x141371cd0 with command-controller+8. That helper
installs vtable 0x143250a58, whose +0x20 getter 0x141371f00 returns DTI
0x144fadda0 naming cVirtualPad. At virtual-pad+8 it installs vtable
0x143250a30; getter 0x141371ef0 returns DTI 0x1451d5ec8 naming
bitset_prop<46>. The confirmed initialization write 0x141371ce7 zeros a
qword at virtual-pad+0x10, exactly command-controller+0x18. It initializes
both dwords used for the 46 source bits; later virtual-pad float writes are
different fields, not bit producers. Standalone virtual-pad factory/constructor
zero writes are not evidence of additional writers to this embedded instance.

Preparation relationship: master update 0x1411a2860 reaches
0x141190ae0 at 0x1411a2eb8; the latter conditionally calls 0x141190b90,
then calls 0x14118e350 at 0x141190afb with the same controller receiver.
0x14118e350 passes its embedded command-controller to 0x141230bb0 as
previously recorded. Base forwarding entry 0x141189230 also jumps to
0x14118e350. This is static order, not proof of device polling order,
thread context or all virtual callers. Initialization path 0x14118dca0
passes the command controller and player to 0x140ce47b0; that function
maintains the player reference at command-controller+0x30, not source bits.
The following call 0x14023d430 is a return-only stub, not input preparation.

Source/output dependency now narrowed: source bit 0 becomes record 0 byte
+8 (command-controller+0x50); source bit 1 becomes record 1 byte +8
(+0x78). Record maintenance 0x1411edc90 computes +0x0a from current +8
and previous +9, and maintains +0x20/+0x22/+0x23 using current input and
timer/countdown state. Output 2 uses record 0 +0x0a/+0x23; output 3 uses
record 0 +0x20/+0x22. Output 4 uses record 1 +0x0a/+0x23; output 5
uses record 1 +0x20/+0x22. These are dependencies, not identity mapping
or named actions. Outputs also encounter the previously recorded player-state
gates, alternate clears and post-filtering. Record history/countdowns can
affect results beyond a single instantaneous source-bit value.

Writer inventory remains incomplete: the exact-instance constructor zero is
verified, but no non-initialization assignment to this source field was
established. Bounded unwind-fragment decoding covered controller preparation
regions 0x141189000..0x1411a9000 and command reconstruction
0x14122e000..0x141234000 for scalar +0x18/+0x28 accesses, then relevant
overlapping vector stores and selected setup callees. Unrelated stack,
player/action-object and table-record accesses were excluded by receiver
data flow. This is not a whole-image alias analysis: pointer-adjusted helper
writes, indirect calls, leaf methods absent from unwind tables, bulk copies,
and producers outside these regions remain uncovered. No non-device writer
has been demonstrated either; lack of a found writer proves neither device
exclusivity nor absence of scripted producers.

Answers: physical keyboard/mouse/controller -> source -> outputs 2..5 is
NOT verified end to end; no physical-versus-scripted/UI/camera discriminator
is established; no candidate isolates movement/attacks/items. Earliest resolved
shared representation is the embedded cVirtualPad bitset, not a verified
earliest producer. Best next action: perform a bounded static alias/xref trace
of pointers to this embedded cVirtualPad/bitset (command-controller+8/+0x10),
including helper and bulk-copy destinations, to locate the first nonzero
assignment and its callers. Do not repeat displacement-only searches or install
a hook to discover ownership. Documentation only; git diff --check; no game
launch, hooks, instrumentation, memory writes, installation, commit or push.

### Embedded bitset property alias and setter limit (2026-10-10)

Material additional alias evidence from the same pinned loaded code:
bitset_prop<46> virtual property-registration method 0x141371e10 stores its
receiver in a descriptor at +0x18 (instruction 0x141371e4e), reader
0x140268290 at descriptor+0x20, count getter 0x1412dca90 at +0x28,
and setter 0x140268500 at +0x30. The count getter returns 46.
The descriptor is copied into allocated storage at 0x141371e92..eb0 and
linked through the registration destination's +8. These bulk copies copy
the descriptor, NOT the command source bytes. Calling registration on the
embedded bitset would retain command-controller+0x10 as this receiver;
invocation of registration/setter on the exact instance is not established.

Setter 0x140268500 has instruction-confirmed non-initialization write
capability: RCX is the bitset receiver, R8D supplies the bit index, DL supplies
the boolean. It computes word=index>>5 and bit=index&31, loads
[RCX+8+word*4], and uses BTS or BTR before storing the dword at
0x14026851e or 0x140268527 respectively. On this embedded receiver these
addresses would be command-controller+0x18+word*4. Indices 0 and 1 would
set/clear only source bits 0 and 1, preserving other bits in that dword.
No index bound check exists in this leaf; the registered count does not itself
prove callers enforce bounds. Reader 0x140268290 uses the same word/bit
calculation and storage offset. This is a generic indexed bitset accessor,
not evidence of a physical-device producer or local-hunter-only scope.

A narrowly targeted E8/E9 reference search for this setter over loaded .text
(candidate instructions checked against unwind-fragment decoding where present)
found no direct call/jump byte candidates. This does not cover indirect calls,
pointer-to-member adapters or other writers. The verified descriptor pointer
is an indirect route, but its invocation, receiver binding, index/value
producers, and scheduling remain unresolved. No actual runtime write to the
exact controller instance is proven; neither a complete producer inventory
nor multiple-producer absence/presence can be claimed. Existing source-to-record
dependencies remain conditional on such a write, not new provenance evidence.

Stop at this indirect-dispatch boundary. The smallest next static evidence is
an instruction-supported invocation of descriptor+0x30 that obtains RCX from
descriptor+0x18, with the embedded registration/descriptor identity and R8D/DL
origins traced. That would distinguish an actual producer from merely an
available generic property setter. No safe hunter-only suppression boundary
is established. Documentation only; git diff --check; no executable/installed
changes, hooks, instrumentation, game-memory writes, launch, commit or push.

### Indexed property dispatch resolved; local binding still missing (2026-10-10)

Same pinned loaded-code evidence, read-only. The descriptor's +0 is the
string pointer 0x142e4fe14 ("Bit"), not a vtable. Registration
0x141371e10 initializes +8 to zero, +0x10 to 0x000a0003, +0x18 to
its bitset receiver, +0x20 to reader 0x140268290, +0x28 to count getter
0x1412dca90, +0x30 to setter 0x140268500, +0x38 to return-only
0x14023d430, and +0x40 to zero. Descriptor allocation 0x142171fe0 uses
0x58-byte records; registration copies fields and links +0x48/+0x50,
with the newest descriptor in the destination list's +8. These operations
retain the receiver alias and accessor pointers, without copying bitset data.

The previously missing generic invocation is now instruction-supported:

1. 0x1421711f0 preserves incoming object RCX in RBX, property-name RDX
   in RDI, value pointer R8 in R14, and index R9D in ESI. It initializes
   a temporary list through 0x142171920, then calls the object's virtual
   +0x18 at 0x142171226, passing that list in RDX.
2. At 0x142171236, lookup 0x142171cb0 receives the list, numeric type 3,
   and the incoming property name. Lookup follows +0x50 links, matches
   descriptor+0x10's low 12 bits, and compares a selected name string at
   descriptor+0 or +8 according to global selection state. A successful
   lookup returns that descriptor. Finding "Bit" remains conditional on
   both the object's registration and name-selection path.
3. 0x142171240 writes the incoming index to descriptor+0x40;
   0x142171246 loads the byte at the incoming value pointer into EDX;
   0x14217124a calls 0x14218f3f0 with the descriptor in RCX.
4. For flags 0x000a0003, dispatcher 0x14218f3f0 takes both tested flag
   branches (bits 19 and 17). At 0x14218f405 it loads RCX from
   descriptor+0x18; at 0x14218f40b it loads R8D from +0x40; at
   0x14218f40f it tail-jumps through +0x30. DL survives as the value.
   With the descriptor built by 0x141371e10 this target is 0x140268500.
5. The previously verified setter sets the indexed bit for nonzero DL and
   clears it for zero DL, storing one dword at receiver+8+4*(index>>5).
   The list cleanup at 0x142171254 recycles descriptor records; it is not
   a reset of the receiver's bitset or a restoration of the assigned value.

This is a conditional static chain from the known registration to a real
setter dispatch, not an observed call on the exact local controller. If the
incoming object is command-controller+0x10 with the verified bitset vtable,
virtual +0x18 selects 0x141371e10 and the setter receiver is precisely that
embedded bitset. No resolved caller supplies that object alias and name.

One verified direct caller narrows index/value provenance without resolving
the object: 0x1411fdfe0 preserves its incoming RDX as the target object in
RDI. At 0x1411fe07b it calls 0x1421711f0 with RCX=that object,
RDX=its own receiver+0x30 (name address), R8=a stack byte, and R9D=0.
The stack byte comes from AL returned by 0x142170df0 at 0x1411fe052,
or is replaced by the caller receiver's +0x58 byte when updated float
state +0x54 is positive. Its float update uses incoming XMM2 and fields
+0x50/+0x54. Thus index 0 is proven at this generic assignment call;
the value is state-selected. The target object's concrete identity, name
contents, upstream caller and any physical-device source are unresolved.
This does not prove a scripted producer of the exact command bitset either.

Independent corroboration: 0x14218a4c0 takes destination/source descriptors,
checks matching low-12-bit types and a destination setter, and dispatches
type 3 through 0x14218a634. It reads a source accessor or byte storage and
calls 0x14218f3f0 (e.g. 0x14218a658). Its verified entry and chained
unwind fragments establish a generic property-value transfer path, not a
device update. Destination+0x40 supplies the indexed setter argument there.

Coverage: targeted indirect-call encoding candidates were filtered by receiver
+0x18 loads and instruction boundaries; only the relevant property consumers
were followed. Targeted direct references to 0x1421711f0 include the caller
above and four sites in 0x1426bbcxx, all supplying index zero. Direct references
to 0x14218f3f0 also include other generic consumers; indirect callers and
unwindless candidates are not an exhaustive inventory. No exact-instance
invocation, source-bit-1 invocation, thread/order guarantee or physical-input
provenance has been established. Existing source 0/1 -> records -> outputs
2/3 and 4/5 dependencies remain conditional, with their existing history/gates.

Stop at the missing object/name binding. Smallest next evidence: resolve the
incoming target-object RDX and receiver+0x30 name at 0x1411fdfe0's caller,
enough to prove or exclude command-controller+0x10 and "Bit". That single
alias would connect or eliminate the now-verified index-zero property path;
generic dispatcher availability alone does not establish its use by gameplay.
No safe hunter-only suppression boundary is supported. Documentation only;
git diff --check; no launch, hooks, instrumentation, memory/input writes,
installed/executable changes, commit or push.

### Property-operation construction and target-array dispatch (2026-10-10)

Targeted static continuation resolves another caller edge, but not the local
command-bitset alias. No direct E8/E9 caller of 0x1411fdfe0 was found.
Loaded read-only data contains its pointer at 0x143240460. Construction
0x1411fca80 proves this is slot +8 of the table at 0x143240458:
0x1411fcc9e installs that table in a newly allocated 0x60-byte operation.
No concrete DTI type is established for this small operation.

Property-name construction is explicit. 0x1411fca80 saves incoming R8 in
R15 and calls 0x1411fe0b0 with RCX=operation+0x2c and RDX=R15 at
0x1411fccb1. The helper stores a length, caps it to 31, copies bytes to
destination+4, and appends zero. Thus operation+0x30 is an inline,
caller-supplied name, not a fixed "Bit" binding. A second string at
operation+0x0c is copied from incoming RDX. Incoming R9 points to the
byte saved at operation+0x58 (0x1411fccdf..cee). The operation is stored
in one of its owner's three slots at +0xf8/+0x100/+0x108 by 0x1411fccf8.

Verified construction caller 0x14120004e supplies the property string from
[RBX+0x10]+8 when non-null, otherwise an empty-string fallback. Its set/clear
byte is AL returned by the source object's virtual +0x70 at 0x141200019.
The first string comes from RSI+0x50. These are dynamic source fields;
neither their concrete contents nor a connection to physical-device queries
is established. Setter-compatible data does not prove the name is "Bit".

Verified indirect consumer: entry 0x141200b60 and its chained unwind
fragments include 0x141200fe9. The loop takes an operation from the same
three-slot list, compares its first string (+0x0c) with another record's
+0x168 string, then calls operation-vtable+8. At that call RCX is the
operation, RDX=[component+0xb0][R14], and XMM2 is loaded from the
component's virtual +0x80 result at +0x68. An operation installed by the
construction above therefore dispatches to 0x1411fdfe0. If it returns true,
the caller frees the operation and clears its slot; this is operation cleanup,
not restoration of an assigned target property.

Supporting owner-layout evidence: constructor 0x1411fcdd0 installs table
0x14323ff68 and initializes +0xb0 and the +0xf8 slot region. Table +0x20
is 0x1411feeb0, which returns DTI 0x1451c3b38 naming cpComponent.
This establishes a component construction/layout compatible with the dispatch;
it does not establish the concrete runtime identity of every receiver or
target-array element. The target passed to the property operation is an
array element, not this component itself. Nearby processing of those elements
includes virtual calls and state writes; it does not demonstrate a virtual-pad
or bitset target. Initial zeroing/freeing of the array pointer does not resolve
the producer of its elements.

The remaining exact edge is now [component+0xb0][R14] -> the target RDX
of 0x1411fdfe0, together with the operation's copied property-name bytes.
No assignment connecting that element to command-controller+0x10 has been
proved. Consequently the index-zero call at 0x1411fe07b and its state-selected
byte are established, but a write to the exact source storage is not.
Terminology: command-controller+8 is cVirtualPad, +0x10 is its bitset
receiver, +0x18 is source storage; the enclosing human controller embeds
the command controller at +0x10. These bases must not be conflated.

Classification: the verified path creates, matches, updates and retires named
property operations. Device/script/UI category and exact target type remain
unknown. No physical provenance, instance alias, or hunter-only suppression
boundary is supported. Best next evidence is the assignment populating the
selected component+0xb0 array element, paired with the corresponding source
record/string used at 0x14120004e. Stop at that unresolved binding rather
than following unrelated actions. Documentation only; git diff --check;
no launch, instrumentation, hooks, memory writes, installed/executable changes,
commit or push.

### Target-array writer and resource-backed property source (2026-10-10)

Read-only pinned static continuation finds an actual target-array element
writer: 0x1411fe55f stores RDI into [component+0xb0][R12D] during
0x1411fe120. This narrows the prior missing assignment, but the selected
factory and runtime record remain unresolved.

The array is an embedded container at component+0x98. Call 0x1411fe1a0
passes that adjusted receiver to 0x14029e7e0. Its storage is container+0x18
(component+0xb0); count/capacity are container+8/+0xc. Growth helper
0x140249b70 allocates pointer storage, clears it, copies existing pointers
at 0x140249bcd, and installs the storage pointer at 0x140249bf9.
0x14029e828 initializes new elements to zero. These are array storage
operations, not command-bitset writes.

Element provenance in the verified population path:

- component+0x90 supplies the data object. 0x1419c0ee0 selects a group
  from its +0xc0 pointer array, with a bounds check against +0xb0.
- 0x141316de0 reads the group's child count at +0x40;
  0x1419f8dc0 selects a child from group+0x50.
- At 0x1411fe2f4, the selected child's virtual +0x128 returns another
  object. At 0x1411fe303, that returned object's virtual +8 produces
  the target pointer, saved in RDI at 0x1411fe312.
- RDI is passed through initialization/registration work without adjustment
  before 0x1411fe55f stores it. R12D advances once per child, flattening
  the group/child traversal. The update at 0x141200b60 traverses groups
  and children in the corresponding order and uses its flattened index
  to fetch the target passed at 0x141200fe9.

This is a concrete factory-produced pointer path; the selected child's vtable,
its +0x128 return identity, and the concrete +8 factory target are not
resolved. Neither type nor exact instance is proved to be cVirtualPad or
bitset_prop<46>. Pointer-array allocation does not allocate the target objects
themselves. No exhaustive claim is made about other population paths.

Property source is now traced to a specific data traversal, rather than just
an incoming string. Entry 0x1411ffd90 obtains records from
[component+0x90]+0xe0, with count at that data object's +0xd0. It matches
a record's +0x70 value and +0x2c string against its incoming selection,
then iterates the record's +0x20 value-pointer array (count +0x10).
For a value entry whose +0x20 numeric type is 3, the verified jump table
selects 0x141200013. That branch obtains AL from the value entry's virtual
+0x70 and property text from [valueEntry+0x10]+8 (empty fallback if null).
Call 0x14120004e copies that text into operation+0x30 and the byte into
operation+0x58; record+0x50 supplies operation+0x0c's target selector.
There is no literal "Bit" in this construction path.

The two traversals are associated at execution by comparing operation+0x0c
with the selected child's +0x168 string at 0x141200f9c..fc5. On a match,
the corresponding target-array element goes to operation virtual +8.
This proves the selection mechanism, not that a particular operation and
target were present or matched in a concrete runtime component. String
uniqueness, literal property bytes, and selected value-entry implementation
are not established. Index zero and the subsequent state-selected byte retain
their prior evidence; no further trace of unrelated actions is warranted.

Classification remains data-driven named-property mutation with a factory
target; physical keyboard/mouse/controller provenance is absent. No exact
local cPlayerCommandController/cVirtualPad instance relationship or safe
hunter-only suppression point is supported. Smallest next evidence is one
resolved component data record: its selected child vtable/+0x128 factory
chain and matching value-entry property string. Static constructor/resource
assignment evidence for that record could resolve both; without it, continuing
generic accessor traces cannot establish the runtime binding. No observation
or instrumentation was added. Documentation only; git diff --check; no game
launch, memory writes, installed/executable changes, commit or push.

### Named-record construction and light-factory scope (2026-10-10)

Bounded static continuation on the same pinned loaded image resolves a concrete
resource-loading path, not the identity of a selected live operation. Only code
and static metadata were inspected; no runtime call or object pairing was observed.

0x1412002f0 replaces component+0x90: after old-target cleanup and reference
handling, 0x14120033a stores its incoming RDX object there. One verified caller,
0x141c86060, obtains a component through 0x141bf1e90 and saves it at receiver
+0x208e0. It requests resource `em\em120\00\light\em120_00` using DTI
0x14500a950, then passes the returned pointer to the component setter at
0x141c860b0. This is one supported caller, not proof that it supplied the
previously discussed operation in a live session.

DTI 0x14500a950 names rLch; its +8 factory 0x1419f7870 allocates 0xe8
bytes and reaches constructor 0x1419f7fd0. That constructor installs vtable
0x1433e7858 (its +0x20 getter returns the same DTI), initializes the group
container at +0xa8 (count +0xb0, storage +0xc0), and initializes the named-record
container at +0xc8 (count +0xd0, storage +0xe0). Thus these are separate
collections in one resource, not child/value fields within one record.

The resource's virtual +0x50 is 0x1419f9270. Its chained unwind fragments
continue through 0x1419f9489; they are not separate complete functions.
After stream/header checks, it sizes the group array, allocates 0x80-byte
groups, installs vtable 0x1433e77f8, and stores them at resource+0xc0
(0x1419f93f8). That vtable's DTI getter identifies rLch::Chr at
0x14500a8e0. Group virtual +0x28, 0x1419f8dd0, reads a child count and
per-child numeric discriminator from the stream, calls 0x1418a3c10, stores
the result at group+0x50 (0x1419f8f40), and calls child virtual +0x120
to populate it.

0x1418a3c10 is a bounded 1..12 switch (table 0x1418a3eb8; bytes after
0x1418a3eb6 are table data, not instructions). Following its constructors,
installed vtables, DTI getters and virtual +0x128 resolves these alternatives:

| Stream discriminator | Constructed child DTI name | +0x128 target | Returned DTI name / +8 factory |
| --- | --- | --- | --- |
| 1 | InfiniteLight | 0x1418a3b70 | uLlkInfiniteLight / 0x141f85330 |
| 2 | PointLight | 0x1418a3bb0 | uLlkPointLight / 0x141f875a0 |
| 3 | SpotLight | 0x1418a3bc0 | uLlkSpotLight / 0x141f8ae80 |
| 4 | HemiSphereLight | 0x1418a3b60 | uLlkHemiSphereLight / 0x141f84840 |
| 5 | ActorPointLight | 0x1418a3b10 | uLlkActorPointLight / 0x141f81460 |
| 6 | ActorSpotLight | 0x1418a3b20 | uLlkActorSpotLight / 0x141f81590 |
| 7 | ObjectLight | 0x1418a3ba0 | uLlkObjectLight / 0x141f85df0 |
| 8 | ActorInfiniteLight | 0x1418a3b00 | uLlkActorInfiniteLight / 0x141f816c0 |
| 9 | NullAngleSpotLight | 0x1418a3b90 | uLlkNullAngleSpotLight / 0x141f81800 |
| 10 | FollowCameraPointLight | 0x1418a3b40 | uLlkFollowCameraPointLight / 0x141f83720 |
| 11 | FollowCameraSpotLight | 0x1418a3b50 | uLlkFollowCameraSpotLight / 0x141f83850 |
| 12 | FollowCameraInfiniteLight | 0x1418a3b30 | uLlkFollowCameraInfiniteLight / 0x141f83980 |

These are instruction-linked factories, not classifications from nearby names.
For example, child vtable 0x1433b2398 has +0x128 = 0x1418a3b70,
which returns DTI 0x14502c6c0. Its factory 0x141f85330 requests a new
0x7b0-byte allocation and tail-calls constructor 0x141f85480; it does not
fetch an embedded command-controller pointer. The selected discriminator is
not known. The table establishes this loader's alternatives, not an exhaustive
inventory of later child replacement or other component population paths.

For named records, 0x1419f9457 passes resource+0xc8 to 0x1419f7df0.
That helper reads a count and per-record type ID. The explicit fast path
compares against DTI 0x14500a988's ID, allocates 0x80 bytes and installs
vtable 0x1433e7828: its DTI getter identifies rLch::OverrideParamHolder.
Other nonzero IDs go through 0x14216ee50 and the returned DTI's +8 factory;
the selected record need not take the fast path. Both paths call record
virtual +0x28, then append the pointer to resource+0xe0 at 0x1419f7f81
or 0x1419f7f99. No selected serialized ID was observed.

The fast-path record loader 0x1419f8fc0 populates its selection string
+0x2c at 0x1419f908a and target selector +0x50 at 0x1419f914a from
separate stream strings (via 0x142175a10 and the verified string-copy helper).
It reads +0x70 from the stream at 0x1419f90a6. Its value array is resized
at 0x1419f91bf; each stream discriminator feeds 0x141a32400, whose result
is stored at record+0x20 at 0x1419f91e7 before value virtual +0x28 loads it.

Value factory discriminator 3 selects 0x141a324bf, constructs vtable
0x1433edd50, and initializes value+0x40 to zero. Its DTI getter returns
0x14500c378, rUnitResource::BoolUnitProperty. Loader 0x141a32fa0 reads
a stream byte, normalizes it with SETNE, and writes value+0x40 at
0x141a32fe4; getter +0x70 = 0x141a31fd0 reads that byte. The base loader
0x141a33630 passes value+0x10 to string helper 0x1419af7f0 at
0x141a33647. That helper reads stream text and replaces the referenced
string. This supplies the later [value+0x10]+8 property-text source.
Importantly, value+0x20 is separately read from the stream at 0x141a3365c;
the operation switch's numeric type 3 does not by itself prove that the
factory discriminator was also 3. This byte-producer chain is conditional
on the BoolUnitProperty construction, not a claimed selected live instance.

Child selector population is likewise resource-backed in the traced
InfiniteLight case: its +0x120 loader 0x1418a4210 calls 0x1418a4500,
which reads stream text and copies it through child+0x164 at 0x1418a45ca,
placing text at child+0x168. The prior string comparison pairs a holder's
selector with that child; construction does not install a direct pointer from
the holder to the child. No literal selector/property strings or successful
match in a specific resource record have been established.

Conclusion: the strongest proven construction path is resource-driven light
targets plus named property overrides. It is not evidence of physical-device
input. No exact alias to local cPlayerCommandController+8, no runtime writer
to its source bitset, and no safe hunter-only suppression boundary is proved.
The factory alternatives above materially weaken this particular generic
property call as a lead to the local bitset, without ruling out unrelated users
of the generic setter. Stop here: the smallest next evidence is the exact
selected resource/record's serialized child discriminator, matching selector
and value-entry type/property bytes. Inspect that concrete record rather than
continuing generic downstream accessor searches. Documentation only; no new
runtime observation, launch, hooks, memory writes or installed changes;
git diff --check; no commit or push.

### Return to the exact embedded source: alias escape and writer limit (2026-10-10)

The resource/light branch is retired as an input-provenance lead unless a
concrete reference connects it to the local embedded bitset. This continuation
uses only the same pinned loaded code/static metadata. Previously recorded
heap identities were not refreshed; no new runtime invocation is claimed.

A material additional alias is established in master setup +0x28,
0x1411a11c0. RDI preserves the incoming human-controller receiver H.
At 0x1411a13f0, LEA produces H+0x10, the embedded command controller C;
0x1411a13fb places it in the fifth argument slot. Call 0x1411a1403 enters
0x1412a8a90 with RCX=H+0x1410. After five pushes and SUB RSP,0x70,
the fifth argument is at [RSP+0xc0]: 0x1412a8b51 reads it and
0x1412a8b60 stores it at the helper receiver+0x1ec0 (H+0x32d0).
At 0x1412a8c59..c64 the same pointer is loaded and copied into each
constructed subordinate record's +0x40. There is no pointer adjustment to C
in those stores. This proves an instance-preserving command-controller alias
conditional on setup being called with H; it does not prove any subordinate
record writes C+0x18 or establish every consumer of that stored pointer.
The subordinate records' concrete types and full alias-use inventory remain
unresolved and were not expanded into unrelated action traces.

The helper's REP STOSQ operations at 0x1412a8ae1 and 0x1412a8b07
clear its own +0x960 and +0xd68 arrays (0x400 bytes each), not C+0x18.
No bulk-copy source/destination to the embedded source follows from those
instructions. The earlier master-update escape at 0x1411a37c8 passes
C in RDX to 0x141782460. That helper calls 0x14122fd50 and
0x14122ff50, querying derived commands/other state; the first query's
bookkeeping writes are not source-bit writes.

Additional bounded coverage targeted the omissions of the previous scalar
displacement search: pointer adjustments in the known master update and
command reconstruction, their chained unwind fragments, and small accessor
entries between the known owner/command-controller methods. The leaf-accessor
check used INT3-delimited entries in 0x141188000..0x1411a2860 and
0x14122e000..0x141230bb0; it is not an inventory of unaligned entries,
all tail calls, indirect targets or other modules. No returned embedded-pad
pointer/writer was resolved there. The verified cVirtualPad table ends at
+0x20; its +0x18 entry is the return-only 0x14023d430, not a hidden
update method. Master calls 0x1411a7730 and 0x1411a7410 preceding
0x141190ae0 were checked with their chained unwind fragments; their
observed pointer paths do not establish a source assignment. These checks
do not prove that transitive/indirect callees cannot reach the source.

Result: no instruction-supported non-initialization writer with destination
aliasing the exact C+0x18 qword has been established. Generic bitset setter
0x140268500 remains write-capable, but its invocation with receiver C+0x10
and a source-bit-0/1 index/value remains missing. Likewise, a store through
the newly retained C alias to C+0x18 is unproved. No physical keyboard,
mouse or controller edge, multiple-producer conclusion, or hunter-only
suppression boundary follows. Constructor zeroing remains initialization only.

Stopping point and different evidence method: in a future separately scoped
runtime investigation, resolve/validate the current local player/controller
again and place a debugger data-write watchpoint on exactly C+0x18 after
construction. Capture one hit's instruction, effective destination, relevant
registers, call stack and old/new bits. This would identify a concrete writer
without another global address search; its input provenance would still need
to be traced from that hit. Such a watchpoint is instrumentation and was NOT
installed under this task's static-only constraints. Documentation only;
git diff --check; no launch, hooks, instrumentation, process-memory writes,
installed/executable changes, commit or push.
