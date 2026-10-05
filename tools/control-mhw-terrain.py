#!/usr/bin/env python3
"""Request one MHW terrain self-check; never launches games or changes accounts.

The plugin casts no ray on its own. `check` drops a request file that the next
one-second sample consumes, and the result is one `Terrain self-check` line in
CrafterHunter.runtime.log. `clear` drops a request that has not been consumed,
so a stray request cannot fire later while nobody is watching.
"""
import argparse
from pathlib import Path

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("action", choices=["check", "clear"])
parser.add_argument(
    "--game",
    type=Path,
    default=Path.home() / ".local/share/Steam/steamapps/common/Monster Hunter World",
)
args = parser.parse_args()

plugin = args.game / "nativePC/plugins/CSharp/CrafterHunter"
request = plugin / "terrain" / "check.request"
if args.action == "clear":
    request.unlink(missing_ok=True)
    print(f"Cleared any pending terrain self-check: {request}")
else:
    request.parent.mkdir(parents=True, exist_ok=True)
    request.write_text("check\n")
    print(f"Requested one down-ray self-check: {request}")
    print("Read the result from the plugin's 'Terrain self-check' lines in CrafterHunter.runtime.log.")