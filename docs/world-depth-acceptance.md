# Next gate: actual Minecraft block depth

Status: implemented validation tooling, **not accepted in game**.
Do not extend native composition or gameplay until fresh live evidence passes.
Use the existing WorldCapture path, not a new debug mesh or overlay.

## Plan and acceptance

1. With deployment authorized, install the built Fabric jar and restart Minecraft.
   Keep the existing bridge/MHW setup unchanged. No launcher/account automation.
2. Use an existing full opaque Minecraft block, preferably stone, in the test
   world. Disable camera following (F7) for this independent guest check. Use a
   level, stationary camera facing squarely at the block's near face. Keep sky
   and ground visible to make colour/depth orientation measurable. Avoid camera
   bobbing, portal effects, fluids, glass, and block-edge pixels.
3. Establish distance independently from block-face and camera coordinates.
   At a level, face-normal view the forward-axis coordinate difference is the
   expected depth. Use the **surface**, not the block centre or player feet.
   Off-axis Euclidean distance is NOT linearised depth. Do not derive the
   expected distance from the captured projection/depth being tested.
4. Run the existing runtime `bash tools/verify-world-capture.sh` only while the
   games are ready for testing. It mutates capture requests and archives results;
   it is not a headless test. Record screenshots of the scene, camera/block
   coordinates, loaded jar hash and fresh logs beside the archived PAIRED files.
5. Choose a pixel safely inside the block face. `--x/--y` address the raw file:
   with bottom-up metadata, PNG top-down row y corresponds to height-1-y.
   Against that archive run:

   ```bash
   python3 tools/validate-world-depth.py --acceptance \
     --meta /absolute/archive/PAIRED/world-capture.meta \
     --depth /absolute/archive/PAIRED/world-depth.f32 \
     --colour /absolute/archive/PAIRED/world-colour.rgba \
     --known-dist 4 --x 400 --y 500
   ```

   Replace the example distance and pixel with independently established values.
6. Repeat at a second distinct known distance (for example 8 blocks), capture
   again and archive separately. Both must pass within the existing 10% tolerance.
   Explain the prior 4.9-degree FOV from fresh pose/projection evidence; do not
   silently clamp it or invent another projection.

Acceptance requires BOTH mode, issue-time capture boundary, positive capture
identity/generation, matching SHA-256 colour/depth artifacts, finite non-flat
window depth, recorded projection, colour/depth orientation CONFIRMED, and a
known-distance match. Hashes bind bytes to metadata; they do not establish that
the pixels or camera transform are physically correct. Synthetic regression
fixtures test the validator, not Minecraft rendering. Screenshots/logs and two
real block measurements remain mandatory.

Without `--acceptance` the validator remains an inspection tool: exit zero is
not milestone acceptance. Old captures without hashes cannot pass the new gate.
The existing GPU-timeout retention/restart policy is unchanged.

Headless checks: `python3 tools/test-validate-world-depth.py`,
`python3 tools/test-world-capture-hook.py`, `bash tools/test-fabric-frame.sh`,
and `cd minecraft/fabric && ./gradlew build`.
