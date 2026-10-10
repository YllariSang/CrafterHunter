# CrafterHunter mod journal

## 2026-10-11 — Exclusive-input filter installed and live-tested

Built the exclusive-input suppression filter (native/mhw-renderer),
installed it (SHA-256 981858aec9780b0114098e0c69df39f68e1a7392c63961e5f52f2e22d4598cde),
and live-tested the acceptance matrix against pinned build 421810.
Fixed resolver non-determinism (ad54597): the owner keeps several cMove
slots for one player and uses one at a time, so all local slots are now
covered (one shared private vtable copy) and the resolver emits every
slot, sorted.

Acceptance (docs/exclusive-input-design.md): cases 1 (baseline), 3
(enable at rest, W held -> 0.94 units/6s, frozen) and 4 (disable ->
resumes) PASS. Case 2 (enable mid-run, W held) FAILS: the hunter keeps
moving at 195 units/s and the trampoline stays silent. Mechanism proven
live: the vtable +0x30 predicate (0x141934220) gates the TRANSITION
into a move state (consulted every frame at rest, ~85/s) but is not
consulted during a move, so the filter prevents STARTING movement, not
continuing it. An active run ends only via cMoveEnd (on key release),
after which re-press is blocked. Matches the observed behaviour:
movement is not stopped until W is released and re-pressed.

Filter is disarmed; nothing is patched at rest. Open: case 2 (stop an
in-progress run - needs cMoveEnd invoked on arm or the move-state update
gated) and cases 5-8. Full handoff in docs/exclusive-input-handoff.md.

## 2026-10-10 — End-of-day input-investigation handoff

Stopped investigation at the user's request and recorded the next bounded goal
in docs/exclusive-input-investigation.md. Save the accumulated investigation in
incremental documentation commits and push the existing CrafterHunter origin;
no production code, installed artifacts, game assets or scratch disassembly/logs
are included. Earlier "no commit/push" entries describe their investigation turns,
not this subsequently authorized documentation handoff.

Next: resolve positive activating writers of exact local H+0x4af8/+0x4afc
from the verified 0x1411a8750 maintenance path; identify a legitimate nonphysical
movement fixture, then compare it with physical movement/held-button release
at local predicates, including C+0x9e3. Revalidate build and fresh instances
before any separately authorized runtime observation. Do not synthesize state.
The exact writer, conditional keyboard chain and native run capture are progress,
not a safe full hunter-only boundary. Exclusive mode remains unimplemented;
attacks/items, controller, UI/camera isolation and restoration/lifecycle are open.

Documentation scope/whitespace checks only for this handoff; no new game capture,
hooks, input/memory writes, build/install, rendering or bridge changes.

## 2026-10-10 — Manual W reaches the exact local movement transition and native action

Continued the user-authorized offline investigation with universal-modder
reverse-engineering, bounded hardware observations and instruction-level checks.
Pinned verifier passed. Local direction read P+0x1510 led to camera-associated
processing, not an isolated locomotion hook. Local LStick magnitude observation
instead identified exact cMove, cMoveTurn and cMoveEnd transition instances;
initializer, live player/controller links, vtables and DTI agree. Stop/end uses
an inverted threshold and timer, while cMove has other state gates/side effects.

After a timed attempt without a qualifying request, a user-confirmed repeat W
hold captured record (1,0x93,0,0,1,4), accepted SPL-wrapped request with pending
(1,4), actual local nHmAction::cActRun entry and native P+0xe214 write 1.0 ->
0.75 on LWP 14178. Thus native execution is established, not only action metadata
or queued selection. Subsequent release snapshot was magnitude zero/current idle;
this is not an exclusive-mode restoration test. Native entry used 0x14026a820's
pending-to-current loop through 0x14026a1e0. Separate bounded aggregate observation
identified exact copy writers and both main/job-thread scalar reads. Static trace
links keyboard mapped bit 29 -> keyboard Y scalar -> aggregate overwrite ->
local LStick magnitude -> cMove. This is a conditional instruction-supported
keyboard path, not a single-event dynamic device-provenance trace or controller
acceptance. Pad, mouse (other pair), state filters and nonphysical overrides remain.

No safe full hunter-only suppression boundary yet. Whole queries, preparation,
transition methods and action dispatchers remain unsuitable for blanket bypass.
Next gate: distinguish physical movement from a valid nonphysical case at the
local predicates while preserving held-button release/end behavior; fixture and
state producers are missing. Attack/item/UI/camera/controller and hook lifecycle
coverage are not accepted. No suppression or production implementation.

Only report/journal edits; preserved existing changes. Verified pinned identity,
exact objects, instruction/unwind/immediate-caller edges and bounded manual native
execution; git diff --check. Every hardware observation detached/removed probes;
no target-memory writes, software breakpoints, hooks, simulated controls, game
launch/restart, build/install, rendering/bridge change, commit or push.

A final bounded read/access check found maintenance of H+0x4af8/+0x4afc
in 0x1411a8750, immediately called by master update. Positive branches restore
near-zero player direction, decrement timers, test command/magnitude state,
and expire/reset; their positive setters were not observed (both values zero
in the 24-stop window). This strengthens the stateful-processing counterexample,
not a scripted-input classification or safe-hook claim.

## 2026-10-10 — Scalar pair consumers, position override and shared camera-associated work

Continued the exact local writer trace using universal-modder reverse-engineering
and bounded static loaded-code analysis; no new debugger or input stimulus.
Mapped the four aggregate queries to H+0x30..0x3c and exact embedded
cPlayerCommandLStick/RStick instances. Constructor, live vtables and DTI getters
agree; verified leaf processors calculate magnitude/normalized state, with a
mapped-command alternative for RStick. First-pair reconstruction produces
direction vectors copied into exact local player+0x1510/+0x7880. Timed
position-derived override clears H+0x9f3 = C+0x9e3, selecting the direct-vector
branch instead of camera-relative processing. Its timer/position producers and
action-selector meanings remain unknown.

The instance-preserving H+0x1410 helper retains the same controller, player
and verified uMhCamera. Consumer 0x1412ab4a0 reads BOTH scalar pairs into
its +0x1e60 accumulator; connected spatial calculations use that accumulator
and camera state. Therefore neither raw pair is established as movement-only.
Local direction-state writes do not yet identify a locomotion/attack/item
executor or safe hunter-only suppression point. Controller provenance, UI
scope, scheduler/SPL order and hook lifecycle remain unproved. Next recommended
evidence is a separately authorized bounded read observation of local P+0x1510
to identify its consequential downstream consumer; not performed here.

Only investigation documentation changed; preserved prior edits. Exact-instance,
constructor/DTI, instruction-boundary and chained-unwind checks; final assertions
and git diff --check. No production code/build/install, game launch/restart,
debugger attachment, hooks, memory writes, GUI automation, commit or push.

## 2026-10-10 — Exact local source writer captured with a hardware watchpoint

User authorized the next bounded runtime investigation and confirmed an offline,
loaded scene. Revalidated the pinned build and current local player/controller,
then captured one hardware-watchpoint change at exact C+0x18. Writer
0x1411a9cc7 stores through H+0x28; observed source changed 0 -> 0x20000000
on LWP 14178. Its loop copies 46 indexed results from 0x141b164c0 using
BTS/BTR; immediate master-update caller is supported by prologue/stack and
call-site evidence, not GDB's unreliable PE backtrace. Bounded follow-up links
native keyboard state +0x138 through mapping +0x26c8 to this exact local source.
Physical-keyboard reachability is instruction-supported; the captured true
result's device branch and physical-only provenance are not established.
Mouse/controller/callback alternatives, bit overrides, scalar outputs and a
timed position-derived branch prevent treating the whole routine as input-only.
No safe suppression boundary yet. Next: classify the local scalar outputs and
their immediate consumers/nonphysical override. Details in the investigation.

The initial GDB attachment refused an internal software breakpoint under the
no-memory-write permission; its residual group stop was cleared for this exact
PID. Hardware-only attachment then succeeded. Watchpoint deleted, debugger
detached; final TracerPid=0 and process running normally. Temporary debugger
stops/debug-register instrumentation occurred, but no game-memory writes,
software breakpoints, hooks, automated input, launches or installed changes.
Scratch evidence is outside the repo. Pinned-build verifier PASS;
git diff --check; documentation only, existing edits preserved, no commit/push.

## 2026-10-10 — Exact command-controller alias escape; writer still unresolved

Returned to the local embedded source instead of extending the light-resource
lead. Verified master setup passes human-controller+0x10 as the fifth argument
to 0x1412a8a90; that helper retains the same pointer at receiver+0x1ec0
and copies it into subordinate records' +0x40. Its bulk clears target separate
storage. This expands the exact-instance alias inventory but proves no write
to command-controller+0x18. Bounded pointer/leaf-accessor and chained-unwind
checks found no verified runtime source assignment. Generic indexed setter
capability still lacks an exact-instance invocation and physical provenance.
No suppression boundary. Recommended different evidence: a future scoped
debugger write watchpoint on a freshly validated source qword, after initialization,
capturing instruction/destination/registers/call stack and changed bits. Not
performed here. Documentation only; prior edits preserved; git diff --check;
no launch, instrumentation, hooks, process-memory writes, installed/executable
changes, commit or push.

## 2026-10-10 — Named-record loader and light factories resolved statically

Following the universal-modder reverse-engineering evidence workflow, traced
component+0x90 assignment to an rLch resource-loading caller. Its loader
constructs rLch::Chr groups and supports rLch::OverrideParamHolder named
records (with a separate dynamic-ID fallback). The bounded child factory's
twelve alternatives return uLlk light DTIs/factories, not virtual-pad DTIs.
Traced stream-backed selector construction and BoolUnitProperty's initial
value byte/property-string loading; factory discriminator and operation type
are distinct stream fields and must not be conflated. Concrete selected
record, literal property, successful selector match and runtime instance
remain unproved. No physical-input provenance or safe suppression boundary.
Details and addresses are in docs/exclusive-input-investigation.md. Only
documentation changed; existing edits preserved; git diff --check. No launch,
hooks, instrumentation, game-memory writes, installed/executable changes,
commit or push.

## 2026-10-10 — Target-array assignment and property record traced

Verified 0x1411fe55f stores a factory-produced pointer in the target array.
The array is container+0x18 within component+0x98; adjusted container helpers
explain allocation, pointer copying and zero initialization. Target production
follows data group/child selection, child virtual +0x128, then returned-object
virtual +8. Property text comes from a type-3 value entry in a separate named
record; execution matches the operation's selector to the child's string.
Selected factory identity, literal property text and concrete runtime pairing
remain unresolved. No local bitset alias, physical-input provenance or safe
suppression boundary claimed. Documentation only; unrelated edits preserved;
git diff --check; no launch, hooks, instrumentation, memory writes, installed
or executable changes, commit or push.

## 2026-10-10 — Property operation target and name narrowed

Static construction installs 0x1411fdfe0 as a small operation's virtual +8;
its property name is a caller-supplied, capped inline string, and its value
byte comes from the construction caller. Verified dispatch 0x141200fe9
passes an element of component+0xb0's object array as the target. The
operation lives in a three-slot component list. No array-element alias to
the local command bitset or actual "Bit" string binding is proved.
The narrowed next edge is the selected target-array element's assignment
and its corresponding source property string. No physical-input provenance
or safe suppression boundary claimed. Documentation only, unrelated edits
preserved; git diff --check; no executable/installed changes, launch, hooks,
instrumentation, memory writes, commit or push.

## 2026-10-10 — Indexed property setter dispatch resolved

Pinned static instructions now connect generic property enumeration/lookup
0x1421711f0 to dispatcher 0x14218f3f0: descriptor+0x18 supplies the
receiver, +0x40 supplies the index, +0x30 supplies the tail-call target,
and the caller's byte supplies set/clear. For the known bitset descriptor
this would reach 0x140268500. Caller 0x1411fe07b supplies index zero
and a float-state-selected byte, but its object/name have no proven alias
to the exact local command bitset. Generic property-copy code independently
corroborates dispatch semantics. No physical provenance or suppression
boundary claimed. Next: resolve that caller's incoming target object and
property name. Documentation only, existing edits preserved; git diff --check;
no executable/installed changes, launch, hooks, instrumentation, memory/input
writes, commit or push.

## 2026-10-10 — Embedded source bitset property setter identified

Read-only pinned instructions resolve bitset_prop<46> registration descriptor
receiver/getter/count/setter aliases. Generic setter 0x140268500 performs
indexed BTS/BTR dword writes at bitset+8, which would reach command source
+0x18 on the embedded receiver. No invocation on that exact instance or
device provenance is proved; descriptor copying is not a source-bit copy.
Targeted direct-reference search found no calls/jumps to the setter; indirect
descriptor dispatch remains the precise blocker. Next: resolve a bound
descriptor+0x30 invocation and its index/value producers. No suppression
boundary claimed. Documentation only; unrelated edits preserved; git diff
--check; no executable/installed behavior, hooks, instrumentation, memory
writes, game launch, commit or push.

## 2026-10-10 — Source bitset initialization and dependency narrowed

Read-only pinned code establishes command-controller+8 as cVirtualPad and its
embedded bitset_prop<46> storage at command-controller+0x18. Constructor
0x141371ce7 zeros that exact source qword. Source bits 0/1 feed records used
for outputs 2/3 and 4/5 respectively, with history/timer and player gates;
there is no identity mapping or independently established action meaning.
Static master/base update ordering is recorded. No non-initialization writer
or physical-device-exclusive provenance was verified; bounded search is not
exhaustive. Next: alias/xref trace of embedded virtual-pad/bitset pointers,
including helper/bulk writes. Documentation only, existing edits preserved;
git diff --check; no game behavior, installation, hook, input write, launch,
commit or push.

## 2026-10-10 — Command-bit owner and reconstruction resolved

Read-only pinned loaded-code analysis identifies controller+0x10 as
cPlayerCommandController through vtable/getter/DTI evidence. The transition
queries output bits 2..5 at +0x780; +0x860/+0x8a0 are query bookkeeping.
Verified controller caller 0x14118e350 -> 0x141230bb0 expands source +0x18
bits into timed/edge-maintained records, clears and reconstructs output bits,
then conditionally filters them. Exact source-bitfield producers and physical
device provenance remain unknown; no hunter-only suppression boundary claimed.
Next: bounded trace of this object's +0x18 assignments, not global query hooks.
Only investigation documentation changed; git diff --check; no installation,
executable behavior, hooks, input writes, launch, commit or push.

## 2026-10-10 — SPL DoAction detour and original path resolved

Installed Core decompilation/cache plus loaded delegate/JIT chain identify
0x140269ce0 as SPL ActionController's existing DoAction hook. It notifies
entity/local-player callbacks by reference, then calls Original on normal
paths; exceptions can interrupt that call. Actual OriginalFunction delegate
points to trampoline 0xfffc0060, which restores displaced instructions and
returns to 0x140269ce7. CrafterHunter implements neither action callback.
Native request can enter current state or queue it. A verified conditional
consumer 0x14026a2d0 -> 0x14026a1e0 dispatches current action +0x30; entry
495's target is 0x140eef2f0. No observed execution of destination (1,495),
action semantics or physical-input-only boundary is claimed. Next: command-bit
producer provenance upstream of the transition predicate. Detailed evidence
in the investigation report; git diff --check passed. Documentation only,
no game writes, instrumentation, installed changes, launch, commit or push.

## 2026-10-10 — transition result to action-state selection, read-only

Verified generic vtable+0x30 dispatch at 0x14026ba7c against actual
PopGimmickButton instances in the local controller's constructed +0xa00
table owner. Consumers match the returned integer to selection records;
-1 rejects a record, while 0 can select a destination pair. Optional
callbacks can alter results. One concrete record targets (1,495), whose
action entry metadata names nHmAction::cActEnvBashKick; actual action semantics
remain unproven. Downstream 0x140269ce0 is already detoured, limiting claims
about live state-update execution. No hunter-only suppression boundary.
Next: establish that existing detour's provenance and original-call behavior.
Full bounded-search coverage and evidence in the investigation report.
git diff --check passed. Documentation only; existing changes preserved.
No hooks, instrumentation, memory/input writes, installed/executable changes,
game launches, commits or pushes.

## 2026-10-10 — classify transition 0x2a early-out, investigation only

Read-only loaded-code inspection verifies 0x140ef5db0 in the constructed
nHmTransition::cPopGimmickButton vtable, with additional transition-family
table references. Active sPlayer state 0x2a returns -1 after controller
registration/cache maintenance, bypassing eligibility/command queries and
success flag writes. It does not establish movement/attack semantics or a
hunter-only suppression boundary. Next: trace consumption of its virtual
0/-1 result into transition/action selection. Details and exact evidence are
in docs/exclusive-input-investigation.md. git diff --check passed; documentation
only, no hooks, memory/input writes, executable/installed changes, launches,
commits or pushes. Existing unrelated edits preserved.

## 2026-10-09 — pinned-build input trace, no executable changes

At b421dc0, verified executable SHA/revision/Steam build and installed SPL bytes
against its 2.0.0 Linux archive. Read-only access to already-running PID 110141
allowed loaded-code inspection (on-disk text remains encrypted). Keyboard writer
0x1422E7AD0 is already SPL-detoured; its native caller produces edge state after
the writer. Installed SPL filters shared keyboard/mouse state for ImGui, not a
verified hunter-only consumer. Independently resolved the game's XInputGetState
thunk to installed Wine xinput9_1_0; decoded a pad routine with XInput and
DirectInput branches. No controller gameplay support is claimed. Installed hook
and unload code has cleanup/thread-order caveats; OnUpdate alone does not prove
post-poll/pre-hunter timing. The semantic hunter consumer remains unidentified.
Full evidence, candidate ranking and exact missing prerequisite appended to
docs/exclusive-input-investigation.md. Host verification passed; no observer
hook, input write, installation, game launch or production change. Extracted
excerpts stay outside the repository. Documentation left uncommitted/unpushed
as requested for review.

## 2026-10-09 — reference input ownership investigation, no suppression installed

Reference source/history at d9cac469 shows F8 window-focus handoff: guest pauses,
releases mouse/hides window; host requests foreground focus; host F8 counter
shows/focuses guest again. Cocoa prevents transparent-window click-through.
This is not a verified hunter gameplay input hook and does not establish
controller suppression. SPL queries and pre-main-update callback do not prove
the required polling/consumption order. Stopped per user's explicit safe-hook
gate rather than clearing global input or replacing the loader. Comparison,
source links, missing prerequisite and future manual plan are documented in
docs/exclusive-input-investigation.md. No code copied or binaries changed.
User manually accepted player-preview occlusion and movement/pose in the tested
setup, including seated boat pose; this is not exclusive input or terrain support.

## 2026-10-09 — player depth viewport conversion, live acceptance pending

User's fixed-camera real/source screenshots show foliage-shaped depth rejection
over Steve against visible sky. Runtime selected depth allocation is 1920x1080,
but observed writer viewport is (0,0,1440,810); player/pre-UI viewport is full-size.
With approval, map player pixels to the selected resource's observed writer
viewport before integer Load. Track first depth-enabled/write-enabled draw each
frame in normal mode, not only debug mode. Missing/stale/invalid viewport refuses
player draw; no guessed scale, lifecycle guard relaxation or depth comparison change.
Diagnostic shaders/probes share the conversion. Geometry/skin/pose/anchor and
stone/frame/paired paths unchanged. Regression checks cover the measured torso
sample (781.945,371.781)->(586,278), identity, nonzero origin and pixel centres.
First-writer viewport correspondence remains a live acceptance requirement, not
proven solely by metadata. No occlusion acceptance or gameplay claim.

## 2026-10-08 — player-model artifacts installed, runtime gate still open

After all builds/headless checks passed, closed only Minecraft game PID 322466
with SIGTERM (MHW was already absent); launchers were left untouched. Installed
a704151's Fabric 0.3.0 JAR and native renderer DLL in their existing mod/plugin
locations. No managed binary change was needed. Byte comparisons match builds.
Fabric SHA-256: 68d76bd7d5a396e2fd78b861ce1e2a7394d772894b4560f4d0a07766dbdc660a.
Native SHA-256: 27a25c38dcd611b9f52d2512281e5b8a71df836dc00b1e7639129732935610eb.
Scoped previous artifacts: ~/.local/share/crafterhunter-backups/player-model-install.5s1taW/;
native flag/license/DLL rollback: native/mhw-renderer/build/rollback.xhRcfa/.
Player flags remain opt-in, game saves/worlds and launch configuration unchanged.
Games not relaunched. Live skin/export/submission are NOT proven; the orbit test
must wait for fresh complete-player upload and draw-submission evidence.

## 2026-10-08 — baked complete player model, headless checkpoint

User observed that the paired preview only includes surfaces visible to the
Minecraft camera. Confirmed: the paired shader reconstructs guestDepth samples,
not hidden geometry. Enlarging those samples cannot supply a complete back/side.
User approved investigation then the smallest player-only geometry slice.

Inspected local 26.2 bytecode: AvatarRenderer (not PlayerRenderer) extracts
AvatarRenderState; LivingEntityRenderer.submit supplies body PoseStack; PlayerModel
setupAnim supplies real bones; ModelPart.Cube polygons supply every face and UV.
Implemented a non-drawing collector for only the actual main player model,
restoring shared poses/visibility afterward. Static baked triangles + resident
skin travel through bounded atomic player.asset; per-part matrices/feet through
player.pose at the existing 4 Hz budget. Existing feet anchor and lifecycle guards
remain authoritative. Added an opt-in pre-UI structured-buffer player draw using
the same host camera/depth. Frame/stone shaders and bridge are not replaced.

Actual normal/slim baked model tests prove 12 cubes, 72 faces including all six
directions, 432 triangle vertices, normalized UVs and native walk/head pose
changes. Actual baked geometry/matrices roundtrip Java/native. Texture fixture
is synthetic, NOT live skin proof. Passed native malformed/matching/freshness/
negative feet tests, isolated controller contracts, all eight shader entry-point
checks, Fabric/native/managed builds, existing reprojection/frame/capture/depth/
SPL regression suites and 26 Rust tests. No runtime draw/skin/orbit/FPS acceptance.

Scope/risks/commands recorded in docs/player-model-preview.md. Armor, items,
cape and other layers are excluded. No world export, collision or gameplay added.

## 2026-10-08 — source-sized paired surface footprints

User screenshots show the actual Minecraft skin in MHW in front/rear F5 views,
but dotted surfaces. MHW was already closed; terminated only the Minecraft game
PID with SIGTERM and confirmed both games absent. No saves/worlds were edited.

Fixed the existing paired shader's fixed four-host-pixel patches: each sample now
projects its four-source-pixel cell at that sample's depth through the existing
camera/anchor transforms. At 949 -> 1920 width, the old patches leave about 4.09
pixels between equal-depth samples. Sample/vertex counts, calibration, lifecycle
guards, scene-depth selection and stone rendering are unchanged. No gameplay added.

Passed reprojection/anchor numerical regressions (resolution, aspect, small
targets, shared edges and camera magnification), preview contracts, all shader
entry-point checks, depth selection, native build and 26 Rust tests. These are
headless checks, not live surface-quality or performance acceptance.

Installed only CrafterHunter.Render.dll via the existing installer; installed and
built SHA-256 match: 3d4831a9b287607e1d4db40dde7e984614192d10797a4829b464b866cee5aad1.
Previous renderer/flag/license are in native/mhw-renderer/build/rollback.wV9sRF.
Games remain closed. Coarse depth edges and disocclusion remain limitations;
next manual gate is surface continuity with the same first-person align then F5.

## 2026-10-08 — player-anchor artifacts installed

User requested automatic installation after verified changes. Confirmed no MHW
or Minecraft game process/window; launcher and Gradle daemon were left untouched.
Installed b041c2d's matching managed/native DLLs in the existing MHW CrafterHunter
plugin directory and Fabric 0.3.0 JAR in ~/.minecraft/mods. Existing scoped files
backed up under ~/.local/share/crafterhunter-backups/player-anchor-install.9ZLcv3/
(mhw/ and minecraft/); native installer also created build/rollback.HL92qZ.
All three installed artifacts compare byte-identical to their built sources;
managed install marker matches the installed SHA-256. Bridge remains active on
127.0.0.1:38470. No saves/worlds, other mods or launch settings changed; no games
launched. Player visibility and alignment still require manual acceptance.

## 2026-10-08 — player-feet calibration, existing compositor preserved

User supplied visible village point-cloud screenshots, then requested both games
closed before the next scoped fix. MHW was already closed; Minecraft received
SIGTERM (no SIGKILL), and both process absences were checked before changes.
No save/world files were edited. Games remain closed.

Camera-to-camera calibration puts the guest player's eye at the host viewpoint,
not at a visible player reference. Added snapshot v3 within the existing 320-byte
header: finite interpolated local-player feet at 280, provenance at 304. Level
render HEAD samples the same frame's partial tick; issue-time frozen copies travel
with both attachments. v2 archives remain readable but cannot player-calibrate/draw.
Native calibration now maps guest feet to existing SPL Player.MainPlayer.Position
in metres. Camera continuity guards, first-person calibration requirement, fixed
F5 anchor, shader/depth selection and renderer architecture are unchanged. Missing
host player invalidates; no guessed height offset, movement writes or input handoff.

Passed Fabric and managed/native builds; Java/native v2/v3 pairing, nonfinite and
provenance refusals, feet-to-feet/F5 anchor regression, existing reprojection,
capture-boundary, preview, shader, depth selection, SPL lifecycle and Rust tests.
Not deployed or runtime accepted. Sparse holes and front-camera source coverage
remain limitations; the hunter may occlude a coincident Steve. No claim that this
fix alone makes Steve visible in every F5 view or completes playable MVP.

## 2026-10-08 — visible-workspace preview reaches paired draw submission

With both user-launched games on inactive workspace 2 and the terminal on
workspace 1, new cadence logs measured consecutive producer identities about
1000 ms apart, matching consumer intervals. This is producer slowdown, not a
4 Hz stream sampled at 1 Hz. The user switched to workspace 2; without changing
thresholds or composition code, producer intervals settled near 255 ms.

Generation 1113423980880: calibration accepted at tick 1280720, epoch 1372;
scene depth 0 measured 56.56% coverage and selected at 1281847; actual paired
reprojection draw submitted at 1281907. Epoch 1372 remained armed through
1366306, over 85 seconds. This passes the immediate upload/calibration/eligibility/
submission gates in the visible-workspace setup, not visible or aligned rendering.
The exact guard trigger in the earlier session was not recorded and cannot be
reconstructed; off-workspace one-second observations demonstrably put both
freshness and host continuity at their existing thresholds. No guard bypass.

The session-scoped bridge remained active with both roles registered and UDP
38470 listening. Earlier listener loss remains unexplained: no historical exit
record proves shutdown versus crash. Updated preview instructions require both
windows visible on the active workspace. Exact compositor/driver throttling and
visual depth/alignment/F5 acceptance remain unproven. No gameplay changes.

## 2026-10-08 — failed preview investigation, live gate still blocked

Fresh native logs show accepted calibrations followed by the combined host
jump/gap/invalid-pose refusal, with one-second upload intervals and no paired
draw submission. Those logs do not record the triggering pose or observation
interval, so an exact root cause cannot be claimed retroactively. Both games
and the bridge were closed when this investigation reached runtime checks.

Added narrowly scoped refusal classification with old/new host positions,
distance and host-clock times; preserved all existing thresholds and reset
behavior. Added same-generation producer issue-time intervals, consumer
observation intervals and identity steps to distinguish producer throttling
from consumer sampling. Upload-driven anchor loss is now explicitly logged.
Regression checks retain exact 1 s/25 m boundaries and distinguish NaN,
rollback, gap and jump. No gameplay or composition behavior changed.

Local 26.2 bytecode shows AFK/minimized frame throttles of 10/30 FPS, not
1 FPS; pauseOnLostFocus is already false. This rules out that option alone,
not other scheduling stalls. PortUnreachable proves an unavailable listener,
not whether it exited or was never started. The paired frame channel is
shared memory, independent of bridge UDP. Runtime gates remain unaccepted;
these diagnostics are required before choosing a non-speculative fix.

## 2026-10-08 — opt-in real paired GPU composition slice, not playable acceptance

User narrowed work to Playable Steve MVP and required implementation rather than
further diagnostics. Audit confirms paired GPU uploads were unused by drawing,
old colour rendering remained sky-only, host player proxy runs in the opposite
direction, session/ownership negotiation and guest terrain collision are missing.

Added actual paired reprojection draw at the existing pinned pre-UI signature,
reusing measured scene-depth selection and saved context state. Real guest depth
unprojects through captured inverse projection/view/position and explicit anchor;
host metres become GPU centimetres once. Guest samples test against host reversed
depth and private depth orders overlapping guest samples. Preview samples every
fourth texel, not continuous geometry; terrain is included and holes expected.
New flag never silently falls back to sky-only on refusal. Stone drawing unchanged.

Fabric opt-in bounded streaming supplies at most four paired captures/second,
one in flight, skipping diagnostic artifact writes without weakening failure
retention. Preview control start/align/stop uses atomic writes and preserves frame
channels. Lifecycle cleanup added for all paired GPU resources on CH_Stop.
No input takeover, host camera writes, terrain import or gameplay claim.

Verified Fabric build; managed/native builds; six shader entry points compile
under glslang (not proof of D3D runtime compilation); paired transport/freshness,
reprojection/alignment, Fabric capture and Rust tests pass. Controller regression
tests cover temporary-root start/align/idempotent stop/channel preservation; shader
contract checks cover resources, host-depth comparison, origin refusal and bounds.
No deployment or live demonstration. Next: one scoped manual composition test,
with screenshots, advancing identities and before/after focused frame rate. See
docs/playable-steve-mvp.md for audit, hypothesis, acceptance and known limitations.

## 2026-10-08 — host-local calibration diagnostic, no guest drawing

Added opt-in align.request consumption by rename-to-claim, requiring a fresh paired
upload in first-person. CH_AlignmentHost observes SPL camera in metres; explicit
calibration fixes guest/host camera origins, and logs transformed camera position
with generation/identity/F5 mode/host-local epoch. F5 does not recalibrate. Host
jump >25 m, gap >1 s, clock rollback/invalid pose or lost upload clears the anchor.
These are continuity heuristics, not an authoritative area/title detector.
Control tool now publishes complete requests using atomic same-directory replace.

Verified: reprojection/alignment regression tests including jump/gap/rollback and
third-person calibration refusal, managed plugin build, native MinGW renderer build.
No deployment/game requests or live acceptance. Stone/depth selection and sky-only
composition untouched. Full calibration protocol/ownership acknowledgement and
visual alignment remain missing. Diagnostic needs advancing paired uploads within
the existing one-second watchdog, so bounded pacing/trace collection comes next
before asking for manual acceptance. No 60 fps or visible Steve claim.

## 2026-10-08 — explicit shared-world anchor reference, drawing unchanged

Reviewed three user F5 archives (215619Z/215700Z/215730Z): identities 3/6/9
in generation 700129956865, matched colour/depth hashes, all finite depth,
colour-aligned bottom-up rows and recorded zero-to-one reversed-Z projection.
Rear/front camera displacements measure 4.0000002 blocks, opposite about the
first-person position. This supports render-camera capture, not host alignment.

Added world_alignment.hpp: explicit same-XYZ guest/host origins and positive
metres-per-block scale; requires armed nonzero matching guest generation/host
epoch. Composes the actual captured view inverse and camera position without
per-F5 re-anchoring. Headless tests require an unchanged block to map identically
through all three camera modes, with translation/scale/inverse and stale/reset
refusal. Existing reprojection test entry point runs these too.

No native drawing, telemetry protocol, input, deployment or game requests changed.
This is the mathematical contract only: calibration exchange and host epoch
lifecycle still need implementation before a manual alignment diagnostic exists.
Agent/manual next objectives are recorded in AGENTS.md and world-reprojection.md.

## 2026-10-07 — exact render arguments and paired snapshot v2

Local Loom 26.2 bytecode establishes GameRenderer copies CameraRenderState's
projection, applies bob/hurt and nausea transforms, then uploads that modified
Matrix4f before invoking LevelRenderer.render with actual camera and view arguments.
The previous resampled CameraRenderState projection missed those effects. Added
observational ModifyArg on the final upload (returns the identical object), and
LevelRenderer HEAD capture of the actual view and double-precision camera position.
Per-world-frame reset prevents last-frame metadata surviving a missing hook.

Paired v2 header carries final projection, actual view rotation, f64 position, all
three F5 modes, queried GL clip-control mapping/origin and depth range, and pinned
main-pass clear 0 provenance. Missing exact arguments/backend metadata refuses
transport, not GPU-lifetime policy or diagnostic publication. Native rejects v1
and unknown provenance, prepares inverse projection/view and eye-to-guest transform
for upload-only staging. Existing stone and colour-only drawing unchanged.

Headless bytecode checks pin effect-before-upload-before-level ordering, exact hook
signature, reset and main-pass clear. Java/native round trip covers all F5 mode
values, f64 precision, view/mapping offsets, restart/resize and twelve malformed
refusals. Fabric/native builds, capture completion/readback-plan tests, 17 depth
validator regressions and affine view/position reprojection checks pass.
This does NOT establish real F5 camera alignment, native GPU contents or
visible Steve. No deployment or live requests. Next fresh v2 runtime camera evidence
and guest/host shared anchor conversion before depth-aware composition.

## 2026-10-07 — projection reference and real archived-block replay

Added column-major inverse/unprojection and explicit eye-to-host/current-host-VP
projection reference, with host-space reversed-Z occlusion. Refuses invalid/clear
depth, unusable matrices, behind-camera and out-of-frustum points. Window mapping
and clear value are explicit caller inputs, not guessed from matrix coefficients.
Upload-only staging now retains matching inverse projection and refuses singular
projections before allocation; neither existing drawing path binds new views.

Native read-only replay of the two hash-bound archived bedrock captures reconstructs
4.894803 and 12.121316 blocks versus independently known 4.895355 and 12.122026.
Ray/face intersection is checked, not just infinite Z-plane distance. Headless math
tests cover pixel centres, bottom-up orientation, off-axis rays, translation/rotation,
window mappings, clipping and invalid cases; native build and paired transport
regressions pass. No replacement Minecraft geometry or
assets generated, no deployment or runtime requests. This is not live GPU alignment.

Snapshot v1 lacks full issue-time view/effects and depth-mapping provenance. The
next gate is obtaining those and validating guest/host anchor conversion, not
guessing an exact camera from player Euler angles. docs/world-reprojection.md and
AGENTS.md record the boundary for the next agent.
User confirms the target includes all native F5 camera modes; added that requirement
to the agent contract. First/third-person gameplay remains unimplemented.

## 2026-10-07 — freshness-gated paired upload, not composition

Added host-local watchdog and bounded generation retirement for world snapshots.
Startup/restart requires identity advance; duplicates cannot refresh liveness,
regression and retired generations are refused, and generation exhaustion fails
closed. Native upload-only diagnostic is off by default (world-upload.enabled).
It polls at 250 ms intervals, atomically stages immutable RGBA8/R32_FLOAT textures
and views with matching issue-time metadata, releases CPU attachments, and clears
staging on expiry/refusal/failure/disable. It never binds these textures for drawing.
Stone shader, scene-depth selection and existing colour-only composition unchanged.

Headless transport tests include startup stale-file refusal, duplicate expiry,
recovery by advance, local clock rollback, restart warmup, retired-generation
refusal and bounded exhaustion. MinGW renderer build, existing shader compilation,
depth selection, frame-source, frame-clock and composite checks pass. No installation or
runtime request writes; real Proton file sharing and GPU upload contents remain
unverified. This is request-paced scaffolding, not 60 fps or visible Steve.
Next: projection-normalised reprojection, then scoped live upload/composition
acceptance with advancing identities, real blocks and current host-camera evidence.

## 2026-10-07 — manual block-depth gate and bounded paired transport

User screenshots plus archived captures establish two distances to existing
bedrock at (917,-60,348), F3 target confirmed in the second screenshot.
Archives 20261007T132237Z and 20261007T132722Z: identities 9 and 12,
generation 1391383374261, centre raw pixel (474,514), 949x1028, FOV 70.
Expected centre-ray distances to the known near-face Z plane are 4.895355 and
12.122026 blocks, from pose and block coordinates independently of depth;
reconstructed about 4.895 and 12.121. Strict validator passed hashes, projection,
finite scene depth and colour/depth orientation at both distances. Both used
pre-hand-clear fallback; this does not verify the optional always-on-top hook,
linked MHW camera, isolated Steve or guest composition. Prior 4.9-degree FOV
did not reproduce at these independent-camera poses, not a global FOV fix claim.

Implemented request-paced WorldFrameChannel publication of a complete paired
snapshot with issue-time generation/identity, pose and projection. Atomic
same-directory replacement isolates incomplete writes; bounded size and malformed
header checks precede native-reader allocation. Separate world.frame path never
modifies the existing colour channel. Transport refusal preserves diagnostic
publication. Added portable native reader, not connected to renderer.cpp, and
docs/world-frame-transport.md with byte offsets and explicit freshness limitations.

Verified headless Java/native binary round trip; pair consistency during concurrent
Java reads; restart/resize; wrong-size producer refusal retaining prior file; eight
native malformed-file refusals retaining prior output. Fabric build and existing
capture tests pass. No deployment, game launches or runtime requests in this turn;
Windows/Proton consumer sharing, linked pose fidelity, continuous throughput and
native depth composition remain unverified. Next: freshness-aware host selection
and upload, before projection-normalised depth composition.

## 2026-10-07 — strict block-distance acceptance, not copy-completion success

The next goal remains actual world-depth acceptance, not native composition.
Implemented an opt-in --acceptance mode in the existing depth validator:
requires BOTH, captureBoundary, positive generation/identity, matched file hashes,
finite in-range non-flat depth, a finite recorded projection, CONFIRMED colour/depth
orientation, and independently known camera-forward surface distance. Diagnostic
mode remains available for old artifacts without pretending its exit zero proves
the acceptance gate. Pixel bounds and positive finite ground truth are now checked;
negative Python indices previously sampled a different pixel silently.

WorldCapture now publishes SHA-256 hashes of the exact raw attachments with each
metadata identity. They detect stale/mixed or partially overwritten files, not
physical pixel validity. No asynchronous resource-lifetime policy, hook, live frame
channel or native compositor changed. The runtime script now explicitly calls a
publish completion rather than claiming valid paired rendering.

Added docs/world-depth-acceptance.md and an AGENTS handoff: two existing opaque
Minecraft block measurements, independent surface coordinates, raw bottom-up pixel
indexing, fresh screenshots/logs and archive, with the previous FOV observation
still unresolved. No new meshes, synthetic game scenes or gameplay writes.

Verified headless: 17 validator regressions pass (including all-zero depth,
undecidable orientation despite a plausible pixel, mismatched files, absent ground
truth, wrong distance, negative pixel and NaN); bytecode hook check passes;
frame/readback-plan tests pass; runtime script bash syntax passes; Fabric build
passes. No deployment, launcher tasks, runtime request writes or live acceptance
performed. Next: permission-scoped deployment/restart and the documented real-block
gate; do not mark Steve/depth-correct guest composition complete from these tests.

## 2026-10-07 — capture before the earlier always-on-top depth clear

Read-only inspection of the latest capture (14:50 local, identity 1) found
depthMin=depthMax=0 and depthDistinctApprox=1 despite both copies completing.
Loom's 26.2 LevelRenderer bytecode exposes a destructive clear in
lambda$addAlwaysOnTopPass$0 before executeAlwaysOnTop. This frame-graph pass
executes before LevelRenderer returns, so the existing GameRenderer pre-hand-clear
hook was already too late when this pass ran. This is a concrete earlier erase,
not evidence that the OpenGL readback recipe needs replacing again.

Added LevelWorldCaptureMixin immediately before that earlier clear. The pass is
conditional (hasAnyAlwaysOnTop); the GameRenderer hook remains as fallback when
it is absent. A per-renderLevel guard services capture exactly once, reset at
HEAD. Metadata records captureBoundary for the issued identity, not whichever
boundary a later frame reached. Both mixins require one injection match; the
synthetic method is explicitly version-pinned. No native renderer, transport,
GPU lifetime policy, installed game files or saves were changed. Corrected the
plan's overclaim about accepted full guest colour/depth composition.

Headless checks: python3 tools/test-world-capture-hook.py pins the early-clear
method, its bytecode ordering, optional-pass predicate, fallback wiring and
registered mixins (the old hand-only hook fails this check); bash
tools/test-fabric-frame.sh passed; python3 tools/test-validate-world-depth.py
passed all eight fixtures; Fabric ./gradlew build passed; git diff --check passed.

Not verified: runtime mixin application or nonzero scene depth after this change.
No deployment or game launch performed. Next agent: deploy only with permission,
restart the guest, archive a fresh paired capture with captureBoundary and
matching identity/projection/colour/depth, and test an existing Minecraft block
at independently known camera-space distances. Verify orientation against the
scene, not capacity or callbacks. The previous 4.9001036-degree FOV remains an
unexplained observation, not a corrected projection or a proven FOV bug. Do not
change native composition or begin gameplay integration until that gate passes.

## 2026-10-04 — scope and recon

User target: real Minecraft inside MHW Iceborne with world, monster, and story
interaction. Existing repository already chooses a separate-process bridge.
Created `MODDING_PLAN.md` with ordered acceptance milestones and explicit
ownership of simulation and progression.

Read universal-modder mod-any-game, game-recon, mashup-mods, and
share-field-notes instructions. Checked the plugin's local knowledge index;
no MHW-specific entry. The CLI is absent from PATH, but the plugin's source
and skills are locally available.

`tools/doctor-linux.sh` found the host installation, Proton prefix, SPL
bootstrap, CrafterHunter endpoint, and build prerequisites. Fingerprint matches
the previous recorded executable and Steam build. No game installation changed.
Reviewed public SPL callback docs and the credited macOS crossover README.
Full story compatibility is not established by these sources.

`cargo test --workspace`: seven tests passed. Direct execution of
`tools/test-fabric-camera.sh` failed because the file is not executable;
run it using Bash without changing file permissions.
`bash tools/test-fabric-camera.sh`: passed anchor, movement, yaw wrapping,
recovery, world change, toggle, decode, and freshness checks. These are
headless checks, not evidence of live game compatibility.

Next acceptance: live camera test, then a depth-composited cube on Linux/DXVK.
Ownership/offline test profile confirmation is pending. No gameplay mutation,
save editing, or game launch was performed during recon.

## 2026-10-04 — live camera and rendering probe

User confirmed that Minecraft follows the MHW camera while both games run.
Screenshots show the Fabric HUD reporting a live packet feed and bright green
probe geometry in an MHW expedition. The first probe used an assumed camera
forward axis; its edges appeared off-centre and sometimes over foreground
objects. This is evidence that `OnRender` and primitive drawing work, but not
proof of correct placement or depth occlusion.

Version 0.2.1 derives the probe's centre ray from the viewport matrices.
Headless tests pass for two view directions, reversed depth, and a singular
matrix; the plugin builds with no warnings. The user still needs to check its
placement and whether MHW terrain hides it. No game was launched or installed
by the agent during this change.

Further 0.2.1 screenshots from Astera's smithy show only a few green cube
edges. The installed DLL hash matches the published 0.2.1 artifact. Because
the smithy is full of close geometry and the camera is aimed steeply down or
up, these images cannot distinguish clipping/occlusion from a placement error.
The next acceptance scene is a broad clearing with a level camera, followed
by a controlled wall-occlusion shot.

User corrected the interpretation of those screenshots: the green geometry
stays in a player-relative direction, moves incorrectly with camera rotation,
and appears over foreground objects. This is a placement/depth failure of the
0.2.1 probe, not merely a crowded-scene artifact. Version 0.2.2 switches the
render probe to the loader's world-space camera target and logs up to twelve
camera/target/centre/projection samples to the loader log. It adds a magenta
centre marker to distinguish pose from line rendering. Headless target-ray
checks and the .NET plugin build passed; live placement is pending.

The user then shared expedition screenshots and a recording of 0.2.2 showing
a magenta wire sphere near screen centre and isolated green lines. The probe
log shows camera/target motion, but every recomputed centre projects to about
(960, 540). This exposes the placement error: the draw location was recreated
in front of the camera every sample. Version 0.2.3 anchors the cube once in
world space during ordinary camera movement, re-anchors after a camera jump
over 25 m, and removes the magenta diagnostic sphere. The screenshots still
suggest that debug primitives do not provide the depth behaviour needed for
Minecraft geometry; that remains an open milestone.

## 2026-10-04 — stone A/B/C first live result

The user's 23-second expedition recording shows the v0.3 comparison labels.
The visible red cube sits under **C**, the projected image-quad path, and draws
through MHW foreground. **A** has no visible mesh and **B** shows only a few
white points. The SPL log confirms Minecraft's stone asset arrived, four
color meshes were registered for A, and the 16×16 PNG loaded for C. The stone
PNG in Minecraft's local client cache is grayscale, so the red C result is
not the intended block color. A/B are staged against dense nearby terrain;
this footage alone does not prove whether they are occluded or the primitive
draws fail. These are observations, not proof of a depth-correct render.

Version 0.3.1 changes C to draw the actual transmitted RGBA pixels directly
as ImGui quads, eliminating the suspect GPU texture-handle path. It moves the
three 70 cm blocks to a tighter row, 40 cm above the original anchor, and adds
full-length pixel-colored edges to B so its line pass is legible. C remains
depth-unaware by design; a host depth-buffer integration is still required.

## 2026-10-04 — cursor overlap confirms overlay ordering

The v0.3.1 recording shows C with the actual gray stone pixels and cube
perspective, but it still covers the hunter and terrain. A's label is visible
against open sky while its mesh is absent; B still contributes only tiny
fragments. The user's observation that C covers the cursor is additional
evidence of the foreground ImGui draw-list ordering, not evidence that the
camera transform itself is wrong.

Version 0.3.2 puts C's pixels on ImGui's background draw list, leaving labels
foreground. This should place UI windows and ImGui's software cursor above the
block, but **does not** put MHW world geometry above it. SharpPluginLoader's
public 1.0 rendering API has no scene-depth texture accessor. The reference
MHW crossover uses a Direct3D Present composite with the game's depth buffer;
porting that behavior to Proton/DXVK requires a separate native graphics path
and validation of the actual D3D version and depth resource before injection.

## 2026-10-05 — explicit placement invalidation (0.3.3)

Baseline first: MHW in-world with the stone composing under the HUD, and the
Minecraft HUD reporting `MHW LINK: LIVE | 1 pkt/s | seq 8147 | age 269ms`,
later `WAITING | seq – | age –1ms` while Minecraft was backgrounded. The bridge
performs no rate limiting and the Minecraft log carried no errors, so the rate
reads as background throttling rather than a protocol defect. Recorded as an
open observation, not changed in this entry.

`PlacementLifecycle` replaces the process-lifetime anchor. It drops the anchor
on a single-frame camera jump over 25 m, or on over a second without a usable
camera sample, and never re-anchors: after a scene change the stone stays absent
until an explicit `place`. `control-mhw-renderer.py clear` drops the anchor on
demand. Headless checks cover the jump, camera-timeout, non-finite sample, clear
and reset paths (`Placement lifecycle checks passed`); Rust, Fabric and the
existing .NET checks stayed green. Version bump to 0.3.3.

Verified live in the running game with 0.3.3 hot-reloaded without an MHW
restart: `place` → `clear` → `place` produced anchored / cleared by request /
anchored log lines with the stone present, absent and present; camera rotation
kept the anchor and re-projected the cube; and a player-driven transition to
Astera logged `Native placement invalidated: camera jumped over 25 m in a
single frame`, after which no anchor event followed for the rest of the
observation — no automatic re-anchor — until a fresh `place` anchored at new
Astera coordinates and the stone drew there.

Injected input does not reach Proton in this session: uinput injection and
Hyprland's `hl.dsp.send_key_state` both left the game unaffected, so the scene
transition had to be player-driven.

No SharpPluginLoader lifecycle callback is relied upon, so none was verified;
placement invalidation uses camera telemetry alone. Depth-candidate selection in
`renderer.cpp` is untouched.

## 2026-10-05 — depth-candidate investigation (no selection change)

Measured the existing read-back sets instead of changing the selection rule.
`depth[0]` held scene geometry in all four capture sets across two sessions
(97.4–99.97 % coverage when terrain filled the frame, 54.2 % when sky filled
46 % of it), `depth[1]` was entirely zero at capture time in every set, and
`depth[2]` was never captured. The paired depth/colour PNGs put sky at the
reversed-Z far value, match foliage and hunter silhouette positions, and show
the stone absent from depth because the renderer reads depth and never writes it.

Candidates are appended on first discovery and pruned only on resize: `depth[1]`
appeared about two minutes after `depth[0]` in this session, so list index
records discovery order rather than scene-depth priority. `renderer.cpp` is
unchanged; any future rule must be validated in more areas and should prefer
per-frame freshness over list index.

## 2026-10-05 — live camera link acceptance: outage, rate, F7

Step 8 ran itself: the bridge process died at 05:02:07 after a clean
03:06:07-05:02:07 stretch, and Minecraft logged
`Bridge I/O failed; retrying: java.net.PortUnreachableException` at 60/min
while the HUD read `WAITING | 0 pkt/s | seq – | age –1ms`. Restarting the
bridge printed `registered Mhw` and `registered Minecraft` within seconds, a
second `Connected to crafterhunter-bridge/v1` appeared in the Minecraft log,
and the HUD returned to `LIVE` with neither game restarted. MHW noticed
nothing: its socket never errored, so no second HelloAck went out and the
ten-second camera grace was not re-applied. The bridge now runs detached from
any terminal via
`setsid nohup ./target/debug/crafterhunter-bridge > native/mhw-renderer/build/evidence/camera-link-acceptance/bridge.log 2>&1 &`.

The open `1 pkt/s` observation resolves as background throttling. Across a
68 s / 30-sample OCR series with MHW rendering, the HUD held
`LIVE | 16-18 pkt/s | age 3-55 ms` on every sample with an increasing
sequence; with MHW on an inactive workspace it fell back to `1 pkt/s` and
flapped between `LIVE` and `WAITING` as age crossed 500 ms. The producer caps
at 20 Hz, so the rate is MHW's frame rate: a window Hyprland is not rendering
blocks on Present and the camera payloads stop with it. Read the HUD with both
games on the active workspace.

F7 toggles the link cleanly: the HUD reads `OFF` while packets keep arriving,
and the second press returns to `LIVE`. Minecraft runs on XWayland, so `wtype`
never reached it; Hyprland's `hl.dsp.send_shortcut{mods="", key=..., window=...}`
did. That dispatcher targeted at the MHW window, `send_key_state`, and
`ydotool key` still leave Proton unchanged, while `ydotool mousemove` does move
the system cursor, so uinput reaches the compositor and only delivery into
Proton fails. Rotation, translation, inverted axes and the F8 fresh anchor stay
player-driven; steps 5, 6 and 9 of `docs/camera-link-test.md` remain open.

## 2026-10-05 — step 9 verified, and what a measured rotation proved

Quit and rejoined the Minecraft world with injected input only. Hyprland's
`send_shortcut{window=<window>}` drives Minecraft *when that window has focus*
(silently no-ops otherwise, which cost several attempts), and `ydotool` reaches
it through uinput, so mouse moves plus `ydotool click 0xC0` walked the pause
menu and the world list. The world unloaded at 06:04:11 and reloaded at
06:10:40; a minute later the HUD read `LIVE | 16-17 pkt/s | age < 68 ms` with
neither game nor the bridge restarted. The placement half comes from
`control-mhw-renderer.py capture`: the `enabled=1` branch was taken with
`parameters.centre` still holding the anchor, and the texels at the anchor's
projected screen position are exactly `127,127,127` and `143,143,143` — the
Minecraft stone palette, which Astera's frame never produces. The runtime log
has logged no invalidation since `anchored` at 04:46:10, so the stone survived
the bridge outage, the world unload and the reload.

Reading the renderer back also produced the measuring tool the eyeball tests
were missing. `parameters.bin` is `{inverse, projection, centre, screen}`,
`centre` is the anchor and `matrices.bin` is the same viewProjection, so the
anchor's screen position is `centre * viewProjection` under System.Numerics'
row-vector convention — the column-vector reading lands on screen centre and is
wrong (it does not match where the stone actually is). One -600 px
`ydotool mousemove` walked that projection from (483,406) to (1335,822): a real
camera rotation that did *not* invalidate the placement, exactly as the
lifecycle specifies, and the stone moved with it — the box at the new
projection is 4200/4200 neutral texels of the stone palette while the old
position holds only dark scene.

Input into Proton is still the blocker for steps 5 and 6. Pointer motion
rotated MHW twice, but after any focus switch to Minecraft the identical moves
leave MHW pixel-identical (mean 0.00-0.14 over 3 s) while the game keeps
rendering (6,790 pixels change elsewhere in those 3 s) and the link stays
`LIVE | 12 pkt/s`; a click to re-grab the pointer does not help and
`ydotool key` with W held a full second never moves the hunter. F7 and F3 do
work on Minecraft when it is focused, which is how the ON/OFF comparison was
taken: `OFF` visibly changes Minecraft's world view (mean 8.45 over 3,514
pixels), so the link's pose really is being applied. Whether it follows MHW in
the same direction still needs a person — Minecraft renders a featureless dark
grass field, and MHW ignores injected input.

## 2026-10-05 — steps 5 and 6 verified by the person

The person drove MHW directly and reported that camera, movement and position
track between the two games: Minecraft follows MHW's rotation in the same
direction with no inverted axis, and its view moves relative to the starting
Minecraft viewpoint instead of jumping to MHW's absolute coordinates. F8
re-anchors from the current vanilla Minecraft camera and re-orients both the
player and the camera, so the fresh-anchor half of step 6 passes too. The two
halves the agent could only prove separately — that Minecraft applies the pose,
and that the native placement tracks rotation without invalidating — are now
covered end to end by a person seeing the link behave.

The run was performed with Minecraft parked on a menu, because Minecraft
periodically takes input away from MHW. That is a workaround around window
focus, not a link defect, and it is recorded in `docs/camera-link-test.md` so
the next run does not mistake it for one.

Two operational notes from the session. The bridge died on `EINTR` with
`Error: Os { code: 4, kind: Interrupted, message: "Interrupted system call" }`
and exited instead of retrying the socket read; a restart brought both clients
back (`registered Mhw`, and `registered Minecraft` on a fresh port) with neither
game relaunched, so the recovery path in step 8 held a second time, but the
missing retry is a real defect to fix. And the layout cost of testing is now
measured: both games must share the active workspace or MHW stops rendering and
the feed falls to `1 pkt/s`, while Minecraft must stay visible above MHW's
window or its response cannot be observed — Hyprland renders only the active
workspace, and MHW re-raises itself over the pinned Minecraft overlay at
intervals.

Milestone 1 in `MODDING_PLAN.md` is recorded with the step 9 dimension change
and fifteen-minute expedition still open. Next milestone is 2, composition: one
Minecraft block inside an MHW expedition, hidden by MHW terrain at different
camera angles, after verifying the actual DXVK color/depth resources and frame
synchronization.

## 2026-10-05 — step 9 closed, camera acceptance passes

The person ran the expedition and the dimension change. `CrafterHunter.runtime.log`
anchors the placement at 11:04:54 and holds it for twenty-seven minutes with no
invalidation, then logs `Native placement invalidated: camera jumped over 25 m
in a single frame` at 11:32:25 — the camp teleport. That is the specified
explicit invalidation for an area transition rather than a loss: the stone goes
away and nothing re-anchors it on its own, so F8 is required to start again.
The Nether change was reported flawless.

The corroborating counts matter as much as the report. `latest.log` contains no
CrafterHunter exception anywhere in the run window; the only error lines are
the bridge outage at 60 per minute from 11:00 to the 11:04:32 restart, and none
after it, with no further `PortUnreachableException` through 11:36. So the link
ran the whole expedition without a single retry, and the outage recorded
earlier in this journal was confined to before the run.

Milestone 1 in `MODDING_PLAN.md` is now a pass: all nine steps of
`docs/camera-link-test.md` are recorded, with the live-game halves driven by the
person. Next is milestone 2, composition — the occlusion acceptance scene is
the gap, since the native depth-tested renderer, the identified scene depth
resource and the placement lifecycle already exist.

## 2026-10-05 — the bridge survives an interrupted read

The outage recorded earlier today was the bridge itself. A signal landed on
`recv_from`, which returned `Interrupted`, and the receive loop only tolerated
`WouldBlock` and `TimedOut`, so `main` returned `Err` and the process printed
`Error: Os { code: 4, kind: Interrupted, message: "Interrupted system call" }`
and exited. From the games' side that is just a dead peer: Minecraft retried at
60/min until the restart, MHW kept sending into the void.

`is_transient` now covers `Interrupted` alongside the timeout cases, so the
receive loop simply tries again. Every other error still returns, because a
bridge that fails loudly beats one that goes quiet during a live session. The
send path carried the same latent fault — `send_packet` and `send_bytes` used
`?`, so an interrupted datagram send would have killed the process in exactly
the same way. Sends now go through `send_with_retries`, extracted as a
closure-taking function so the retry is testable without a socket: it retries
an `Interrupted` send until it lands and returns any other failure at once.

Five tests cover the decision: interrupted, would-block and timed-out reads are
retried; broken-pipe and permission-denied are not; an interrupted send is
retried to completion; a real send failure returns on the first attempt; a
truncated datagram is still an error. `cargo test --workspace` reports twelve
passing, `cargo clippy --workspace --all-targets` is clean, and
`tools/test-fabric-camera.sh` passes. The bridge was rebuilt and restarted so
the running process carries the fix.

This was step 0 of the composition prep. Next: depth-resource selection by
this-frame freshness instead of discovery order, plus the frame-synchronization
trace, before the cutscene case is attempted.

## 2026-10-05 — freshness alone would have broken occlusion

Reading the two captures left in `render/` from today (`13166284` at 06:24 and
`14757842` at 06:51) rather than launching anything turned up the design
constraint for milestone 2's depth rule. Both hold three candidates now, not
the two the earlier table saw. In `14757842` candidates 0 and 1 were **both**
fresh (`age=1`), and `inspect-depth-capture.py` reports candidate 1 at `0.00 %`
covered — cleared every frame and never rendered into.

That matters because the naive rule the plan was heading towards, "pick whichever
depth was cleared this frame", would have selected an empty buffer. Under
reversed Z an empty depth is all zeros, `sceneDepth` then reads 0, and
`depth + epsilon < 0` is never true, so nothing is ever discarded and the stone
draws through every object in front of it — the exact failure the depth path
exists to prevent, reached by a "safer" rule rather than by the old one.
Candidate 2 shows the mirror image: `age=87757` and `age=134603` with
byte-identical percentiles across two different views, so it is stale and
frozen.

Selection needs two conditions — fresh this frame **and** known to contain
scene content — with content coming from an occasional read-back rather than
every frame, since a per-frame read-back stalls the GPU. Until that exists
`depths[0]` plus the staleness check stays, because it is the only candidate
verified to hold geometry. Recorded in `docs/depth-renderer.md` next to the
table it extends.

## 2026-10-05 — recorded cutscene, teleport, and second-area evidence

Two recordings from the person close the observable half of milestone 2, and
one of them changes what was claimable about cutscenes.

`recording_2026-10-05_12.21.00.mp4` (146.75 s) is a scripted sequence: a white
transition at 42 s, a handler cutscene at 54 s, a monster cutscene at 90 s. The
HUD reads `LIVE | 16–17 pkt/s` at `age 5–29 ms` with `seq 978 → 2458` over 84 s
— no gap, no `WAITING`, Minecraft's view following the cutscene camera
throughout. That is the camera half of the cutscene case, measured rather than
argued.

It is also the recording that shows why the *composition* half was never
observed: `CrafterHunter.runtime.log` has placement anchored at 12:19:01 and
invalidated seven seconds later by a >25 m jump, with no re-anchor before or
during the recording. With `enabled=0` the renderer does not compose — the
session's `renderer.log` has no `Composing before MHW UI` line after its init —
and indeed no cube appears in any of the twelve frames sampled. So that video
is evidence for the link and explicitly not evidence for composition during a
cutscene. Recorded as such instead of being counted as a pass.

`recording_2026-10-05_12.37.51.mp4` (13.16 s) supplies the other half in
normal play: placement anchored at 12:37:34, never invalidated during the
recording, and the stone visible as a world-anchored cube with two faces over
the camp while the HUD holds `16–17 pkt/s`, `age 1–25 ms`, `seq 12245 → 12436`.
Three frames saved to the gitignored evidence run directory. Separately, the
log shows three single-frame >25 m invalidations that day (12:19:08, 12:33:05,
12:37:23), each followed by a manual `place` and never by an automatic
re-anchor.

The capture taken at 12:35 in a second area is the "capture another area" step
the depth table asked for, and it reproduces the morning's hazard exactly:
`depth0` 99.75 % covered, `depth1` `age=1` and `0.00 %`, `depth2` `age=24924`
and `0.00 %`. Fresh-but-empty is a property of the resource, not of one area,
and `depth2` having no content here at all (4.47 % in the morning) means
"known content" must be re-checked periodically rather than fixed at discovery.

Milestone 2 is recorded as passing on these checks. What stays open is honest
and small: the cube was never seen *on screen during* a cutscene, the
depth-selection rule still needs its fresh-and-content design, and
frame-synchronization evidence has not been traced. Composition reads MHW's own
GPU camera constants each frame, so camera ownership is not the risk; the
depth candidate is.

## 2026-10-05 — the cube during a cutscene, finally observed

The gap the previous entry left open is closed by
`recording_2026-10-05_12.48.56.mp4`: 101.31 s of dialogue cutscene, subtitles
at 10 s, 66 s and 86 s, with the stone live the whole time. Placement anchored
at 12:48:52 and the only invalidation is a >25 m jump at 12:50:51 — fourteen
seconds *after* the recording stopped — so `enabled=1` for every frame of it.

Four sampled frames show the cube world-anchored inside the cutscene: behind a
close-up character at 10 s, two faces behind the Handler at 34 s with her
drawn in front of them, behind the white-fade shot at 50 s, and among three
talking characters at 86 s with a foreground character occluding it. At 66 s it
is simply out of frame, which is what a world-anchored block should do when the
cutscene camera looks elsewhere.

The occlusion is the part worth writing down. The cutscene's own characters
draw *in front of* the cube, so the depth resource bound during the cutscene
held the cutscene's geometry — the fresh-but-empty candidate that the morning's
captures made the leading selection hazard did not fire here. Selection still
needs its rule; this says the risk is real but not constant.

Link quality through the sequence: `seq 14706 → 15217 → 15557` measures 16.0
and 17.0 packets/s over the two intervals, agreeing with the `17–18 pkt/s` on
screen, `age` between 7 and 57 ms, `LIVE` in every frame that has a HUD at all.
A 101 s scripted sequence with no stall.

Eight `Native placement anchored` lines span the recording 4–21 s apart.
`render/place.request` has exactly one writer, `tools/control-mhw-renderer.py
place`, and no request file was left behind, so each anchor came through that
sanctioned path rather than from anything re-anchoring on its own.

Milestone 2 is now recorded as passing **with the cutscene case included**;
`MODDING_PLAN.md` and `docs/depth-renderer.md` updated. What remains on this
milestone is design work rather than observation: the depth-selection rule
(fresh *and* known content, re-checked periodically — `depth2` held content in
one area and none in another) and the frame-synchronization trace. Next phase:
terrain and player queries for milestone 3.

## 2026-10-05 — depth selection implemented, frame-sync trace added

The two follow-ups that were recorded as design work are now code.

**Selection** moved into `native/mhw-renderer/selection.hpp`, which carries no
D3D types on purpose so it can be tested with the host compiler. A candidate
binds only when it matches the backbuffer, was cleared to 0 (reversed Z), is at
most one frame old, **and** a read-back has shown it holds geometry. That last
condition is the one the morning's captures demanded: `14757842` and `2089775`
both held a candidate at `age=1` reading `0.00 %` covered, so freshness alone
would have bound an all-zero buffer and drawn the stone through everything.
An unmeasured candidate is not bindable at all, so on startup composition waits
for the first read-back instead of defaulting to index 0.

Content measurement is deliberately cheap and non-blocking: one copy in flight,
`Map` with `D3D11_MAP_FLAG_DO_NOT_WAIT` retried on later frames, a 90-frame
timeout, only for candidates that are fresh right now, and only on first sight
then every 300 frames. The re-check interval matters because content is not a
property of a resource for ever — `depth2` held 4.47 % in one area and 0.00 %
in another. Threshold is 2 % of a whole-frame sample grid, chosen to err
strict: too strict hides the stone, too lenient draws through.

`bash tools/test-depth-selection.sh` compiles the header with `-Werror` and
runs 13 checks — freshness boundaries, the reversed-Z clear, target mismatch,
the fresh-but-empty case, the threshold from both sides, discovery-order
fall-through, corner content still counting as content, and every failure mode
resolving to "skip this frame".

**Frame sync** is a bounded diagnostic rather than a permanent cost:
`python tools/control-mhw-renderer.py framesync` arms 60 composed frames, and
each one logs the bound depth with its coverage and age, an FNV-1a hash of the
GPU host view-projection of the very draw being composed (so a frozen camera
buffer shows up as `same` across frames that should differ), and the block
centre projected twice — once with those GPU constants and once with the
CPU-side view-projection handed in for the same frame. The pixel difference
between the two is the CPU/GPU sync error. `tools/inspect-framesync.py` judges
a trace and exits non-zero if a frame bound no depth, a bound depth was older
than one frame, the camera could not be read, or the projections disagree by
more than `--max-delta` (default 2 px); its failure paths were checked against
synthetic good and bad traces.

Renderer rebuilds clean under mingw with `-Wall -Wextra`. Gates run: 12 Rust
tests, the fabric camera checks, and the new depth-selection tests, all green.
Still outstanding is the live half — deploy the DLL, run the framesync trace
against the running games, and judge it.

## 2026-10-05 — depth selection and frame sync validated against the live game

The rebuilt DLL was deployed through the development reload path — previous
build backed up to `~/.local/share/crafterhunter-backups/renderer-20261005T131415/`,
pinned executable hash re-checked first — and the plugin picked it up at
13:14:16, one second after the reload request.

The new selection rule then showed its two behaviours in the running game,
which is what the whole change existed for.

**It fails closed.** The first composed frame logged
`no eligible depth candidate (1 observed; unmeasured, stale or empty) -
composition skipped`, then at 13:16:17, two seconds later, `depth[0] content
56.94% holds geometry (age=0)` and `depth select=0`. Nothing binds until a
read-back has proven the buffer holds geometry — no defaulting to index 0, and
no draw-through while the measurement is pending.

**It refuses a fresh empty buffer.** At 13:17:58 a second candidate appeared
(`depth[1] 1920x1080 format=39 bind=72 clear=0.000`) and was measured
`content 0.00% empty (age=0)`. That is the hazard from this morning's captures
— fresh, correctly cleared, holding nothing — occurring on its own in live
play, and the rule declined it. It never appears in a selection line.

The frame-sync trace was armed repeatedly across a minute of camera movement.
`python tools/inspect-framesync.py` reports PASS (exit 0) over 958 frames:

- every frame bound a depth, `depth=0` throughout, `age=0` throughout, no
  `depth=-1` anywhere;
- 37 distinct GPU camera hashes with 36 flagged `changed`, and the projected
  anchor moving from `(960.0, 540.0)` to `(1060.5, 504.4)` across 35 distinct
  positions — so the camera was demonstrably live rather than stuck;
- `delta` 0.000 px on all 958 frames including the moving ones: the CPU-side
  view-projection handed to `CH_Frame` for a frame is bit-identical to the GPU
  host matrix of the draw composed for that frame.

Rate context worth keeping: 28.9 fps averaged across the run, 37.7 fps with
MHW's window focused, and roughly 1 fps while MHW sat on an inactive
workspace. That last number is why the games must share the active workspace —
it is the same rule the packet-rate observation recorded in milestone 1, now
measured on the renderer side.

Evidence archived as `20261005T1321-framesync-trace.log` (958 frames,
gitignored). Both milestone 2 follow-ups are closed; `docs/depth-renderer.md`
carries the full table and `MODDING_PLAN.md` the updated status. The trace is
bounded — once disarmed, `frameSyncRemaining` hits zero and no further GPU
read-backs happen.

## 2026-10-05 — milestone 3 reconnaissance: what the host actually offers

Started milestone 3 by finding out what the pinned build will and will not do,
before writing any of the query path. Findings are recorded in
`docs/host-queries.md`.

**The player side needs no reverse engineering.** Reflecting the installed
`SharpPluginLoader.Core.dll` (2.0.0) in a scratch process — metadata only,
nothing executed inside the game — confirmed `IPlugin.OnUpdate(float)` as a
game-thread tick, `Entities.Player.MainPlayer` carrying `Model.Position`,
`Rotation`, `Forward`, and `CollisionPosition`, `SingletonManager.GetSingleton`,
and `Memory.PatternScanner.FindFirst(pattern, cache)` with `NativeFunction<..>`
for typed calls. Camera sampling already runs on that same tick.

**There is no loader API for a terrain raycast.** SPL's
`SharpPluginLoader.Core.Collision` namespace turned out to be attack and
hit-detection data (`AttackParam`, `CollGeomResource`, `HitZoneResource`), and
`MtGeometry` describes shapes attached to a model. Stage terrain has to be
queried through the game's own collision routine.

**The executable's code section is encrypted on disk.** `.text` is 48,304,640
bytes at entropy 8.00/8.00 with zero `48 89 5C 24` prologues — about eleven
would appear by chance in random data of that size — and `40 53`, `E8`, and `C3`
counts exactly at their chance expectation. The section has no instruction
structure at all.

This is worth recording honestly because it cost a wrong turn: the first tool I
wrote, `tools/inspect-mhw-signatures.py`, scanned the file for the seven
terrain-ray signatures and reported all seven MISSING in 0.8 seconds. I assumed
my matcher was broken and went to debug it. The matcher was fine; the bytes
really are not code. Offline signature verification is impossible on this build,
so the tool was replaced by `tools/verify-host-build.py`, which verifies what
can be verified and exits non-zero when any of it fails:

- the installed executable is the SHA-256 pinned in `MODDING_PLAN.md`;
- `.text` is opaque, which is the reason the scan happens in the loaded image;
- SharpPluginLoader's runtime address cache resolves `Player:FindMasterPlayer`
  to `0x141B42010`, identical to the address the MIT-licensed
  `justbustin/minecraft-crossover-bridge` published for build 421810, inside
  `.text` — the runtime layout of this executable matches the build those
  terrain signatures were written against;
- SharpPluginLoader's pattern cache already holds two plugin signatures it
  resolved in the loaded image on this build, both inside `.text`, so the scan
  path the adapter depends on demonstrably works here.

The adapter rules that follow are stricter than "call the same function":
signatures in one adapter type as byte patterns, fingerprint before resolving,
any miss disables terrain queries for the session, game-thread execution only,
a fixed query budget per tick with dropped-and-reported overflow, and "no
terrain" published instead of a stale answer while the singleton, player, or
stage is missing. The player proxy maps by relative displacement for the same
reason the camera link does: MHW world coordinates sit hundreds of metres from
Minecraft's origin.

No game file was modified and nothing was executed inside the game process.

## 2026-10-05 — milestone 3: the host player proxy, end to end

The player half of milestone 3 now runs from the game thread in MHW to the
player in Minecraft.

**Protocol.** `PlayerState` (kind 11) is seven `f32`: world position in metres
and the model rotation quaternion, mirroring `CameraState`. The Rust crate
encodes and decodes it and pins the layout with golden bytes, because C# writes
it and Java reads it and neither compiler checks the other's work.

**Plugin.** `SamplePlayer` runs inside `OnUpdate`, the same game-thread tick as
the camera, at the same 20 Hz. No player is a normal state rather than an
error: a loading screen or an area transition produces no sample, therefore no
packet, and the guest ages the stream out instead of holding a position that no
longer exists. Only a managed API failure stops player sampling, and it stops
the player alone - camera telemetry keeps running. The bridge needed no change,
since it already forwards every game-to-game packet.

**Guest.** `PlayerMixin` runs at the tail of `LocalPlayer.tick()`, so vanilla
finishes its own movement for the frame first; the proxy then places the player,
sets facing, zeroes velocity, and clears accumulated fall distance so physics
cannot apply a second move for one the host already made. F9 toggles the link.
The release path is the same code with no work to do: stale feed, disabled
toggle, or world change makes `PlayerLink.update` return null and the mixin
touches nothing, so movement returns to the keyboard on that tick.

Two decisions are worth stating rather than leaving implicit.

*Relative displacement, not absolute coordinates.* MHW's world sits hundreds of
metres from anything in a Minecraft world, so the first fresh sample anchors
both the host position and the player's current position, and later samples
apply their difference: one metre of hunter travel becomes one block.

*The 25 m single-frame jump rule, reused.* The placement anchor already treats a
single frame that moves more than 25 m as a scene change rather than motion. The
proxy does the same: such a jump re-anchors it where the player stands instead
of sweeping the player across the world to catch up.

**Refactors the change forced, both with their suites green afterwards.** The
camera's quaternion-to-yaw conversion moved to `Rotation`, shared with the
player, because two copies of that math would drift the first time one is
corrected. The freshness and arrival rules moved to `SampleFeed`, so camera and
player age samples by exactly one rule while remaining independent streams -
the player check asserts that a stale player sample stays stale while the camera
feed goes live, and vice versa.

**Checks.** Rust: 10 protocol tests including golden bytes and rejection of
wrong length, non-finite values, and an unassigned kind. Java: a new
`PlayerLinkTest` covering anchor, movement, yaw wrapping across +/-180, the jump
re-anchor, every release path, world change, payload decoding, and feed
independence; the camera suite still passes after both refactors. The two
headless scripts now read one shared source list so they cannot drift. The
mixins compile against the mapped 26.2 jar through `./gradlew --offline
compileClientJava`, which is the gate that proves `PlayerMixin` and its
injection target are real.

**Not done.** The terrain half of milestone 3 has not started. The proxy has not
yet been accepted in the running games: the yaw convention in particular is
derived from the camera's proven conversion and has to be watched against the
hunter's actual facing, because a mirrored proxy would still look plausible in
motion. My first run of the new suite failed on an expectation of mine, not the
code: identity wraps to -180, not +180.

## 2026-10-05 — player proxy accepted, and the bridge that was eating it

Minecraft was restarted with the rebuilt jar (40,805 bytes, previous jar saved
to `~/.local/share/crafterhunter-backups/minecraft-mod-20261005T1412/`), the
plugin hot-reloaded at 14:11:51, and both guarded reads armed at 14:12:03. The
guest connected on launch with no discarded-packet errors, so the deployment
looked complete.

**It was not, and neither end could tell.** The bridge binary had been started
at 12:20, before `Kind::PlayerState` existed, and the protocol rejects an
unassigned kind, so every player packet died in the bridge:

```
discarded invalid packet from 127.0.0.1:39442: UnknownKind(11)
```

5,138 times, at 20 Hz, while the plugin logged `First guarded player read
succeeded` and Minecraft logged nothing at all. The camera kept working — its
kind was already known — so the proxy simply never arrived and the HUD stayed
on `WAITING`. The only evidence was in the bridge's own log.

`cargo build --offline --workspace` recompiled the bridge and probe, and a
restart on the same log file re-registered both endpoints (`registered Mhw`,
`registered Minecraft`) with zero discards afterwards; the guest reconnected at
14:36:01 after one `PortUnreachableException` retry. This is the operational
half of "the bridge needs no change": the source is forward-compatible with new
kinds, a running binary is not. The line to add next time a kind is introduced
is in `docs/host-queries.md`, next to the status claim where it will be read.

**Verdict from the person:** camera and movement between the two characters are
precise and accurate, the same judgement that closed milestone 2, and they
asked to proceed to the next phase.

**What is still open, deliberately recorded rather than folded into the pass.**
The one reading of the proxy as *reversed* was taken with the camera link (F7)
off. That matters because F7 is what normally overwrites the view from the MHW
camera every frame: with it on, the model quaternion never reaches the screen in
first person, and with it off the vanilla camera follows `LocalPlayer.yRot`, so
the view *is* the model-quaternion path. The person attributed the reading to
the camera link being off rather than to the conversion, but the clean test —
F7 off, F9 on, turn the hunter 90 degrees and confirm the character turns the
same way — has not been run, nor has F9 mid-motion. Both stay in the
verification table in `docs/host-queries.md`.

**Why the agent could not take the screenshot itself.** MHW runs floating on
the same workspace, above Minecraft and re-grabbing focus, so `grim` captures
and an injected F2 both landed on MHW. Hyprland here dispatches through Lua:
`hyprctl dispatch 'hl.dsp.focus{workspace=hl.get_workspace(3)}'` switches
workspace, `hl.get_windows()` returns window objects, and `hyprctl eval` can
write files with `io.open`, which is how the window geometry was read. Raising
the game or lowering MHW is the person's layout, so the HUD reading stays with
them until they say otherwise.

## 2026-10-05 — terrain stage A: the game's own segment cast, behind a fingerprint

Milestone 3's terrain half was a design and nothing else. It now has an
adapter, `native/mhw-spl-plugin/TerrainAdapter.cs`, that resolves the game's
segment cast in the loaded image and proves it on the host.

**Getting the routine's contract back.** The seven terrain signatures were
known to exist only as bytecode: `tools/inspect-mhw-signatures.py` was never
committed, and its `__pycache__` entry was the only surviving copy. Marshalling
that `.pyc` back and disassembling the module gave the seven name-to-pattern
pairs and their published addresses. All seven match the MIT prior art's
constants exactly (`sCollision::CheckSegment` at `0x14231AC00`, the `Param`
block's three routines, `TriangleInfo`'s three), which is what makes the
pairing trustworthy: an off-by-one pair would have disagreed. The prior art's
`raycast()` supplied the rest — `Param::ctor(param, 0x7FFFFFFF, 0x3FFFFFFF, 0,
0, 0xA, 0, 1, 1, 0, 1)`, `param[0xF9] = 0`, two `0x140` caller-owned buffers
zeroed and 16-byte aligned, eight segment floats `start.xyz 0 end.xyz 0`, flag
`1`, hit position at `tri+0xC0`, normal at `tri+0xB0`, attribute through
`TriangleInfo::attr(tri, 0)`, and `reset`/`dtor` before the buffers go out of
scope. `docs/terrain-query.md` records it in full.

**The calling convention, answered from the running process.** SPL's
`NativeAction`/`NativeFunction` call through `delegate* unmanaged` handles, so
the first question was whether a managed call can reach game code at all under
Proton: the game's PE uses the Windows x64 convention, while a Linux CLR would
emit the System V one. `/proc/4959/maps` answers it — the game's process maps
`coreclr.dll` out of
`steamapps/compatdata/582010/pfx/drive_c/Program Files/dotnet/shared/Microsoft.NETCore.App/8.0.12`.
The runtime inside the prefix is the Windows build, so its function-pointer
calls already match the game, and the loader's own types are the right door.
Calling one is an unsafe call, so `AllowUnsafeBlocks` is now on in the plugin
project and the unsafe surface is three methods: `TerrainAdapter.Tick`,
`TerrainAdapter.CastDownRay`, and `Plugin.OnUpdate`, which exists only to reach
the ray. `Pattern.FromString` was round-tripped in the scratch reflection tool
against both the loader's 2.0.0 and the 1.0.0 package the plugin compiles
against, confirming `??` wildcards parse and garbage throws.

**The adapter.** `OnLoad` hashes `MonsterHunterWorld.exe` from the process
working directory (the game root, confirmed by `readlink /proc/4959/cwd`),
compares it with the pinned SHA-256, resolves all seven patterns with
`PatternScanner.FindFirst(pattern, cache: true)`, and allocates the aligned
buffers. The first failure latches `Disabled` with its reason and no tick can
move it back. `OnUpdate` ticks the adapter *before* the bridge gates anything,
because a terrain answer is a host fact and should not wait for a guest that is
still connecting; it publishes the state before it casts, so a loading screen
reads *unavailable* rather than the previous ray's answer. One ray per tick,
one-second burst of twelve self-checks then a five-minute heartbeat, each line
carrying the ray height, `CollisionPosition`, the model origin, the normal, and
the attribute. `CollisionPosition`'s meaning is not assumed: if it is the feet
rather than the capsule centre, the delta shows it instead of hiding it. Every
native call is wrapped, so a failure disables terrain for the session instead
of escaping into the game process.

**A gate that was missing.** The plugin's headless tests
(`native/mhw-spl-plugin/tests`) existed but nothing in the gate ran them.
`tools/test-spl-plugin.sh` does now, and the terrain policy went into them:
every signature parsed through the loader's own `Pattern.FromString`, the
down-segment geometry, the agreement tolerance including its boundary and its
refusal to judge non-finite numbers, and all seven branches of the state
machine. A typo in a byte pattern fails there instead of quietly disabling
terrain in the next session.

**Not done, and not hot-reloaded yet.** The protocol half (a request kind, a
result kind, and a bounded queue for requests arriving from the guest), the
guest half (mapping Minecraft coordinates through the proxy anchor), and the
live run are all still ahead. The adapter has deliberately not been loaded into
the running game: a native call into the game's collision routine is the first
thing in this project that can take MHW down if the recovered ABI is wrong, and
it should go in on purpose, with someone at the keyboard who can stop it.

## 2026-10-05 — the terrain self-check became a request, not a heartbeat

The adapter committed in `f572a79` cast a down-ray on a one-second burst as soon
as the world was ready. Reviewing it before deploying: that puts the first
native call into the game's collision routine about a second after a world
loads — during a cutscene, a quest transition, or with nobody watching — and it
would have fired unattended in a session that had not asked for terrain at all.
It was never hot-reloaded, so no game was ever exposed to it.

The rule is now explicit. `TerrainAdapter.Tick` publishes the state every
sample and casts **at most one ray, only when a request is pending**, and the
request is a file next to the plugin's log:
`nativePC/plugins/CSharp/CrafterHunter/terrain/check.request`, written by
`tools/control-mhw-terrain.py check` and removed by `clear`. The file is deleted
before the ray runs, so a request fires exactly once; a request that arrives
while the singleton or the hunter is missing is refused in the log with the
state that refused it, rather than silently discarded. This is the same
request-file pattern the native renderer already uses for `place` and `clear`,
and it has the property the acceptance run actually needs: the cast happens
when a person is standing on the ground they want measured, which is also the
only way to measure a slope and compare it with flat ground.

`TerrainRequest` holds the channel and the test project covers it: an absent
request produces nothing, a pending request is taken exactly once, a cleared
request never fires. The per-tick budget is unchanged in spirit and tighter in
fact — one ray per one-second sample, requested — and stage B's guest queue will
spend that same budget rather than a second, larger one.

## 2026-10-05 — the terrain ray answered, live, on demand

Plugin 0.3.4 was installed into the running game at 15:51 and the first native
call into MHW's own collision routine went through without taking the game
down. Everything below came from four requested casts and the log they wrote:

```
15:51:18.231  Terrain adapter ready: all 7 signatures resolved against the pinned executable.
15:51:18.783  Terrain state changed to Ready.
15:51:59.803  Terrain self-check 1: hit=1 agree=True rayY=-3.821m collisionY=-3.471m
              delta=0.350m positionY=-3.821m normal=(-0.33, 0.89, -0.32) attr=1048576
15:52:36.803  Terrain self-check 2: (identical)
15:52:42.805  Terrain self-check 3: (identical)
15:52:48.806  Terrain self-check 4: (identical)
```

**Resolution works as designed.** All seven patterns resolved in the loaded
image in about a second, alongside the executable hash, with no address ever
written to the log. `SingletonManager.GetSingleton("sMhCollision")` returned a
live pointer on the first sample, so the state machine went straight to `Ready`
— which matters more than it looks: had the singleton name been wrong, the
adapter would have stayed `Unavailable` and never cast anything at all.

**The ray hits what the hunter stands on.** `rayY` and `positionY` match to the
millimetre, so the segment lands exactly on the surface under the hunter's feet
and the model's origin is that surface. This is the question milestone 3 asked,
answered on the host with no protocol and no Minecraft restart.

**`CollisionPosition` is not ground height.** It sits 0.35 m above the hit —
consistent with a collision reference point above the contact rather than the
contact itself, and exactly the kind of assumption that would have quietly
skewed the guest's ground height by a third of a metre in stage C. The adapter
logged ray, collision point, and model origin as three separate numbers for
this reason; had it logged only an `agree` verdict, the offset would have looked
like a passing test.

**The sample is indoors.** The screenshot put the hunter inside the Astera quest
house, standing on slanted wooden architecture — the normal is a unit vector
about 27 degrees off vertical, which is a plank, not terrain. Stage geometry,
platforms, and terrain share one collision system, so the cast is valid
evidence about the routine and not yet about terrain. Two outdoor samples, one
flat and one on a slope, are still owed.

**A note on how it went in.** Nothing cast on its own: the four rays came from
`tools/control-mhw-terrain.py check`, one request at a time, and the game
survived all of them with the bridge and both endpoints still registered. The
periodic version that was committed first would have fired a native call the
moment the world became ready, which is why it was replaced before it was ever
loaded.

## 2026-10-05 — terrain on the wire: kinds 12 and 13

The protocol half of milestone 3's terrain stage is now defined, and both
payloads have golden bytes pinned by tests, because the plugin will decode one
in C# and the guest will write the other in Java and neither side is written
yet — this is the moment to fix the layout rather than after.

`TerrainRequest` (12, guest to host) is a `u32 id` and six `f32`: `start.x y z`
then `end.x y z`, 28 bytes. `TerrainResult` (13, host to guest) is a `u32 id`,
a `u8 status`, three reserved zero bytes, six `f32` for position and normal, and
the game's `u32` surface attribute: 36 bytes. Endpoints are metres in host
coordinates, because the guest is the only side that knows where it anchored —
the host cannot answer a question asked in Minecraft coordinates. A guest that
maps through its own proxy anchor keeps the question and the answer in one
space, the same reason the camera and player payloads carry raw host
coordinates instead of pre-mapped values.

Three statuses, and the difference between them is the whole point:
`0 no terrain`, `1 miss`, `2 hit`. *No terrain* has to be an answer, not an
absence, or the guest's freshness timeout would be the only way to learn about a
loading screen. *Miss* is a real answer and must be distinguishable from having
no answer, because a guest that conflates them holds its last hit straight
through a wall; it therefore carries a zeroed position so it also cannot be read
as a hit at the origin. Decoding refuses a wrong length, a non-finite float, an
unknown status, and non-zero reserved bytes, matching the header's own reserved
bits.

Two of the new tests caught my own mistakes rather than the code's: `-4.0` is
`0xC0800000`, not `0xC0000000`, and a non-finite `attribute` is just a large
`u32` — the attribute is not a float and cannot be probed that way. Both were
caught because the layout was pinned as bytes rather than described in prose.

Still nothing sends either packet: the host half (accept requests from the
guest into a bounded queue, cast within the per-tick budget, answer) and the
guest half (pace requests, map coordinates, feed collision) are next, and the
bridge has to be rebuilt and restarted before either can work — the lesson from
`UnknownKind(11)`.

## 2026-10-05 — the host answers terrain requests

The host half of stage B is in. `TerrainRequest` arrives on the endpoint thread,
where nothing native may happen, and is queued for the game thread: eight deep,
a ninth dropped and counted with a report every 64, so a guest that outruns the
host loses answers rather than making the game wait. The tick samples at twenty
hertz and casts at most two rays per sample, which drains a full queue in about
two frames of budget instead of stalling one. A request that lands while the
state is not `Ready` is answered `no terrain` straight away — the whole reason
that status exists, rather than letting the guest time out holding an old hit.
Answers queue separately at thirty-two, where the slow side is the endpoint
rather than the guest, and drop the newest.

`TerrainPacket` is the wire layout in C#, and the plugin's tests pin the same
bytes the Rust crate pins: the same constants read in one language and written in
the other, plus the refusal of a short request, a request with a NaN in it (never
handed to the game), and a miss that must carry its id and status and nothing
else.

`TerrainRay.DownSegment` and `Agrees` moved to metres, because the adapter now
takes and returns metres everywhere and only the cast itself works in MHW units.
That is the same class of mistake as reading `CollisionPosition` as ground
height: a scale factor that leaks into a boundary is a bug waiting for a
multiplier nobody expected.

**Why the wire path is not proven yet.** The obvious test is the probe, which
already speaks the protocol. It cannot be used: the bridge keeps one endpoint
per source and routes only Mhw↔Minecraft, so a probe would have to register as
`Minecraft` and take the guest's slot, and the real mod only registers on its
Hello at startup — the camera and player links would stay dead until Minecraft
was restarted. Probe-sourced packets are not routed at all, so it would not see
its own answers either. Proving the wire path with the real guest is stage C, and
the bridge has to be rebuilt and restarted first or kinds 12 and 13 will vanish
into `UnknownKind`.

## 2026-10-05 — three surfaces, and a normal that is not a normal

Two more requested casts, flat dirt in Astera's base camp and a dirt path in the
Wildspire Wetlands, completed the ground-truth comparison across three surface
classes. The hit height agreed with the hunter's own height every time — to the
millimetre indoors, within 6 mm outdoors — and `CollisionPosition` came back
0.350 m, 0.344 m and 0.350 m above the hit. Three surfaces, three offsets inside
6 mm of each other: that is a property of `CollisionPosition`, not of the ground,
and stage C has to use the ray's own hit for ground height.

**The finding worth the trip.** On the Wetlands path the hit normal came back
`(-0.00, 0.01, 0.00)` — magnitude 0.010 — while the two earlier surfaces
returned unit vectors, and its surface attribute was `0x00104000` against `0` for
dirt and `0x00100000` for the wooden platform. Three samples, same value, so it
is not noise from one cast: either that surface class leaves the triangle-info
normal unfilled, or the hit resolved against a collision volume instead of a
triangle. Nothing available from outside the process can tell those apart, and
it does not matter, because the consequence for the guest is the same either
way — **a normal cannot be assumed to be unit length.** Normalizing a vector
that short amplifies rounding noise into an arbitrary direction, and a slope
feature built on that would walk the player off a surface that is actually flat.
Stage C's rule is now written down: below a threshold, treat the normal as
absent and treat the surface as flat.

The self-check line prints `|n|` because of that, and this is the reason to
care about a log line's shape. `(0.00, 0.01, 0.00)` reads as a direction — "the
ground faces almost straight up" — to anyone reading it quickly, including me,
which is why it took a second look to notice that the vector has no length. The
hit count came back on the line too for the same reason: one hit means the game
found a surface, and without it a zeroed normal is ambiguous between "no hit" and
"hit with nothing to report".

## 2026-10-05 — the normal is a value, not a direction, so slope comes from heights

The person asked whether a cave in the Wetlands was good enough as a slope
sample. The right answer was not to look at the screenshot, so the self-check
stopped being a single ray.

**Why.** A fifth surface, a cave floor with attribute `5`, reported the
*identical* normal `(0.00, 0.01, 0.00)` that the Wetlands path had reported
minutes earlier — same value, different place, different attribute. An unfilled
buffer reads as zero; three surfaces do not agree to two decimals on a sentinel
that is not zero. Whatever the game is doing there, it is reporting a value, and
that value is not a direction. With two of five surfaces carrying a real normal
and three carrying this, any slope feature built on normals would work on flat
ground and fail on a hillside, which is the worst possible failure: correct until
the player walks uphill.

**So slope is measured, not read.** A check is now a sweep of three columns —
the hunter's own, and one metre either side along their facing — and the report
gives the rise per metre between the outer two. Three rays at two per sample, so
a sweep spans two ticks and stays inside the per-tick budget in both. The centre
column keeps its comparison against `CollisionPosition`, which has now come back
0.350, 0.344, 0.350, 0.236 m on four surfaces: not a constant, and not a
ground height either way.

On that cave floor, three times in a row: `right[1m=36.533m] left[-1m=36.551m]`,
so `slope=-0.01 rise/m`. Flat — on a floor with a visibly inclined wall metres
away. The measurement settled a question the screenshot could not, and it will
settle the next one the same way: the person will know they are on a slope
because the number moved off zero, not because either of us judged a picture.

`SlopePerMetre` returns NaN rather than a number when a column found no surface
or the spacing is zero. A slope averaged across a missing sample is an invented
number, and an invented number in a collision path is how a player ends up
standing on nothing.

## 2026-10-05 — a 22-degree hillside, and the ground-truth row closes

The person found the steepest slope they knew in the map and stood on it. Three
sweeps, identical:

```
centre[0m=27.351m] right[1m=27.722m] left[-1m=26.923m] | slope=0.40 rise/m
```

0.40 rise per metre is about 22 degrees, 0.8 m of height across the two-metre
span. All three columns carry the same attribute, so it is one smooth incline
and not steps wearing the same number — and all three report `|n| = 0.010`,
which is the whole reason the sweep exists. The centre column agreed with
`CollisionPosition` to 0.350 m, the same offset it shows on flat ground, so the
hunter's feet are on the surface the ray finds even at 22 degrees.

Milestone 3's ground-truth question is answered: the routine is callable, it
lands on the ground the hunter stands on, it does so at 22 degrees as cleanly as
on the flat, and the two things it needed to tell us — that `CollisionPosition`
is not ground height, and that the normal is not a direction on most surfaces —
were both found by measuring rather than by reading the documentation.

Five surfaces, three of them reporting the same non-normal, one measurement
method that does not need it. The next stage is the guest half: a heightmap of
column heights around the player, mapped into Minecraft coordinates through the
proxy anchor, with the bridge rebuilt and restarted first so kinds 12 and 13
survive it.

## 2026-10-05 — the plan was aimed at the wrong direction, and it was my fault

The person asked when Steve would be walking around in Monster Hunter. I
answered with milestone arithmetic, said no milestone delivered it, and then said
the honest thing: the plan's own line 41 had already said *"a Minecraft-controlled
playable mode is a later feature and must not have both games drive the player."*
I had read that line several times while building three milestones against it and
never once asked whether "later" was acceptable.

The goal was always **Minecraft playable inside MHW**: you are Steve, MHW's world
is the world around you, an iron sword damages a real monster, a monster's claw
damages Steve, a shield refuses it, TNT works. The plan instead built the inverse
— a player proxy mirroring the hunter, monsters proxied *into* Minecraft — and
milestone 4 as written would have moved an MHW monster into Minecraft, which is
the opposite direction to the one thing worth wanting.

**What it cost:** plan text, not code. The expensive 90% was already aimed
correctly. Camera link into MHW's camera, Minecraft pixels composited into MHW's
frame with validated occlusion, host terrain queries, the fingerprint and
signature discipline, the protocol with golden bytes, the evidence discipline —
every one of those is required by the real goal. The wrong 10% is the player
proxy (now retired to its honest role as the MHW-owned spectator mode) and
region-selected block placement (superseded by a full-frame composite).

**What changed.** `MODDING_PLAN.md` now opens with the contract as testable
properties rather than a sentence of intent, and the milestones run 3 Playable
Steve → 4 Terrain under Steve → 5 Combat exchange → 6 Building → 7 One quest →
8 Story → 9 Broader interaction. Ownership is stated as the keystone: exactly one
owner of input, movement and camera at every instant, negotiated by handshake,
visible in the HUD, with the MHW-owned mode kept intact because scripted
sequences need it.

**And the risky item is now first.** Milestone 3 opens with a spike —
compositing a *full* Minecraft frame at native resolution and 60 fps with the
player model visible, instead of one depth-selected block. Everything after it
depends on that working, it is the only item in the plan with a genuinely
unknown cost, and it is cheap to find out. `MODDING_PLAN.md` also gained a
"What could still kill this" section, naming the full-frame composite, writing
damage into MHW, elytra disagreeing with MHW's own physics, and quest state, so
none of them is discovered halfway through.

The lesson worth keeping: a plan that defers the keystone will be followed
faithfully and still arrive somewhere else. Ask what the keystone is before
milestone three, not after milestone six.

## 2026-10-05 — the spike: can Minecraft's frame be read at all?

Milestone 3 opens with the one item whose cost nobody could honestly estimate, so
it goes first and alone: read a rendered frame out of Minecraft, on demand, and
look at it.

Minecraft 26.2 changed the shape of this. There is no `Minecraft.mainRenderTarget`
any more and no `Framebuffer`; the game's own renderer moved to an abstract GPU
layer — `com.mojang.blaze3d.textures.GpuTexture`, `RenderTarget.getColorTexture()`
— behind `RenderSystem.getDevice()`, with a `GpuDeviceBackend` per backend. The
old `glReadPixels` plan would have been written against an API that no longer
exists.

The good news is that the new layer has the right primitive built in:
`CommandEncoder.copyTextureToBuffer(texture, buffer, offset, Runnable, mip, x, y,
w, h)`, with a mapped-buffer read via `GpuBuffer.map`. That is an async
texture-to-buffer copy with a completion callback, backend-agnostic, on the
render thread — no PBO plumbing, no `glFlush` guesswork, and it does not care
whether the backend turns out to be OpenGL or Vulkan. So the readback is queued
on one frame and read on a later one, and the render thread never blocks on a
full-frame download.

`FrameCapture` polls `<gameDir>/crafterhunter/frame.request` four times a second
and does nothing at all unless a request is there; the file is deleted *before*
the copy runs, so one request means one run even if the game is busy. Requests
are `capture [frames]` with the count capped at 600, parsed by `FrameRequest` —
the same parser the Python tool's output has to satisfy, so the two cannot drift
into a request that is silently ignored and looks like a failed capture.

`FrameLayout` holds the contract the native side will trust, and it is where the
one real subtlety lives: a GPU readback arrives with row zero at the **bottom**
on an OpenGL backend and at the **top** on Vulkan. Published frames are therefore
flipped to top-down once, in the guest, and the format is named in the meta line
(`rgba8-topdown`) rather than assumed by whoever reads it next. A consumer that
guessed would show MHW's sky at the bottom of Steve's world on one driver and
the right way up on the other, which is the kind of bug that gets blamed on the
renderer.

Each capture also writes a PNG next to the timings, and that PNG is the actual
acceptance: a frame that is upside down, from the wrong buffer, or missing its
geometry is obvious in a way no assertion would catch. The hook is the tail of
`GameRenderer.render` rather than the level renderer, deliberately — including
the HUD makes the readback self-checking before any native code assumes anything.

`tools/test-fabric-frame.sh` checks row order (including flip-twice-is-identity
and the one-pixel case), frame byte counts, the meta line's fields, that a zero
field does not read as absent, and every refusal in request parsing. Minecraft 26.2
needed no Fabric API at all: loader and Mixin only, and the mixin compiled
against the mapped jar.

Mod 0.3.0 rebuilt and installed (50,526 bytes, previous jar backed up to
`~/.local/share/crafterhunter-backups/minecraft-mod-20261005T172944/`). It needs
a Minecraft restart before anyone can ask it for a frame.

**Not done:** the native half. Nothing composites this yet — no shared-memory
transport, no per-pixel depth compare, no frame in MHW. The next step after a
human confirms the PNG is a correct frame is the transport, then the composite.

## 2026-10-05 — the frame capture shipped a bug that looked like a feature

The first live run of the spike accepted a request and then produced nothing.
No exception, no output file, and a summary that said `status=queued 1` — the
capture was neither working nor failing, which is the one state a request path
must never be in.

**The bug was an inverted flag, and it was mine.** One boolean stood for both
"the copy is in flight" and "the copy has landed". It was set true when the copy
was queued and cleared by the GPU callback when the copy completed, so the frame
loop published *while the copy was still being written* and re-queued the request
the instant the callback arrived. Every frame: publish nothing, queue again. The
request was genuinely accepted and genuinely never satisfied, and the only
visible symptom was a status line that had been written before the attempt.

Two flags now, because those are two different facts, and the transition between
them is a state machine: `IN_FLIGHT` waits, `READY` publishes exactly once, `IDLE`
may queue. `FrameCopyState` holds it as pure logic and `tools/test-fabric-frame.sh`
walks all three transitions plus the "callback fires twice" case, because a bug
this shape is invisible to integration testing — the request genuinely went in,
and nothing genuinely came out.

**The lesson is about the evidence channel, not the flag.** I designed the
summary file to make silent failures impossible, and then the first thing I did
with it was `writeSummary` only on request and only at the end, so the one
failure mode that mattered reported nothing. Every outcome now writes the
summary: on accept, on copy failure, on short readback, on completion. The HUD
line stopped saying "idle" for a failure too, because idle and broken are
indistinguishable from outside and the HUD is all most people will ever look at.

**Wasted time worth recording.** I burned several turns on Hyprland window-close
after being told to read the configs first. What actually closes Minecraft is
`SIGTERM`: `window.close()` reaches the game, which answers with a "Save and Quit
to Title?" dialog that waits for a click, so the window stays open and the
process never exits — which reads exactly like a broken close request. The
focus dispatch also takes an `HL.Window` object from `hl.get_windows()`, not an
address string, and the call has to go through `hl.dispatch` inside an `eval`
because the `hyprctl dispatch` shortcut only accepts a bare dispatcher
expression. `tools/mhw-control.py` now uses that form and
`tools/restart-minecraft.py` uses `SIGTERM`, both documented at the call site.

Mod 0.3.0 rebuilt (50,927 bytes) and installed. Full gate green: 19 protocol
tests, five headless suites including the new copy-state checks, gradle, plugin,
host verification.

**Not done:** still nothing composites this. No transport, no depth compare, no
frame in MHW. The next run answers one question — does a real frame come back,
and how long does it take.

## 2026-10-05 — the spike is answered: a Minecraft frame reads out, and it is cheap

Plugin and mod aside, the one number this project could not estimate: **a full
Minecraft frame can be read out of the GPU, on demand, correctly.**

30 frames requested, 30 captured, `status=done`, 1908x1028, 7,845,696 bytes —
exactly the size the contract predicts, checked by the headless suite rather than
by eye. The PNG is right way up: crosshair centred, HUD legible, hotbar along the
bottom, the ocean horizon where the horizon is, and our own `MHW LINK: LIVE` and
`PLAYER: LIVE` lines captured from inside the frame. Row order is therefore
correct on the first attempt, which is the part most likely to have needed
debugging.

**What it costs, measured rather than asserted.**

| Step | Cost |
| --- | --- |
| Read a 7.8 MB frame out of `/dev/shm` | 1.41 ms median, 3.70 ms worst of 20 |
| Write a 7.8 MB frame to `/dev/shm` | 4.7–5.1 ms |
| Frame size at 1920x1080 | 8.3 MB |
| Sustained if we ship every frame at 60 Hz | 0.50 GB/s |

So a frame crosses the boundary in single-digit milliseconds at this
resolution, and the honest reading is that **shared memory is enough** — no
zero-copy, no Vulkan external memory, no GPU handle sharing. That is the
expensive design I was going to have to write if this had gone badly, and it is
not needed.

The number that first came out was 1001 ms and it was a lie. `mapMillis` was
measuring from *queueing* a copy to *publishing* it, so it reported how long the
frame waited for me to ask for it — dominated by request pacing, not by the GPU.
It is now three numbers: `readMillis` and `flipMillis` are the work, `ageMillis`
is the waiting. Conflating them is how a cost measurement becomes a number
nobody can act on, which is the same mistake as the bridge that discarded 5,138
player packets while both ends looked healthy: a metric that measures something
adjacent to the thing you care about.

**What this settles.** Milestone 3 was restructured today around the fear that a
full frame might be unaffordable. It is affordable, so "Playable Steve" is an
ordinary engineering milestone and the order stands: transport (double-buffered,
one frame in flight, with an age), then the per-pixel depth compare that makes
occlusion correct rather than approximate, then the ownership handshake.

**The honest caveat.** 1908x1028 is not 1920x1080, and this measured a *file*
in `/dev/shm` rather than the double-buffered ring the transport will actually
use. The number will move; the order of magnitude will not, and the design
decision it drives — shared memory, not zero-copy — is safe either way.

## 2026-10-05 — the transport, and a cross-language test that earned its keep

The frame contract, stated on both sides and checked against each other.
`frame_transport.hpp` on the native side, `FrameChannel` on the guest, and
`tools/test-frame-transport-agreement.sh` compiles both and compares the
constants. Two slots, one frame in flight, a sequence number per slot, pixels
published before the header that claims them.

**The agreement test caught three real disagreements on its first run**, which is
the argument for writing it:

1. `sizeof(SlotHeader)` is 64 in C++ because of `alignas(64)`, not the 40 bytes
   its fields occupy. The guest had assumed the field count and written slot
   one's header inside slot zero's pixels.
2. `sizeof(BufferHeader)` is 128, not 64 — the `reserved[14]` array — so every
   offset downstream was wrong by half again.
3. Then, once the sizes matched: **the guest was writing big-endian.**
   `ByteBuffer.allocate` defaults to big-endian and every field was byte-swapped.
   This is the worst class of transport bug — every field present, at the right
   offset, holding the right value, and useless — and it is invisible to any test
   that only checks "did it write something".

The native struct also gained explicit field order with padding first. Leaving
layout to the compiler is fine until a second language writes the bytes by hand,
which is exactly what happened: the first C++ ordering and the first Java
ordering were mirror images of each other around the alignment boundary.

**A mutation test, because "the test passes" is not evidence.** The endianness
fix was reverted, the suite run, and confirmed to fail with the swapped values,
then restored. A suite that cannot fail on the bug it was written for is a suite
that will pass on the next one.

Field offsets are now pinned three ways: `static_assert` and `offsetof` checks in
the C++ test, byte-level reads in the Java test at the same offsets, and the
cross-language comparison. Fifteen constants agree: magic, version, format name,
bytes per pixel, slot count, both header sizes, both slot offsets, both pixel
offsets, and the buffer size for the live 1908x1028 capture.

**Also fixed while here:** `FrameLayoutTest` and `FrameChannelTest` were in the
default package while the classes they test are not, so `javac` put their
classes in two different directories and `java` could not find one of them. The
headless script runs them by their full names now, which is what stopped the
silent "Could not find or load main class" that was being misread as a test
failure.

Mod 0.3.0 rebuilt and installed. Gate green: 19 protocol tests, seven headless
suites (three new), gradle, plugin, host verification.

**Not done:** nothing reads the channel yet. The native side has the contract and
its rules, but no code maps or draws from it — that is the next piece, and it is
where the depth compare lives.

## 2026-10-05 — the compositing rule, pinned before it is written in a shader

The last piece of milestone 3's composite is the per-pixel question: given
Minecraft's pixel and MHW's depth at the same screen position, does Minecraft
draw? That is the whole difference between "Minecraft is inside MHW" and "a
Minecraft screenshot pasted over MHW", and it is written as
`frame_composite.hpp` with its tests *before* it goes into the pixel shader,
because a shader cannot be tested without a GPU and a backwards depth test still
produces a plausible-looking image.

**The comparison is reversed Z and that is the whole trap.** MHW clears scene
depth to 0 and nearer is the *larger* value, so "is Minecraft in front" is
`minecraftDepth > hostDepth`. Written the conventional way it inverts, and Steve
renders through walls — an image that looks fine in an open field and wrong in a
canyon. Both directions are asserted, because only one of them fails if the
comparison is flipped, and the mutation test confirms it: flipping the operator
fails 5 of the checks and restoring it passes them.

Two other rules came out of the same trace:

- A host pixel the pass never touched still holds the clear value, which under
  reversed Z is the *far* end. So depth 0 means sky, and Minecraft **must** be
  drawn there — otherwise a Minecraft building against the sky punches a hole in
  the world. This is the opposite instinct from "empty means discard", and it is
  why `hostHasDepth` is a separate flag rather than inferred from the value.
- Minecraft's own sky carries no information for MHW: it is Minecraft's sky, not
  the world's, and drawing it would replace MHW's sky with a flat gradient. So
  Minecraft pixels with no geometry behind them are `Unusable`, never drawn, and
  the frame's Minecraft coverage is reported so a frame that is 99% Minecraft sky
  can be refused before it costs any bandwidth.

Colour-without-depth is refused rather than drawn. Pasting it would put Minecraft
over MHW's characters; refusing it loses the frame. The failure is visible as
missing Minecraft instead of a Steve standing in front of a Rathalos.

Gate green, mutation test included. Next: the native reader that maps the shared
channel, uploads the texture, and runs this rule in the shader — which is the
first part of this that needs MHW running.

## 2026-10-05 — the native reader and the frame draw

`frame_source.hpp` maps the guest's channel once and hands out the newest
complete frame; `renderer.cpp` uploads it and draws it through the rule already
pinned in `frame_composite.hpp`. This is the piece that needs MHW running, and
everything that could be checked without it was:

- **Uploads are conditional on the sequence, not the frame rate.** The guest
  publishes at 20-60 Hz and MHW renders at its own rate; uploading 7.8 MB per MHW
  frame would cost more than the frame. A frame whose geometry changed means the
  guest resized, and the texture is rebuilt rather than stretched.
- **The two clocks are not assumed to agree.** The guest stamps frames with
  `System.nanoTime()` and this side reads `GetTickCount64`. Rather than convert
  and hope, the offset is measured once against the first frame seen and every
  later age is computed against that. Same machine, so drift is negligible;
  different machines would need a real handshake, which is stated in the code
  rather than left as a trap.
- **Minecraft's frame is centred, not stretched.** The two windows are almost
  never the same size and that is the normal case. Scaling to fit would shear
  Steve and, worse, shear the depth being compared against MHW's own, so a
  letterbox keeps the comparison honest.
- **The frame draws after the stone** and against the same depth candidate the
  stone used, so the two never disagree about what is in front within one frame.

`frame_source.hpp` is Windows-and-POSIX in one header, with the refusals stated
rather than at the call site: not mapped, shorter than its headers, no complete
frame, stale, or a frame claiming more space than the mapping holds. That last
one is the half-finished-resize case, and walking off the end of a mapping is the
sort of bug that reads as corruption somewhere else entirely.

Compiling it needed three fixes that are worth recording because two were mine:
`FrameView` and `SlotHeader` are in `crafterhunter::frame` and I wrote them
unqualified inside `crafterhunter::frames`; `munmap` takes a mutable pointer and
the view was declared `const void*`; and the member `newestFrame` shadowed the
free function `newest`, so an unqualified call inside the class would have
resolved to the member. The DLL builds at 20:11.

**Not verified:** nothing here has run inside MHW. The upload path, the shader,
and the letterbox mapping are all unexercised. Gate green across nine suites and
the mingw64 cross-compile, but the first run of this in the game is the real
test, and it is the step after this.

## 2026-10-05 — the gate that made the frame path unreachable

The first live attempt produced nothing, and the reason was mine rather than the
platform's. The frame composite was called from inside the stone's draw path,
which begins:

```cpp
if (ownDraw || ctx != context.Get() || (!traceDraws && (!drawEnabled || composed))) return;
```

`drawEnabled` only goes true once the plugin has a *placed anchor*. The MHW
restart cleared it, so every draw returned immediately. The frame composite could
not run without a stone placed — a debug aid silently deciding whether the player
was allowed to exist. The log showed depth candidates discovered but never
measured and no pipeline line at all, which is the signature of this and nothing
else.

Three changes, in the order they had to happen:

- **`selectSceneDepth()` is now shared.** The stone and the frame ask the same
  question and must get the same answer, or a block and a Steve in one frame
  could disagree about what is in front of them. It is cached per frame because
  the content measurement behind it can stall the GPU, and running it twice in
  one frame would pay twice for the same number. It is also, deliberately, not
  gated on the stone.
- **`ensureSharedState()` owns the fixed-function objects** that both paths bind —
  the context-state slot, the screen-size constants, the sampler, the rasterizer
  and the no-depth state. They were created inside `createPipeline()`, which only
  the stone calls, so the frame had nothing to bind even on the draw path that
  worked. All five have identical parameters for both draws, so two sets would
  only be two things to keep in step.
- **`createFramePipeline()` is called on first use by `ensureFramePipeline()`**,
  which fails once and reports once, instead of being chained off the stone's
  creation. A machine without shader model 5 should see one line, not one per
  frame.

The order inside `drawFrameComposite()` is deliberate and worth stating: pipeline,
then pixels, then depth. Uploading before the depth is known means a frame
arrives even in a frame where no depth is usable, so the next one has something to
show immediately. Minecraft still draws after the stone, so a block placed inside
Minecraft's geometry stays visible rather than being buried — the order decides
only who wins where both write.

`CH_Stop()` now releases the frame objects and unmaps the channel, which it did
not before; a reload would otherwise have left a stale mapping and a live texture.

**Not verified:** still not run inside MHW. This is the second attempt, and the
first failed for a reason that no host-side test could have caught.

## 2026-10-05 — two lost entry points, and the gate that should have caught them

The composite still did not draw, and this time the log said exactly why:

```
[32818087] FrameVS compile: CrafterHunter(70,10): error X3000: syntax error: unexpected token '('
[32818087] Minecraft frame pipeline unavailable; frame composite disabled
[32902979] VS compile: CrafterHunter(70,10): error X3000: syntax error: unexpected token '('
[32902980] Pipeline unavailable; drawing disabled
```

Line 70 was a verbatim duplicate of line 69, and there was no `VS` function in the
file at all. Both came from one careless edit: its `oldString` was the whole `VS`
function and its `newString` reused that body for `FrameVS` while also ending with
the `float4 PS(...)` line that already followed. So one edit renamed the stone's
vertex shader into `FrameVS` and duplicated `PS`. Nothing noticed for three
deployments because nothing on the host ever looked at the shader — the renderer
only logs its own compile failures, and only once someone is standing in an
expedition waiting for Steve to appear.

`tools/test-shader-source.sh` now compiles every entry point the renderer asks
`D3DCompile` for, with the names read out of `renderer.cpp` rather than listed in
the script, so a new entry point is validated without editing the test and cannot
be forgotten. Structural checks (brace balance, functions defined twice) run
whether or not a compiler is installed. glslang is not D3DCompile and will not
accept everything it does, so this is a floor rather than proof: it catches the
structural damage a text edit causes, which is what it is aimed at.

Mutation-tested both ways, since a gate that does not fail is worse than none:

- duplicating `PS` again → 5 checks fail, structural check catches it too;
- renaming `VS` away → `VS vs_5_0 FAILED / Entry point not found`, exit code 1.

Exit codes were checked directly rather than through a pipe, having first been
masked by `tail` in the mutation run — a gate whose failure is invisible in a log
is a gate nobody trusts.

## 2026-10-05 — X4500, and the depth the transport does not carry

Third deployment, third distinct failure, and the sharpest one yet:

```
FramePS compile: CrafterHunter(26,18-31): error X4500: overlapping register semantics not yet implemented 't1'
```

**The stone and the frame both bound `t0`/`t1` inside one source string.** `D3DCompile` parses the entire unit and rejects the file; glslang compiles each entry point in isolation and had happily accepted it. So the gate added an hour earlier did not catch this, and the honest conclusion is that it is a floor rather than proof — now stated in the script's own comment.

The two shaders are now separate string literals, `Shader` and `FrameShader`, each declaring `t0`/`t1` once.

**The deeper problem was hiding behind the compile error.** `FramePS` sampled
`minecraftDepth` at `t1` while `drawFrame` bound *MHW's* `sceneDepth` there. So
the shader wanted Minecraft's depth and would have been handed MHW's, comparing a
surface against itself and producing an image that looked plausible and was
meaningless. The pinned rule in `frame_composite.hpp` wants Minecraft's own
**linear eye depth** plus a flag for "Minecraft drew nothing here".

None of that exists. The transport is `rgba8-topdown`, `BytesPerPixel = 4`: colour
and nothing else. Per-pixel occlusion between the two games cannot be implemented
against this transport, and pretending otherwise is how a plausible-looking image
gets mistaken for a working one.

So the shipped rule is the interim one, `decideSkyOnly()`: draw Minecraft where
MHW holds no geometry, refuse everywhere else. That is the pinned rule with the
depth comparison unavailable, not a loosened version of it — `decide()` refused
colour-without-depth because *unconditional* pasting would cover the hunter, and
restricted to the host's own empty pixels nothing is covered.

**What this costs, stated plainly:** Steve appears against MHW's sky and is *not*
occluded by MHW's terrain. A tree between him and the camera will not hide him.
Minecraft's sky is indistinguishable from its geometry without depth, so the
letterbox carries Minecraft's sky with it. Full occlusion needs Minecraft's depth
attachment published and linearised — a transport change and a guest change, not a
shader tweak — and it is now written down as its own piece of work rather than
left as a comment promising it will arrive.

`decideSkyOnly()` is unit-tested with four assertions, including one that states
the gap as a test: a nearer Minecraft pixel over host geometry is `Draw` under the
full rule and `Skip` under the interim one. If that ever starts passing, depth has
arrived. Deleting the rule's refusal fails all four.

Two of my own test expectations were wrong on the way in, both caught by running
them: `DepthEpsilon` makes *equal* depths draw rather than skip, and I had the
reversed-Z direction backwards in the "nearer Minecraft" case. The rule was right
both times.

The 1x1 `minecraftDepthProbe` texture is gone. It existed only to stand in for a
binding the shader no longer reads, and leaving it would have suggested depth was
on its way without saying what it would cost.

## 2026-10-05 — the Windows mapping length, which made Steve impossible

Diagnosing the frame path before shipping it turned up three faults, each fatal on
its own. This is the first, and it is the reason nothing could ever have appeared
in MHW regardless of how many frames were published.

`frame_source.hpp` read the channel's length with `GetFileSizeEx` on Windows and
then **threw the value away**. `length_` was only ever assigned in the POSIX
branch, so on Windows it stayed `0`, and `newestFrame()` opens with:

```cpp
if (length() < headerBytes() + SlotCount * slotHeaderBytes()) { ... refuse ... }
```

`0 < 256`, so **every frame was rejected as "channel shorter than its headers",
forever.** No log line, no crash, no way to tell from the game that the reader was
alive and refusing. The comment directly above the discarded read even said the
length "is the only size that can be trusted before a header has been read".

No host test could have caught it, and that is the part worth keeping: Linux
compiles the POSIX branch, so the branch that was wrong is the one the test suite
never built. A test of the *arithmetic* would still have passed, because the
arithmetic was correct — the value it was given was not.

So the fix removes the failure instead of repairing it. On Windows the length is
now obtained from the mapping itself via `VirtualQuery`, so there is no cached
length in that branch to forget:

- `channelLongEnough()` is extracted as a pure rule and tested, including that a
  zero length is never sufficient and that one byte short of the headers is
  refused;
- the refusal now distinguishes `mapping length unknown` from
  `channel shorter than its headers`, because a zero means the mapping's size
  could not be established, which is a different failure from a truncated file;
- an empty file is refused at `open()` on both platforms. The guest creates and
  fills the channel in one constructor, so an empty one means we caught it
  mid-creation, and mapping it yields a zero-length section whose every read
  fails — a far more confusing symptom than "not there yet".

One test asserts the *shape* of the Windows branch: that it exists, that it asks
the mapping for its length, and that it contains no assignment to the stored
length. Mutation-tested: reinstating the cached `length_` fails both of those
assertions by name, and weakening the length rule fails the boundary check.

**Build passes; nothing here has been seen in-game yet.** That distinction is the
subject of the next entries.

## 2026-10-05 — the timestamp conversion, and why the clocks cannot be compared

The second fault, and the one that had been hiding behind the first.

The conversion was wrong by a factor of ten thousand:

```cpp
GetTickCount64() * 1'000'000ull / 10'000ull   // milliseconds -> ?
```

GetTickCount64 counts **milliseconds**, so milliseconds to nanoseconds is a
multiplication by one million. That expression divides as well and yields
milliseconds times one hundred — a number far too small to compare against
anything. `fresh()` opens with `if (capturedNanos == 0 || nowNanos < capturedNanos)
return false;`, so every frame looked like it had been captured in the future and
every frame was refused.

Worse, the offset that was supposed to rescue this could not be learned: it was
measured only *after* a frame passed the freshness gate. With clocks that genuinely
disagree, no frame passes the gate, so the offset is never learned and no frame is
ever accepted. The two requirements are contradictory.

**Measured rather than assumed.** The guest stamps frames with `System.nanoTime()`
and this process reads `GetTickCount64`. Reading a live channel:

```
guest stamp is 3422486.7 ms behind CLOCK_MONOTONIC
```

Fifty-seven minutes, which is what a JVM that counts suspend time and a kernel
whose monotonic clock stops during suspend will disagree by after a laptop has
been closed once. Not a constant, and not calibratable.

That killed the plan of correcting for the difference, and not only because of the
deadlock. Fitting an offset to a single sample makes `local - guest` equal that
offset *by construction*, so the age it reports is identically zero — for every
frame, forever. Both approaches were implemented, both were wrong, and the tests
that caught them are in `frame_clock.hpp`'s header comment so the next person does
not try them again.

**What runs instead.** Age is measured on the reader's own clock, from whether the
guest's sequence number advanced recently. That needs no agreement between the
processes and answers the question that matters — whether the world in the frame
has moved on.

The bound is deliberately loose, and the reason is measured too. The transport's
`MaxAgeNanos` is 50 ms, but publication is gated on a GPU read-back returning, and
the measured interval is **140 ms when Minecraft is busy and 1000 ms when it is
not** (`ageMillis=1000.5`, one sequence per second). A 50 ms bound would refuse
every frame ever published. `StallBoundNanos` is 3 s: it clears the slowest
observed rate with room to spare and still notices a stopped guest quickly. A test
asserts the bound exceeds both measured rates, so it cannot be quietly tightened
back to a number that refuses everything.

`chooseAgeSource()` keeps the transport's own rule for the case where clocks *do*
agree, so this is a decision made from a measurement rather than a replacement of
the contract. The chosen source is logged once with the divergence, so the log
states which rule is in force instead of leaving it to be inferred. One test
asserts the branches do not rescue each other: a fresh-looking guest stamp must not
rescue a frame that has stopped publishing.

`lastUploadedSequence` and `lastAdvanceSequence` were separated, because the upload
dedup and the liveness check are different questions and sharing one counter made
the second unaskable.

**Still unverified in-game.** This is the state of the code, not the state of MHW.

## 2026-10-05 — safe frame publication, and a test that could not fail

The third fault. `FrameChannel.publish()` wrote pixels, then a header, and never
marked the slot incomplete in between:

```java
channel.write(pixels, ...);            // slot still says published = 1
channel.write(header, ...);            // from the frame before this one
```

With two slots the *newest complete* header always names the slot that is not
being written, so a reader choosing by sequence was safe. The exposure is the
other end: `newestFrame()` hands back a pointer into shared memory and the caller
then spends milliseconds uploading 7.8 MB out of it, while the guest recycles
that same slot two frames later. Nothing re-checked, so the upload could composite
half of one frame and half of another — an artefact indistinguishable from a
rendering fault.

Publication is now three writes: mark incomplete (keeping the sequence, so a
reader can still tell what the slot used to hold), write pixels, mark complete.
The header write also loops until it is out; `FileChannel.write` may write fewer
bytes than asked and the header write was the one call that never checked. No
`force` is issued: both processes share this host's page cache, so a flush would
cost a 7.8 MB round trip per frame to achieve nothing, and every write goes
through `write(2)` so the ordering is the kernel's to keep.

On the native side `Source::stillHolds()` is asked *after* the upload whether
that is still the frame just uploaded, and a false answer drops the frame. One
frame of latency is invisible; a torn frame is not.

**The test for this took four attempts and the first three were theatre.**

- It watched a 16 KB frame and never caught the window. Too small to observe.
- It read the whole 8 MB buffer per sample, so one sample took longer than forty
  publishes and the run reported "the reader never ran".
- It waited for that reader with `Thread.onSpinWait()`, which starved the thread
  it was waiting for. On twelve cores.
- With the reader fixed it still **passed with the fix deleted**. 5430 samples,
  22 caught mid-refill, and every one of those was from the *first* publish,
  where `published` is 0 simply because the file was zeroed — not the state under
  test.

A test that passes when the bug is present is worse than no test, because it is
taken as evidence. What made it real:

- the reader now settles both slots *before* starting, so the zeroed-file state
  cannot be mistaken for the incomplete state;
- it samples the header, the first and last pixel bytes, then the header again.
  Tearing is not "pixels changed" but "looked finished, changed underneath, still
  looks finished" — which is also exactly what `stillHolds()` checks in the
  renderer, so the test and the fix assert the same condition;
- the frame is 16 MB. The window is only as wide as the pixel write, and at
  949×1028 that is microseconds — rare enough that the bug survived. A write
  lasting milliseconds makes it impossible to miss.

Deleting the incomplete write now fails with the tearing assertion. Restored, five
consecutive runs pass with 124–169 mid-refill samples, **all** of them marked
incomplete.

Two incidental finds: `FileInputStream.seek` **no longer exists in JDK 27** (the
legacy channel methods were removed), so the sampler uses positional
`FileChannel.read`. And an array element incremented by one thread and polled by
another is not guaranteed visible — the spin loop read a hoisted value forever.
Atomics, and a yielding wait.

**Build passes. Not yet seen in-game** — that is the next entry, and it is the
only one that can close this.

## 2026-10-05 — first run of the fixed reader, and a mistake of mine

The new DLL loaded and the frame path ran for the first time:

```
Shared draw state ready (context state, constants, sampler, rasterizer, no-depth)
Minecraft frame pipeline ready (colour only: sky-against composite, no depth channel)
Minecraft frame refused: no complete frame
```

Faults 1 and 2 are fixed — the length is derived, the conversion is right, and the
reader got as far as *deciding* about a frame instead of refusing on arithmetic.

**Then I broke it myself.** `verify-frame-composite.sh` deleted the channel before
requesting a capture, on the reasonable assumption that a new capture means a new
file. It does not: the guest holds the channel open for the life of the capture,
so deleting the path left Minecraft writing to an unlinked inode. The guest kept
reporting healthy publishing — `seq=30 published=1` — while no reader could ever
see it. The worst combination to debug, and self-inflicted.

The script no longer deletes it. The channel comes back when the guest reopens it,
which `reopenChannel()` only does on a **size** change, so a nudge of the
Minecraft window is enough and no restart is needed.

**The second thing the run exposed is a real gap.** Refusals were logged only when
the reason *changed*. The reader refused every frame with "no complete frame", said
so once at startup, and was then silent for the rest of the session — a log
identical to a reader that had stopped being called at all. Nobody should have to
guess that distinction from a log, so refusals now report on change and then once a
second while they persist.

A third, smaller defect in my own script: it created the evidence *parent*
directory but not the per-run one, so every `tee` failed and the run reported "no
evidence" while the capture itself had worked.

Not yet a composite on screen. Next: recreate the channel, re-run, and read the
upload line.

## 2026-10-05 — the transport rejected the host's resolution, not the frame's

The refusal the live run reported was not the clock or the mapping. It was this,
in `usable()`:

```cpp
if (slot.width != width || slot.height != height) return false;   // width/height = the HOST's
```

`width` and `height` were the *host's* 1920×1080. The guest publishes 1908×1028.
The two are never going to be equal — that is precisely why there is a letterbox —
so the check refused every frame the transport has ever been asked to carry. The
"no complete frame" line was accurate and completely misleading.

A frame's dimensions are the guest's to declare. They are now validated two ways
instead:

- **`usable()`** keeps only self-consistency: magic, both versions, a non-zero
  sequence, `published == 1`, and non-zero dimensions. A zero dimension is refused
  before any offset is computed from it.
- **`within()`** is new, and checks a slot's own geometry against the size of the
  mapping. The *sum* is tested, not either term: a slot can claim a size whose
  offset is comfortably inside the buffer while its pixels run past the end, which
  is exactly what a half-finished resize leaves behind.

Two more refusals came out of writing the tests:

- **Slots disagreeing about geometry are refused rather than resolved.** A slot's
  pixel offset depends on the buffer's declared geometry, so using one slot's
  dimensions to locate the other's pixels reads the right number of bytes from the
  wrong place. The next frame settles it.
- **Refusals carry a reason.** `newest()` now says which of the five ways it
  declined. A reader that refuses silently is indistinguishable from one that
  stopped being called, which is a distinction that has now cost this project two
  debugging rounds.

Regression-tested against the real numbers, and the mutation is behavioural rather
than cosmetic: putting the host-dimension rule back *inside* the new signature, so
the API is unchanged and only the behaviour differs, fails eight checks including
`a 1908x1028 frame is chosen by a transport that has never heard of 1920x1080`. An
earlier attempt at that mutation only changed a function signature and produced a
compile error, which proves the API moved and proves nothing about behaviour.

`newestFrame()` lost its host-resolution parameters entirely. The renderer change
is one line; the stone renderer and depth selection are untouched.

## 2026-10-05 — bounded recovery, so a stale mapping cannot outlive the channel

`uploadNewestFrame()` returned on "no complete frame" without re-opening anything.
So once its mapping described something the guest was no longer writing, it kept
describing it for the rest of the session — and nothing observable *from inside that
mapping* would ever have said otherwise. Three ways in, all with the same symptom and
the same remedy:

- the file is unlinked while the mapping is held. Not hypothetical: a verification
  script of mine deleted the live channel, and the guest went on publishing to an
  unlinked inode while reporting healthy sequence numbers.
- the guest recreates the file, at a new size after a window change.
- the guest restarts, so sequence numbers begin again at one.

`recoverChannel()` now handles all three, rate-limited to once per second by
`shouldRemap()` in `frame_source.hpp`. Bounded rather than eager because the reader
runs every frame and the channel is legitimately absent for as long as Minecraft is
closed — an unbounded retry would be a syscall storm in the most common case.

On a successful re-open it **forgets what it was holding** before the mapping goes:
`lastUploadedSequence`, `lastAdvanceSequence`, `lastAdvanceNanos` and the age-source
decision. That is what makes a guest restart safe — a sequence of 1 must not be
mistaken for the frame already on the texture — and a resized channel safe, since the
new geometry must not be compared against the old one.

The liveness path had its own remap under a different interval
(`StallBoundNanos / 3`). Two bounds for one remedy is one bound too many, so it now
calls the same routine. The recursion that introduces is bounded: recovery sets
`lastRemapNanos`, so the nested call cannot remap again, capping the depth at one.

`mayRemap()` treats a clock that has gone backwards as due, and a reader that has
never tried as due — otherwise the subtraction could never reach the interval and
recovery would deadlock permanently.

One test expectation of mine was wrong on the way in and the suite caught it: I
asserted that re-opening *is* allowed at the instant of the last attempt. It is not —
that attempt set the clock. Getting this wrong in the permissive direction is one
remap per frame, so the boundary is now asserted from both sides.

## 2026-10-05 — the letterbox was never letterboxing

Extracted from `renderer.cpp` into `frame_composite.hpp` so it could be checked
without a GPU, and immediately found to be **inverted**.

```hlsl
float2 uv = hostUv * uvRect.xy + uvRect.zw;   // what it did
```

That scales the *screen* coordinate. To draw a rectangle of relative size `scale`
starting at `offset`, the screen coordinate has to be brought *into* the rectangle:

```
guest = (host - offset) / scale
```

Scaling instead inverts the relationship. The region drawn becomes
`host ∈ [-offset/scale, (1-offset)/scale]`, and for 1908×1028 into 1920×1080 that is
**[-0.022, 1.022] vertically** — larger than the screen at both ends. So Minecraft was
not letterboxed at all. It was **cropped, about 2% lost off the top and the bottom,
and stretched to fill all 1920×1080** — the single thing the letterbox exists to
prevent, and the thing that would have silently invalidated the depth comparison the
composite depends on.

Extracting it also surfaced two smaller defects:

- **`scaleX` came out as 1.0000003** on the tight axis, which made `offsetX` slightly
  negative and mapped the host's own edge column to a uv below zero, discarding it.
  The scale is now clamped to the host.
- **A degenerate rectangle mapped everything to uv (0,0)** — which is *inside*
  `[0,1]` — so a zero-sized guest would have drawn one stray texel at the centre of
  the screen instead of nothing, contradicting the comment that claimed otherwise.
  `insideGuest()` now rejects it, and the shader checks a `frameFlags.x` validity flag
  because a NaN uv would sail past the discard (every comparison against NaN is false).

Two of my own assertions were wrong before the code was:

- I asserted the host's **left and right edges were outside** the rectangle. For these
  sizes there is no side bar — the guest is wider relative to the host than it is tall —
  so the frame spans the full width and those columns are correctly *inside*.
- I passed `offsetY` as the **horizontal** argument in three places.

Both were caught by the suite rather than by reading, which is the argument for
extracting the arithmetic rather than trusting it.

Mutation-tested: putting the scaling form back fails **eight** letterbox assertions,
including `the host's top-left corner is outside the rectangle` — which is the
assertion the bug itself made false, and which the old test asserted the other way.

The shader and the host now both derive from the one tested function, so the mapping
cannot drift between them again.

## 2026-10-06 — recovery that stopped resurrecting frozen frames

`recoverChannel()` zeroed `lastAdvanceNanos` on **every** successful re-open. But
re-opening a channel whose file still exists always succeeds — including when that file
is the *same frozen one*. So once per second the reader forgot the sequence had stopped
advancing, saw the same unmoving sequence as if it were fresh progress, and drew a
frame from a guest that had gone away. Recovery was not merely failing to help; it was
actively defeating the stall detection meant to stop exactly that, and it would have
done so indefinitely while looking healthy in the log.

The distinction needed is between a channel that is *the same writer, stuck* and one
that is *a different writer*. `classifyGeneration()` returns exactly that, from
evidence rather than from the file merely existing:

| Evidence | Verdict | Consequence |
|---|---|---|
| sequence unchanged, geometry unchanged | `Same` | keep the memory, keep refusing |
| sequence went **backwards** | `Restarted` | clear |
| geometry changed | `Resized` | clear |
| magic or version unrecognisable | `Replaced` | clear |

A *higher* sequence is deliberately `Same`: that is progress by the writer we already
know, not a new one. The regression test simulates the loop — memory, a stalled
sequence, five re-opens — and asserts the frame is still refused after each, so a fix
consisting of "never forget" would not pass it. The three genuine-new-generation cases
are asserted too, for exactly that reason.

Mutation-tested: making `shouldForget` return true unconditionally fails the test
repeatedly (`so the reader must not forget what it knows about it`), exit 1.

The reader's memory of the writer is now one struct, `FrameMemory`, holding the
sequence, the geometry and the liveness clock together. They are one fact — "what the
writer was last seen doing" — and splitting them across three globals is what let the
clock be cleared while the sequence was not.

`Source::headerValid()` was added because I first passed a *guess* for it: inferring
the header's validity from whether a frame happened to come back. That guess produces
the wrong verdict exactly when it matters, since a valid header holding nothing
publishable is the normal state before the guest's first frame.

## 2026-10-06 — the Windows length was page-rounded, and the path was a guess

Two claims about the Windows reader had never been checked, and both were wrong in a
way a host test could not see.

**The length was not the file length.** The previous fix replaced a missing assignment
with `VirtualQuery`, reporting the mapping's `RegionSize`. That is the size of the
*region*, rounded up to the allocation granularity — for a 15,691,648-byte channel it is
thousands of bytes more than the file contains. `within()` uses that number to decide
whether a frame's pixels fit, so a rounded-up length would accept a frame extending past
the real end of the file: no crash, since the mapping is larger, but pixels of nothing.

Now both platforms produce a byte count and a **single** assignment sits after the
platform split, so neither branch can bypass or omit it. `length()` has no
platform-specific variant at all, which is what removes the possibility rather than
patching the instance.

The test asserts the *shape* of the code, because a host test cannot run the Windows
branch and so cannot observe a zero length there. What it can insist on: exactly one
`length_ =` inside `open()`, positioned after the last `#endif`; no `#if` in `length()`;
no `VirtualQuery` anywhere in it. Mutation-tested both ways — reintroducing
`VirtualQuery` and dropping a path spelling each fail, exit 1.

**The path was a single unverified spelling.** The reader is a Windows process under
Proton handing `CreateFileA` a Unix-style path. Wine accepts those and maps them onto
`Z:`, but if that ever stopped being true the symptom would be "no frame arrives" —
identical to a guest that has not started. Both spellings are now tried, `Z:\dev\shm\...`
second, and **which one worked is logged** rather than left to be inferred. The test
pins that the Unix spelling is byte-for-byte the path the guest writes to (otherwise the
two processes are using different files), that the drive spelling has no forward slashes,
and that an out-of-range index falls back instead of reading past the list.

What this cannot tell us: whether Wine actually accepts the Unix spelling on this host,
or whether it needs the drive form. That is now a single log line rather than a
hypothesis — `frame channel opened via the ... spelling` — and it will say so the first
time the reader runs.

## 2026-10-06 — the guest and the reader, tested together

Fabric recreated the channel only when the window's dimensions changed. A channel that
was deleted — or replaced by another file of the same size — therefore left the guest
holding an open handle to an **unlinked inode**. It kept publishing, reported healthy
and rising sequence numbers, and every byte went to a file no reader could open. From
the guest's side everything was perfect, which is the worst combination to debug, and it
happened here because a verification script deleted the live channel.

`FrameChannel.channelLost()` now answers whether the file at the path is still the one we
are writing, and `writeShared()` recreates on a lost channel as well as a size change.
The check costs two stat calls, so it is rate-limited to once a second rather than made
per frame — a second is far faster than the failure it prevents.

Presence alone is not what is checked, and cannot be: a replaced file of the same size
reads as present and correctly sized. What reveals that is the sequence numbers going
backwards, which the native reader treats as a restarted writer. The test asserts the
recreated channel's sequence is *lower* than the one the reader last saw, because that
inequality is the reader's only evidence.

**The test that was missing is the integration one.** Each half had been tested alone,
and the fault above is invisible from either side: a predicate that correctly reports
"the file is gone" can coexist with a reader still serving a frame from an inode nothing
writes to, and a guest can report a healthy channel while the reader sees nothing. So
`tools/test-frame-recovery.sh` compiles a probe from the real headers
(`frame_source.hpp`, `frame_transport.hpp`, `frame_clock.hpp`) and has the Java guest
drive it across five stages — publish, unlink, recreate, resize, restart — asserting on
what the **reader** reports at a path the guest really writes to.

Two things came out of writing it:

- **A page-rounded length is caught by the integration test and not by the unit test**,
  because it makes the probe die rather than return a wrong number — `munmap` is then
  called with a length that was never mapped. That the failure is a crash rather than a
  clean assertion is itself worth knowing.
- **`Source::open()` gained an optional path**, defaulting to the fixed channel path. It
  is a parameter only so a test can point a reader at its own file; a runtime-configured
  path in production would be a second source of truth about where the two processes meet,
  which is the one thing this header exists to avoid.

Two of my own mutations were worthless, and the second only after checking. Removing the
`Files.exists` guard did **not** fail the test, because `Files.size` on a missing file
throws `NoSuchFileException` and the catch already returns true — so that guard is
redundant for correctness and kept only because throwing is expensive in the render
path. Mutating just its final `return` also did nothing, since the guard short-circuits
first. Replacing the whole method does fail, with the assertion naming the consequence. A
mutation that cannot fail is worth as little as a test that cannot.

## 2026-10-06 — Steve renders inside MHW. Screenshot, not inference.

First live composite. Measured, in order:

```
[2534188] re-opened the Minecraft frame channel (was absent, size 15691648): restarted, memory cleared
[2534189] frame channel opened via the Unix-style spelling (15691648 bytes)
[2534192] First Minecraft frame uploaded: seq=42 1908x1028
[2535203] Compositing Minecraft frame with MHW scene depth
```

- **Guest sequences advancing**: sampled the channel twice four seconds apart; the
  sequence moved by 58, about 14 fps, newest frame 126 ms old.
- **Upload**: `First Minecraft frame uploaded: seq=42 1908x1028`.
- **Draw issued**: `Compositing Minecraft frame with MHW scene depth`.
- **Visible**: `screenshot-live.png` and `live-a.png` / `live-b.png` in
  `build/evidence/frame-composite/`. Minecraft's ocean, its blocky cliff, the hotbar,
  the "Potion" stack and our own HUD are composited into the middle of MHW's screen,
  **and the hunter, the Palico, the minimap and MHW's control prompts are drawn over
  it** — the sky-only rule behaving exactly as specified: Minecraft fills the pixels MHW
  left empty, MHW's geometry wins everywhere else.
- **Liveness, visually**: `live-a.png` and `live-b.png` four seconds apart differ in
  99.95% of the pixels of the HUD band, so the composited content is moving. That is a
  stronger statement than any log line, because a log line only says a draw was issued.

**Two more things the run settled.**

*The Windows path question is answered.* `frame channel opened via the Unix-style
spelling` — Wine accepted `/dev/shm/crafterhunter/frame.channel` outright and never
needed the `Z:` fallback. The fallback stays, because a silent fallback from "the path
spelling Wine stopped accepting" to "the guest has not published" is precisely the
failure this file has produced before.

*The publisher stall is fixed in-game, which is the point of fix 1.* When a capture ran
out, the reader logged `same writer, memory kept` with `frame is not being published`
once a second, 168 times, and uploaded **zero** further frames. Before that fix it
zeroed the liveness clock on every re-open, so a frozen Steve would have been redrawn
every second indefinitely while the log looked healthy. A guest restart was exercised
live too — Minecraft was relaunched before this run — and the reader classified it
`restarted, memory cleared` before uploading.

**Two defects in my own verification script, both found by it disagreeing with the
screen.**

1. It reported `pipeline: NOT created` because it searched only the log since its own
   mark, while the pipeline is created once per session long before any capture.
2. It reported `upload: none` while uploads were demonstrably happening, because the
   upload line is guarded to fire once. Its absence means nothing at all.

Both now read from the session start, and the verdict states that liveness is judged by
whether the stall lines have stopped and by the screenshot.

**Still not verified: a window resize.** The channel is still 1908×1028 and no resize has
happened this session, so the `Resized` branch of the generation classifier has live
evidence only from the integration test, never from the game. Dragging the Minecraft
window is all that is needed, and it is the last thing between this milestone and
"verified".

**Unrelated and also broken: the bridge is down.** Minecraft's HUD reads `MHW LINK:
AWAITING | 0 pkt/s` and the log shows `PortUnreachableException`, so the camera and
player-proxy links are not running. That is the UDP bridge, not the frame path, and
nothing in this milestone depends on it — but the HUD is reporting two dead links and
that should not be left looking healthy.

## 2026-10-06 — measuring the link instead of reading it off a screenshot

The bridge was down: no process, nothing on UDP 38470, and Minecraft's log full of
`PortUnreachableException`. The binary was newer than its source, so no rebuild was
needed; it is started detached with `setsid` so it survives the shell that launched it.
Both endpoints registered within five seconds and the plugin reported:

```
Connected to crafterhunter-bridge/v1
Bridge HelloAck received; camera reads are armed after the startup grace.
First guarded player read succeeded.
First guarded camera read succeeded.
Received actual minecraft:stone pixels and PNG from Minecraft.
```

**Reading that off a screenshot would have been the wrong instrument**, twice over. The
HUD sits exactly where Minecraft's own F3 debug screen does, so the first attempt to
photograph the telemetry got the debug screen instead — which was itself useful, since it
reports the guest build as 1.27.8.1 / 27.8.1. And a composited image cannot say which
frame it came from, which is the whole question.

So `LinkStatus` writes the same numbers the HUD draws, plus the camera pose, to
`crafterhunter/out/link.txt` at 4 Hz, and `tools/sample-link-status.py` samples it. The
pose is recorded because it is what alignment is measured *against*: yaw span, pitch span
and position travel over a window say whether synchronized movement is even possible
during that window, and a static camera would make any such demonstration meaningless.
Failures are swallowed deliberately — instrumentation that can interrupt the frame it
measures is worse than none.

**A test of mine was silently environment-dependent.** `checkUnmappedRefuses` opened the
*fixed* channel path to prove that opening a missing channel fails. With Minecraft
publishing, the "unmapped" source mapped a live channel and read a real frame, and two
assertions failed for reasons that had nothing to do with the rule under test. It now
opens a path that cannot exist. This is the second time a test of mine has depended on
the machine's state rather than the code's, and it is worth stating as a rule: a test
whose result changes because a game is running is not a test.

## 2026-10-06 — what depth capture would actually take, checked rather than assumed

Before promising item 3, the three things that could have made it impossible were
checked against the deobfuscated 26.2 client jar rather than guessed:

1. **Is the depth attachment reachable?** Yes. `RenderTarget` exposes
   `protected GpuTexture depthTexture` with public `getDepthTexture()` and
   `getDepthTextureView()`. `MainTarget` inherits it. So depth is the same kind of
   object the colour read already uses, and `CommandEncoder.copyTextureToBuffer` is
   generic over `GpuTexture` — the call proven for colour takes depth unchanged.
2. **Is the current injection point wrong for this?** Yes, and now known rather than
   suspected. `FrameCaptureMixin` injects at `GameRenderer.render(...)` TAIL, which is
   *after* GUI compositing. That is why our own HUD text and Minecraft's F3 debug screen
   both appear in the composited frame: they are in the texture being copied. A
   world-only capture wants `GameRenderer.renderLevel(DeltaTracker)`, which is public and
   ends before the GUI pass.
3. **What about hands?** `renderItemInHand(CameraRenderState, float, Matrix4fc)` is
   private and called from inside the level path, so the first-person hand is drawn
   *within* `renderLevel` and no injection point in that method excludes it. That is a
   real constraint rather than a scheduling detail: "world colour and depth before
   hands and HUD" needs either a hook inside the hand call itself or acceptance that the
   hand is part of the world pass. Which of those is right depends on whether item 5's
   isolated scene is played first- or third-person, and that has not been decided.

So depth capture is feasible, not blocked. What is *not* established is the depth
format `allocateDepthAttachment` picks, the linearisation from that format plus
Minecraft's projection, and the normalisation against MHW's reversed-Z — items 4 and 6,
which are the parts where a wrong answer produces a plausible image rather than an
error.
# 2026-10-06 — bounded capture-lifecycle repair (headless)

Single-attachment completion now uses a capture-owned completion object; format
metadata no longer dereferences unrequested textures. Partial completion times
out too. Timeout or setup/copy failure disables new capture until process restart,
retaining the existing buffer pair rather than freeing/reusing uncertain GPU
resources. This is a conservative diagnostic policy, not a depth-backend fix.

The world verification script now emits `colour 1`, `depth 1`, or `capture 1`
rather than the invalid `capture colour` command. Commands are atomically renamed
into place and the consumer claims them before reading/deleting. AGENTS.md records
the goal, actual Git directory, working checkpoint, and next acceptance gate.

Verified: tools/test-fabric-frame.sh (including new completion checks), Fabric
`./gradlew build`, and Git diff whitespace check. No games launched, no deployment,
no runtime tests. GPU completion still does not establish valid copied pixels;
D32_FLOAT readback, row layout and paired depth acceptance remain unresolved.

# 2026-10-06 — the D32 readback refusal, measured instead of argued

The live log line that stopped every depth attempt was `GL_INVALID_FRAMEBUFFER_OPERATION
in glReadPixels(incomplete framebuffer)` at 04:18:57. Which recipe produced it was
settled against the deobfuscated 26.2 jar, not guessed: `GlCommandEncoder.
copyTextureToBuffer` binds the source with `bindFrameBufferTextures(readFbo, texture, 0,
mipLevel, GL_READ_FRAMEBUFFER)`, which puts the image on `GL_COLOR_ATTACHMENT0` and 0
in the depth slot. A D32_FLOAT image on a colour attachment makes the FBO incomplete,
`glReadPixels` is refused with 1286, and the backend then throws
`IllegalStateException("Couldn't perform copyTobuffer ...: GL error 1286")` *after*
`queueFencedTask` — so an earlier comment claim that a refused copy "raises nothing in
Java" was wrong and the comment is corrected where it survived.

`tools/gl-depth-readback-probe.c` (run by `tools/test-depth-readback.sh`) answers with
a real driver instead of a build. On both vendors available here — the default NVIDIA
RTX 2050 and, forced through glvnd, the AMD Radeon 660M / Mesa 26.2.4 that Minecraft
itself logs — the game's recipe gives `GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT` plus
`GL_INVALID_FRAMEBUFFER_OPERATION`, while the corrected recipe (depth on
`GL_DEPTH_ATTACHMENT`, `glReadBuffer(GL_NONE)`, completeness checked before reading)
returns the written values exactly. The same probe pins the row contract the capture
metadata had wrong: rows are tight `width*4` bytes, row 0 is the framebuffer's *bottom*
row, and a 4x larger destination changes neither — so capacity is not stride, and the
old `depthRowsPadded = capacity > width*height*4` could only ever be false because the
buffers are allocated exactly `width*height*4`. `glReadBuffer(GL_DEPTH_ATTACHMENT)` is
`GL_INVALID_ENUM`; `glGetTexImage` works as a fallback.

Consequently the depth half leaves the shared copy path: new `DepthReadback` (GL state
saved and restored around `GlStateManager`'s cached framebuffer bindings, completion
queued behind the fence, fail-closed on refusal) driven by the pure contract in
`DepthReadbackPlan`, with `DepthReadbackPlanTest` pinning row stride, written bytes,
row order (`bottom-up`), and depth convention (`gl-window-depth`, never compared
directly to MHW reversed-Z). `WorldCapture` metadata now reports pack row length,
pack alignment, row stride, written bytes, capacity *as capacity*, rows padded, row
order, convention, and the read status/error name.

Verified headless: `bash tools/test-depth-readback.sh` (both drivers),
`bash tools/test-fabric-frame.sh`, Fabric `./gradlew build`. Not verified: any live
capture — that needs both games running. The fail-closed policy (permanent disable
after timeout or setup failure, buffers retained) is unchanged; what changed is that
the refusal now arrives as an exception on a path that checks its own framebuffer
first, so the cause is named instead of inferred.

## 2026-10-06 — the pack state the read inherits, enumerated instead of assumed

The depth read set two pixel-pack parameters (row length, alignment) and left the
rest to whatever the context already held. Two of the untouched ones are silent:
a leftover `GL_PACK_SKIP_ROWS`/`GL_PACK_SKIP_PIXELS` moves the read deeper into the
destination — past the end of a buffer sized exactly to the frame — and a leftover
`GL_PACK_SWAP_BYTES` byte-reverses every float on the way in. Neither raises a GL
error; both destroy every value. The incoming state is now read first, reported as
`packNonDefault=` in the capture metadata, set to explicit values (skip 0, no swap,
no lsb-first, alignment 4, row length = width), and put back in the `finally` block
alongside the framebuffer and pack-buffer bindings. `DepthReadbackPlan.nonDefaultPack`
is the pure, tested half: it names each deviation from the GL defaults, in order,
with its value.

`tools/gl-depth-readback-probe.c` grew a section that starts from a deliberately
hostile pack state (`rowLength=17 alignment=1 skip=5,7 swap=on lsb=on image=3/2`),
then measures both contracts from it: the old row-length-and-alignment-only read
comes back **corrupted** — values wrong and data written past the end of the frame,
which is the regression this test exists to expose — while the guarded read returns
the written values exactly, writes nothing outside the frame, and restores all eight
pack parameters plus both bindings (verified against recorded hostile values and a
dummy FBO/PBO, since restoring zero to zero proves nothing). The two image parameters
are hostile too but deliberately not guarded: if they affected a 2D `glReadPixels`,
the guarded read would fail — so their surviving untouched is the measurement that
they are ignored.

`WorldCapture` metadata additionally records `colourRowOrder=bottom-up` (same GL read
origin as depth, so the two raw buffers are comparable) and `handInFrame=` derived
from the recorded perspective — the first-person hand is drawn *inside* `renderLevel`,
so excluding the HUD does not exclude the hand. The evidence PNG is now flipped to
display order (the probe proved row 0 is the framebuffer's bottom row); the raw
`.rgba` keeps GL order to stay in agreement with the depth buffer.

`tools/verify-world-capture.sh` now archives every run under
`out/archive/<UTC stamp>/`: the previous run's leftovers are copied before the first
request deletes the meta, each publish is copied the moment it lands (only the files
that mode actually wrote), and the run's GL/mod log lines are captured on exit — so
a later failure cannot overwrite an earlier success.

Verified headless: `bash tools/test-depth-readback.sh` (both drivers: NVIDIA RTX 2050
and Mesa AMD Radeon 660M, section D passing on each), `bash tools/test-fabric-frame.sh`
(pack-state assertions included), Fabric `./gradlew build`, `bash -n` on the gate
script. Deployed: `~/.minecraft/mods/crafterhunter-fabric-0.3.0.jar` is byte-identical
to the build (`sha256 9a0c2d53…d4cd9d159`, `cmp` clean); the previous jar is preserved
in `~/.minecraft/crafterhunter/old-mods/`.

Not verified — the live gate itself. One launch attempt (reconstructed from the
SKLauncher logs) brought the game up in-world but *without Fabric*: no
"Loading … with Fabric Loader" line, no CrafterHunter output, frame hook never fired;
the instance exited cleanly and its log is kept at
`/tmp/opencode/my-launch-logs/latest.log`. The launcher's injection evidently only
works under the launcher, so the live capture (colour-only, depth-only, paired, one
block at known distances, camera sync LIVE, screenshots) is handed to the user to run
with both games. Unchanged: the fail-closed retention policy, the shared-memory
transport, and the MHW compositor.

## 2026-10-07 — projection recorded from the camera; the gate blocks on scene depth

The projection metadata was NaN because the old chain read
`GameRenderer.levelProjectionMatrixBuffer.lastUploadedProjection`, a field the level
upload path nulls by calling `getBuffer(Matrix4f)` — structurally dead, so the numbers
were never there to record. `CameraProjectionAccessor` (a new `@Accessor("projection")`
on `Camera`, validated by the mixin AP at build) reaches the matrix the camera actually
holds; `WorldCapture.recordCamera` now writes `fov`, `zNear`, `zFar`, the 16 matrix
floats and `projectionSource=camera-projection`, and both dead accessors plus their
mixins.json entries are deleted. Measured live: `fov=54.37698`, `zNear=0.05`,
`zFar=1024.0`, and a matrix that is exactly the zero-to-one reversed family —
`m22 = n/(f-n) = 4.883051E-5`, `m32 = nf/(f-n) = 0.05000244`, `m23 = -1` (both computed
from the recorded planes to every printed digit). Committed `affc2b6`, pushed.

`tools/validate-world-depth.py` now *detects* the depth convention by trying both
closed forms against the recorded matrix instead of assuming one, and blocks reporting
`worst relative` for each when neither fits; `tools/test-validate-world-depth.py`
(4 fixtures) is the regression that fails on the pre-fix validator at the reversed
case. The live gate then exposed the next gap honestly: on this matrix the validator
BLOCKED with both conventions near `1.0` worst-relative, because the referee
`distance_from_matrix` hardcodes the classic `zw = (ndc+1)/2` window remap while the
measured buffer is `zw = ndc` (ZERO_TO_ONE). For this family the `reversed` closed form
`d = nf/(zw*(f-n) + n)` is algebraically exact — it reconstructs the recorded corner
values correctly — so the referee, not the closed form, is wrong. That fix needs a
fixture carrying the measured matrix, which is the regression test this bug deserves.

The gate run itself (`out/archive/20261006T172918Z/`, both games LIVE, camera ~17
pkt/s) passed everything that does not depend on scene depth: COLOUR, DEPTH and PAIRED
all published with both attachments complete, no GL errors, `packNonDefault=rowLength=949`,
exit 0, per-run archive so nothing overwrote run 1. The HUD was deliberately *on*
during capture — F3 debug, the CrafterHunter overlay and the hotbar are all visible on
screen and all absent from `world-colour.png` (HUD excluded, verified by comparison);
the crosshair and the held item are present in the capture and are recorded as such,
not treated as exclusions. `perspective=FIRST_PERSON`, `handInFrame=included` as
before.

BLOCKED stage, with fresh evidence: **the terrain is not in the depth attachment.**
95.0% of the depth buffer (928,016 / 976,572) is exact clear `0.0`. The only non-zero
content is (a) a hand-shaped rectangle in file rows 0–~320, cols 757–948 — screen
bottom-right under the recorded bottom-up order, `zw` 0.069–0.082 → 0.61–0.72 m, and
(b) a ~10x12 px blob at rows 512–523, cols 466–475, `zw ≈ 0.0498` → 1.00 m — the
crosshair, which writes depth inside the captured pass. The crosshair pixel therefore
reconstructs 1.00 m although the camera sits 3.5666 m above the ground plane and its
view axis must meet ground near 4.6 m, and `depthMax=0.08206147` is identical to the
previous run's — camera-attached geometry, not scene. Colour, meanwhile, is complete
(99.99% non-zero, full terrain). So the colour image is being obtained through a path
that carries the world, and the depth read is against an attachment that only the held
item and the crosshair ever draw into; where the terrain's own depth lives (the
intermediate target its colour is composited from) is the investigation for the next
round, before any transport or compositor change.

Known-distance ground truth was not forced: F3 is bound (`key.debug.overlay:
key.keyboard.f3`) and readable, but the player's ray has no Targeted Block within
reach, and the feet height (`y=128.00000`) is not confirmed at the camera's crosshair
hit point — a plane intersection from `anchor`+`anchorRot`+matrix would be computed
against the same projection under test. The distance claim waits for scene depth and
an independent ground height. Orientation likewise stays open: the validator reports
`depthRowOrder` as the writer's claim because gradient statistics are identical under
both orientations; the colour-based decision is not implemented.

Verified headless: `./gradlew build`, all `tools/test-*` (frame, depth,
frame-selection, world-depth 4/4), `bash -n tools/verify-world-capture.sh`;
`~/.minecraft/mods/crafterhunter-fabric-0.3.0.jar` byte-identical to the build
(`sha256 89743ae88c7fe3820cff8da499e20490df9d900dd92114323ab308513cdf5f0b`), previous
jar preserved in `~/.minecraft/crafterhunter/old-mods/`. Verified live: the archive
above — three modes published, projection recorded, HUD excluded from colour, camera
LIVE throughout. Not verified: scene depth (blocked, evidence above), distances
(validator BLOCKED, referee gap), the convention-vs-matrix match on the real matrix
(needs the measured-matrix fixture), colour/depth orientation as a *measurement*, and
the known-distance check itself.
# 2026-10-08 — bounded native skin upload/binding investigation

The live serialized 64x64 RGBA skin matches the cached user skin exactly after
vanilla SkinTextureDownloader alpha normalization: CRC32 ab329927, 29 distinct
RGBA values. This establishes Java readback/serialization, not GPU sampling.
No production texture bug is established. Fabric, channels, geometry, pose,
anchor and depth behavior are unchanged.

Added opt-in render/player-skin-check.enabled: once per asset, checksum the exact
CPU upload bytes and a staging CopyResource readback (skip RowPitch padding), log
dimensions/DXGI format/pitches/UV bounds, then query actual PS t0 and PlayerPS
identity immediately before drawing. No test texture or image dump. This small
GPU readback may block once; disable the flag after diagnosis. It does not alter
acceptance guards or shader output. GPU equality/binding and live appearance are
NOT yet proven: games were closed and were not launched by the agent.

Passed player model real normal/slim baked tests, Java/native roundtrip, CRC32
known-vector/padded-row regression, player controller/source contracts, all eight
shader entry points, native Release build, world reprojection/alignment,
depth selection, world-frame transport/freshness and cargo workspace (26 tests).
Installed only native DLL, build/install SHA256
9c22b7615c9fd8d6239bff55467a272b979ad7d997b07f825d75a6a495df2360;
previous DLL retained in
/home/yllaris/.local/share/crafterhunter-backups/native-skin-check.GJDQOW.
Diagnostic flag enabled before next launch; no saves/worlds modified.
# 2026-10-08 — diagnostic-only player UV milestone

Inspected actual local 26.2 ModelPart.Cube/Polygon/Cube.compile bytecode: UVs are
normalized by texture dimensions; transforms apply only to XYZ/normals and pass
the baked U/V unchanged. Mirroring/face corner pairing is already baked. Existing
row-zero texture upload/readback and sampling preserve V; framebuffer-origin
rules do not imply a skin flip. No production UV change justified.

Added opt-in UV gradient PlayerUVPS (retains production depth/UI/alpha rejection)
and separate deterministic 64x64 debug skin net, selectable with
tools/control-player-uv.py uv|net|real. Real asset bytes/cache remain untouched.
Each part/face carries independent color channels, blue vertical gradient and
white corner marker. Debug selection does not alter geometry, pose, anchor,
transport, calibration or depth policies. Default installed mode is real.

Passed normal/slim actual baked 12-part/72-face UV rectangle and V-direction
tests, player native/Java roundtrip and debug pattern tests, controller toggles,
all nine shader entry points, native Release build, world reprojection/alignment
and depth selection. Live gradient/net orbit and final skin appearance NOT yet
accepted; games were closed, not agent-launched.
Installed only native DLL SHA256
d4986afc486ac713c6eaa527f0a59d7416d0c72e676f69945ea81fd5d42ee67d;
backup /home/yllaris/.local/share/crafterhunter-backups/player-uv-debug.ZBWs0E.
# 2026-10-08 — host-depth pixel-coordinate investigation, not acceptance

Suspected displaced occlusion after UV/skin gate. Production uses direct integer
SV_Position.xy mip-0 depth Load; player viewport starts at zero with target size,
selection requires matching depth size, direct R32_FLOAT SRV, no copy/resolve or
UV normalization. Current source establishes identity texel mapping, not matching
MHW depth-writer viewport/image. No measured offset, root cause or production fix.

Added opt-in depth-position/depth-source debug shaders and mode controller. They
expose exact normalized sampled texel centres/raw-depth visualization on player
surfaces; host rejection bypass is debug-only, never production acceptance.
Metadata captures target/depth/pre-UI/selected depth-writer viewport plus four
posed-mesh projected bounding-box probes using actual GPU camera constants.
Initial depth-writer measurement can be absent: switch modes after several frames.
No geometry, pose, anchor, skin, transport or production depth changes.

Passed native build, all 11 shader entries, actual normal/slim player tests and
Java/native roundtrip, debug controllers, pixel-centre math across resolutions,
depth selection and reprojection/alignment regression. Runtime measurements and
controlled occlusion acceptance still pending. MHW closed but Minecraft remains
open; scoped installation deferred until requested safe shutdown confirmation.
