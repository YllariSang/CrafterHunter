# Tested builds

This file records non-secret compatibility evidence. Never commit or upload a
game executable.

## Monster Hunter: World

| Date | Game window build | Steam build ID | Executable size | SHA-256 | Host | Result |
|---|---:|---:|---:|---|---|---|
| 2026-10-03 | 421810 | 15539686 | 84225952 | `c2ebbbd2c49f216d484e31a5219bed419eb1e5e7d206d02cba040a3ab79d90ea` | Arch Linux, Proton/DXVK | SPL 1.0.0 loaded endpoint; D3D11 and UDP heartbeat confirmed |

The hash identifies compatibility; it cannot reconstruct or redistribute the
game. Runtime features must still fail closed when their required API or
signature is unavailable.
