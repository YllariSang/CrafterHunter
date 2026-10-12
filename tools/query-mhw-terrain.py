#!/usr/bin/env python3
"""One diagnostic ray through the running Minecraft endpoint; host metres only."""
import argparse
import json
import math
import os
from pathlib import Path
import secrets
import tempfile
import time


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--start', type=float, nargs=3, required=True)
    parser.add_argument('--end', type=float, nargs=3, required=True)
    parser.add_argument('--channel', type=Path, default=Path('/dev/shm/crafterhunter'))
    args = parser.parse_args()
    values = args.start + args.end
    length2 = sum((a-b)**2 for a, b in zip(args.start, args.end))
    if not all(math.isfinite(v) and abs(v) <= 100000 for v in values) or not 0 < length2 <= 4096:
        parser.error('finite endpoints within 100000 metres, segment length (0,64] metres required')
    args.channel.mkdir(parents=True, exist_ok=True)
    nonce = secrets.randbelow(2**63 - 1) + 1
    request = args.channel / 'terrain.request'
    # Lock only cooperating diagnostic writers; never replace another pending request.
    import fcntl
    with (args.channel / 'terrain.writer.lock').open('a') as lock:
        fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
        if request.exists():
            raise SystemExit('A terrain request is already pending; wait for the guest to consume it.')
        fd, temporary = tempfile.mkstemp(prefix='terrain-request-', dir=args.channel)
        try:
            with os.fdopen(fd, 'w') as stream:
                stream.write(f'{nonce} {int(time.time()*1000)+3000} ' + ' '.join(map(str, values)) + '\n')
            os.replace(temporary, request)
        finally:
            Path(temporary).unlink(missing_ok=True)
        deadline = time.monotonic() + 5
        while time.monotonic() < deadline:
            try:
                parts = (args.channel / 'terrain.reply').read_text().split()
                if parts and parts[0] == str(nonce):
                    print(json.dumps({'nonce': nonce, 'start_host_metres': args.start,
                                      'end_host_metres': args.end, 'reply': parts[1:]}))
                    return 0 if parts[1] in ('hit', 'miss', 'unavailable') else 1
            except FileNotFoundError:
                pass
            time.sleep(.02)
        # A late consumer will reject the expired request. Do not race its claim.
        raise SystemExit('No matching guest reply; request expires automatically. No surface inferred.')


if __name__ == '__main__':
    raise SystemExit(main())
