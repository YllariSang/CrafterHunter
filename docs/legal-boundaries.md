# Development and distribution boundaries

This document is an engineering policy, not legal advice.

## Required

- Develop and test only with legitimately obtained game installations or an
  official trial that permits the intended use.
- Distribute only original source code and binaries produced from this project.
- Require users to obtain both games independently.
- Keep the project visibly unofficial.
- Keep initial testing offline and avoid impacting other players.
- Record the origin of reverse-engineered facts needed for interoperability.
- Preserve copyright and license notices for any reused open-source code.

## Prohibited in this repository

- Game executables, JARs, DLLs, archives, decompiled source, memory dumps, or
  extracted art/audio.
- Product keys, access tokens, cookies, account credentials, or authentication
  bypasses.
- DRM or anti-cheat circumvention.
- Launch instructions whose purpose is to play either game without a license.
- Cheats or public-matchmaking functionality.

Third-party launchers are supported only when they authenticate a legitimately
licensed account and obtain game files through an authorized path.

## Analysis tooling and its output

Reverse-engineering tools may be used locally to derive interoperability facts
for a legitimately obtained installation. Running such a tool creates no
license obligation for this project: CrafterHunter neither links nor
redistributes them.

Tools in use: `objdump` and `x86_64-w64-mingw32-g++`, and Ghidra (Apache-2.0,
obtained from the system package manager). CrafterHunter is itself Apache-2.0,
which is compatible with that license in any case. Ghidra's distribution
bundles some GPL-licensed third-party components; those must not be linked into
anything this project ships, as it is used only as an external analyzer.

Because MHW's `.text` is encrypted on disk, analysis runs against a PE
reconstructed from the live process image. That file, and every artifact derived
from it, is game-derived material:

- it stays outside this repository, in scratch space;
- decompiler output is decompiled source and is **never** committed here;
- only the original reasoning, offsets and conclusions are recorded, exactly as
  for findings derived by hand.

This is an engineering policy, not legal advice.

## Named dependencies and prior art

SharpPluginLoader is an external dependency obtained from its official project;
CrafterHunter does not redistribute or silently install it. The existing
`justbustin/minecraft-crossover-bridge` project is acknowledged as prior art.
If its MIT-licensed source is reused later, the corresponding copyright and
license notice must ship with that code.
