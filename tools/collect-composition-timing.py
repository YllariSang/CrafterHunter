#!/usr/bin/env python3
"""Bounded read-only collection of stage reports. No control/capture requests."""
import argparse
import datetime
import json
from pathlib import Path
import re
import shutil
import struct
import time


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--seconds', type=float, default=30)
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    if not 1 <= args.seconds <= 120:
        parser.error('--seconds must be between 1 and 120')
    root = Path(__file__).resolve().parents[1]
    stamp = datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%d-%H%M%SZ')
    out = args.output or root/'captures/composition-timing'/('runtime-'+stamp)
    # Never overwrite a previous trace or write inside either game/channel tree.
    out = out.resolve()
    allowed = [root/'captures', Path('/tmp')]
    if not any(out.is_relative_to(p) for p in allowed):
        parser.error('--output must be under project captures/ or /tmp/')
    out.mkdir(parents=True, exist_ok=False)
    channel = Path('/dev/shm/crafterhunter')
    render = Path.home()/'.local/share/Steam/steamapps/common/Monster Hunter World/nativePC/plugins/CSharp/CrafterHunter/render'
    guest = Path.home()/'.minecraft/crafterhunter/out/world-capture.txt'
    source_log = render/'renderer.log'
    events = []
    errors = []
    prior = None
    start = time.monotonic()
    with source_log.open('rb') as log:
        log.seek(0, 2)
        offset = log.tell()
        while time.monotonic()-start < args.seconds:
            try:
                summary = guest.read_text()
                if 'captureTimingClock=java-nanoTime' not in summary:
                    raise ValueError('Guest timing build is not reporting yet')
                timing_identity = re.search(r'^captureTimingLastIdentity=(\d+)$', summary, re.M)
                if timing_identity is None:
                    raise ValueError('Guest summary was incomplete during read')
                with (channel/'world.frame').open('rb') as f:
                    h = f.read(320)
                if len(h) != 320 or h[:4] != b'CHWP':
                    raise ValueError('Incomplete world header')
                generation, identity = struct.unpack_from('<2Q', h, 32)
                reply = (channel/'camera.reply').read_text().strip()
                if summary != prior:
                    events.append(dict(observer_seconds=time.monotonic()-start,
                        world_generation=generation, world_identity=identity,
                        world_mode=struct.unpack_from('<I', h, 264)[0],
                        timing_identity=int(timing_identity.group(1)),
                        camera_reply=reply, guest_summary=summary))
                    prior = summary
            except (OSError, ValueError, struct.error) as error:
                errors.append(dict(observer_seconds=time.monotonic()-start, error=str(error)))
            time.sleep(.1)
        log.seek(offset)
        native = log.read().decode(errors='replace')
    (out/'renderer.log').write_text(native)
    (out/'samples.json').write_text(json.dumps(events, indent=2)+'\n')
    (out/'errors.json').write_text(json.dumps(errors, indent=2)+'\n')
    shutil.copy2(__file__, out/'collector.py')
    native_reports = re.findall(r'paired timing clock=host-steady interval stage=(\S+) n=(\d+) min-ms=([\d.]+) mean-ms=([\d.]+) max-ms=([\d.]+)', native)
    report = dict(duration_seconds=time.monotonic()-start, guest_snapshots=len(events),
        native_interval_rows=len(native_reports), errors=len(errors),
        world_generations=sorted({e['world_generation'] for e in events}),
        world_modes=sorted({e['world_mode'] for e in events}),
        camera_states=sorted({e['camera_reply'].split()[0] for e in events if e['camera_reply'].split()}),
        limits=['Guest summaries contain process-lifetime aggregates, not interval medians.',
            'Header, guest summary and camera acknowledgement are independently read.',
            'Java and host CPU durations use separate clocks; none are screen latency or GPU completion.',
            'This observer changes no flags, requests, settings or calibration.'])
    (out/'report.json').write_text(json.dumps(report, indent=2)+'\n')
    print(json.dumps(dict(output=str(out), **report), indent=2))
    if (not events or not native_reports
            or events[-1]['timing_identity'] <= events[0]['timing_identity']
            or not any(stage == 'upload-submit' and int(count) > 0
                for stage, count, *_ in native_reports)):
        raise SystemExit('No advancing guest timing and native upload reports; inspect saved evidence')


if __name__ == '__main__':
    main()
