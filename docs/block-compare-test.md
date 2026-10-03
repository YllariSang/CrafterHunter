# Minecraft block A/B/C comparison (v0.3)

This is an experimental rendering comparison, not a finished passthrough.
Minecraft supplies the `minecraft:stone` texture from its active resource pack
at runtime; the repository and release do not contain that texture. All three
blocks use the same Minecraft-owned pixels and stay at separate fixed MHW
world positions. The MHW plugin draws nothing until it receives both the pixel
and PNG packets. There is no monster damage, interaction, placement, or MHW
collision yet.

The three labeled methods are:

- **A — 3D mesh:** texel-colored face geometry registered with SharpPluginLoader.
- **B — 3D lines:** Minecraft pixel colors drawn with SharpPluginLoader's line pass.
- **C — image quads:** Minecraft PNG drawn as projected ImGui image quads. This
  intentionally has no MHW scene depth and serves as a draw-through control.

Each method can fail independently; check the CrafterHunter plugin diagnostic log
for `Block method A/B/C failed` if one vanishes. Rendering and occlusion remain
unverified until an in-game test. A good result holds position while moving the
MHW camera, rotates in perspective, and is hidden by nearer MHW geometry. C is
not expected to satisfy the last condition. Record the three methods near a
wall or supply box from several angles, including above and below. Do not
interpret a bright line or floating sphere as a successful block.

## Prepare without launching either game

Close both games, then in a normal terminal (outside this restricted build
environment):

```bash
cd /home/yllaris/CrafterHunter/minecraft/fabric
./gradlew build --offline
cd /home/yllaris/CrafterHunter
bash tools/prepare-render-probe.sh block
```

For MHW's Steam launch options, replace the old cube probe flag with:

```text
DOTNET_ROOT= WINEDLLOVERRIDES="ucrtbase=n,b" CRAFTERHUNTER_BLOCK_COMPARE=1 %command%
```

Then run `bash tools/prepare-play.sh` in a terminal. It installs the v0.3
Fabric JAR, starts the local bridge, and does not launch either game. Launch
your legitimately licensed Minecraft client and MHW normally, enter a
Minecraft world and MHW field area, and use the existing F7/F8 link controls.
The plugin needs Minecraft's active block resource; an MHW-only session shows
no A/B/C comparison. Leave `CRAFTERHUNTER_CUBE_PROBE` unset for this test.

The old Minecraft JAR is moved to a timestamped rollback directory by
`prepare-play.sh`; the previous MHW plugin is copied to its rollback directory
by `prepare-render-probe.sh`. Neither script launches an account tool.
