# CrafterHunter agent contract

## Goal and current checkpoint

The goal is real Minecraft gameplay inside MHW on Linux/Proton: Minecraft owns
Steve/items/building; MHW owns its monsters, quests and world. Exactly one game
must own movement/input at a time. Combat and quest integration are NOT implemented.

Verified checkpoint: Minecraft colour-frame transfer and diagnostic sky-only
composition. This is not isolated Steve rendering or depth-correct guest geometry.
Preserve the working stone renderer and its validated scene-depth selection.
Read README.md, MODDING_PLAN.md, docs/depth-renderer.md and the latest MODLOG.md
entries before changes; distinguish plans from measured evidence.

## Next acceptance gate

Validate same-frame Minecraft world colour/depth capture before changing native
composition. WorldCapture currently fails closed permanently after a timeout or
setup/copy exception, retaining at most its existing buffer pair until process exit.
This deliberately prevents uncertain GPU work from causing buffer reuse/free or
unbounded allocation. Restart is required to retry. Do not weaken this policy
without verified backend completion and resource-lifetime semantics.

Callbacks signal completion, not valid pixels. Investigate the actual OpenGL
D32_FLOAT readback failure. Validate projection/depth/row packing against a real
Minecraft block at known distances. Buffer capacity is not row stride. Match
attachments and camera metadata by capture identity. Do not directly compare
Minecraft linear depth to MHW reversed-Z depth.

Capture now hooks the optional LevelRenderer always-on-top depth clear, with a
once-per-frame GameRenderer pre-hand-clear fallback when that pass is absent.
Run `python3 tools/test-world-capture-hook.py` against the local Loom 26.2 jar
when changing this boundary. `captureBoundary` records the issue-time hook.
This fix is headless-checked, not runtime-accepted; obtain fresh nonzero terrain
depth and known-block distance evidence before advancing the acceptance gate.
Update: manual bedrock captures on 2026-10-07 passed the two-distance gate
(about 4.895 and 12.122 blocks), both at the pre-hand-clear fallback boundary.
The optional always-on-top boundary and linked-camera scenes remain unaccepted.
Next transport is request-paced paired snapshots; see docs/world-frame-transport.md.
An opt-in upload-only native diagnostic now has local freshness and bounded
generation/identity selection (docs/world-frame-transport.md); it is not bound
to native composition or runtime-accepted. Keep diagnostic sky-only and
stone rendering unchanged until native paired composition has its own evidence.
Follow `docs/world-depth-acceptance.md` for the two real-block measurements.
Use `tools/validate-world-depth.py --acceptance`: diagnostic exit zero alone
does not accept a milestone. New captures include colour/depth SHA-256 hashes
binding the artifact bytes to the issue-time metadata identity.

## Work discipline

- One bounded change at a time; regression tests must expose the original bug.
- No render-thread spin waits or pumping global tasks without verified semantics.
- Request writers publish complete files by atomic same-directory rename; consumers
  must claim a command before reading/deleting it. Never delete the live frame channel.
- Builds prove compilation, not visible rendering, synchronization or valid depth.
- Runtime acceptance requires screenshots plus fresh sequence/capture/log evidence.
- Packet receipt age measures freshness, not end-to-end latency.
- Do not expand into combat/inventory while capture is unverified.
- Preserve user changes; no launcher/account tasks or deployment without current scope.
- Publish original code, not game assets, saves, credentials or decompiled sources.

## Git and verification

Actual project history uses `.git-crafterhunter`, NOT the unrelated `.git`:
`git --git-dir=.git-crafterhunter --work-tree=. <command>`.
Commit isolated changes when authorized. Do not push unless requested.

For Fabric: `cd minecraft/fabric && ./gradlew build`.
Run relevant headless scripts in tools/ and `cargo test --workspace` for protocol
changes. Avoid runtime verification scripts when working headless: they mutate
request files and require live games. Record exact checks and unresolved gaps.
