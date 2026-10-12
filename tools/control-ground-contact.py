#!/usr/bin/env python3
"""Bounded flat-patch sampler for the current ACTIVE camera anchor. No game launches.
Reads only our loaded renderer symbols. Five fixed rays; no position/physics writes.
"""
import argparse
import fcntl
import json
import math
import os
from pathlib import Path
import re
import secrets
import struct
import subprocess
import tempfile
import time

ROOT = Path(__file__).resolve().parents[1]
CHANNEL = Path('/dev/shm/crafterhunter')


def atomic(path, text):
    fd, name = tempfile.mkstemp(prefix=path.name+'-', dir=path.parent)
    try:
        with os.fdopen(fd, 'w') as stream:
            stream.write(text)
        os.replace(name, path)
    finally:
        Path(name).unlink(missing_ok=True)


class AnchorReader:
    def __init__(self, pid):
        proc = Path('/proc')/str(pid)
        if not (proc/'comm').read_text().startswith('MonsterHunter'):
            raise RuntimeError('PID is not MHW')
        rows = [line.split(maxsplit=5) for line in (proc/'maps').read_text().splitlines()
                if 'renderer-' in line and '.bin' in line]
        paths = {r[5] for r in rows}
        if len(paths) != 1:
            raise RuntimeError('Expected one loaded renderer')
        loaded = Path(paths.pop())
        subprocess.run(['cmp', str(loaded), str(ROOT/'native/mhw-renderer/build/CrafterHunter.Render.dll')], check=True)
        base = int(next(r for r in rows if int(r[2],16)==0)[0].split('-')[0],16)
        preferred = int(re.search(r'^ImageBase\s+([0-9a-f]+)$', subprocess.check_output(
            ['x86_64-w64-mingw32-objdump','-p',str(loaded)],text=True),re.M).group(1),16)
        symbols = subprocess.check_output(['x86_64-w64-mingw32-nm','-C',str(loaded)],text=True)
        hits = re.findall(r'^([0-9a-f]+) [bBdD] \(anonymous namespace\)::alignmentSession$',symbols,re.M)
        if len(hits)!=1:
            raise RuntimeError('Missing own anchor symbol')
        self.address = base+int(hits[0],16)-preferred
        self.fd = os.open(proc/'mem',os.O_RDONLY)

    def read(self):
        data = os.pread(self.fd,73,self.address)
        if data != os.pread(self.fd,73,self.address):
            raise RuntimeError('Anchor changed while reading')
        guest = struct.unpack_from('<3d',data,0)
        host = struct.unpack_from('<3d',data,24)
        scale, generation, epoch = struct.unpack_from('<dQQ',data,48)
        if data[72]!=1 or scale!=1 or not generation or not epoch or not all(map(math.isfinite,guest+host)):
            raise RuntimeError('No valid one-metre fixed anchor')
        return data, guest, host, generation, epoch


def validate_strip_reply(lines, nonce, segments, issued, now):
    """All-or-nothing batch: no geometry from partial, mixed, stale or nonflat data."""
    if (not lines or lines[0].split()!=[str(nonce),'batch','8'] or len(lines)!=9
            or len(segments)!=8 or now<issued or now-issued>=.5):
        raise RuntimeError('Incomplete, mismatched or stale strip batch')
    samples=[]
    for i,(line,segment) in enumerate(zip(lines[1:],segments)):
        f=line.split()
        if len(f)!=9 or f[:2]!=[str(i),'hit']:
            raise RuntimeError('Strip requires eight ordered hits')
        v=list(map(float,f[2:8]));hx,hy,hz,nx,ny,nz=v
        attribute=int(f[8])
        if (not 0<=attribute<=0xffffffff or not all(map(math.isfinite,v))
                or abs(hx-segment[0])>.001 or abs(hz-segment[2])>.001
                or not segment[4]<=hy<=segment[1]
                or abs(nx*nx+ny*ny+nz*nz-1)>.01 or ny<.999):
            raise RuntimeError('Invalid strip hit geometry')
        samples.append(v)
    if max(v[1] for v in samples)-min(v[1] for v in samples)>.01:
        raise RuntimeError('Strip is not flat within one centimetre')
    return samples


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--pid',type=int,required=True)
    parser.add_argument('--seconds',type=int,default=60)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--strip',action='store_true',help='Two adjoining fixed cells along +X; eight-ray batch')
    args=parser.parse_args()
    if not 1<=args.seconds<=120:
        parser.error('duration must be 1..120 seconds')
    reader=AnchorReader(args.pid)
    original,guest,host,generation,epoch=reader.read()
    token=int((Path.home()/'.minecraft/crafterhunter/guest-control.enabled').read_text())
    if token<=0:raise RuntimeError('Control not requested')
    last_sequence=0;advanced=time.monotonic();observations=0
    def active():
        nonlocal last_sequence,advanced,observations
        if int((Path.home()/'.minecraft/crafterhunter/guest-control.enabled').read_text())!=token:
            raise RuntimeError('F6/release or different token')
        f=(CHANNEL/'camera.reply').read_text().split()
        if len(f)!=5 or f[0]!='active' or list(map(int,f[1:4]))!=[token,generation,epoch]:
            raise RuntimeError('Camera not ACTIVE for this anchor')
        seq=int(f[4]);now=time.monotonic()
        if seq<last_sequence:raise RuntimeError('Camera sequence rollback')
        if seq>last_sequence:last_sequence=seq;advanced=now;observations+=1
        if now-advanced>=.5:raise RuntimeError('Camera acknowledgement stopped advancing')
        if reader.read()[0]!=original:raise RuntimeError('Anchor changed')
    args.output.mkdir(parents=True,exist_ok=True)
    records=[];hits=[None]*5;published=0;start=time.monotonic()
    patch=CHANNEL/'ground.patch'
    with (CHANNEL/'terrain.writer.lock').open('a') as lock:
        fcntl.flock(lock,fcntl.LOCK_EX|fcntl.LOCK_NB)
        patch.unlink(missing_ok=True)
        reason="interrupted"
        try:
            active();time.sleep(.1);active()
            if observations<2:raise RuntimeError('Need advancing ACTIVE acknowledgement')
            print(f'Fixed test patch guest centre={guest} host centre={host} token={token} epoch={epoch}',flush=True)
            offsets=[(0,0),(-.25,-.25),(-.25,.25),(.25,-.25),(.25,.25)]
            index=0
            while time.monotonic()-start<args.seconds:
                if args.strip:
                    strip_offsets=offsets+[(.5,0),(.75,-.25),(.75,.25)]
                    active();x,y,z=host
                    segments=[[x+dx,y+1,z+dz,x+dx,y-5,z+dz] for dx,dz in strip_offsets]
                    nonce=secrets.randbelow(2**63-1)+1
                    if (CHANNEL/'terrain.request').exists():raise RuntimeError('Another query pending')
                    issued=time.monotonic()
                    values=[v for segment in segments for v in segment]
                    atomic(CHANNEL/'terrain.request',f'batch {nonce} {int(time.time()*1000)+1000} 8 '+
                           ' '.join(map(str,values))+'\n')
                    answer=None
                    while time.monotonic()-issued<1:
                        active()
                        try:
                            lines=(CHANNEL/'terrain.reply').read_text().splitlines()
                            if lines and lines[0].split()[0]==str(nonce):answer=lines;break
                        except FileNotFoundError:pass
                        time.sleep(.005)
                    now=time.monotonic()
                    samples=validate_strip_reply(answer,nonce,segments,issued,now)
                    records.append({'nonce':nonce,'hits':samples,'elapsed':now-start,
                                    'round_trip_ms':(now-issued)*1000})
                    active();now=time.monotonic();age=math.ceil((now-issued)*1000)
                    if age>=500:raise RuntimeError('Strip batch too slow; collider remains disabled')
                    fields=[2,time.monotonic_ns(),int(time.time()*1000),token,generation,epoch,*guest,*host]
                    for v in samples:fields.extend([*v,age])
                    atomic(patch,' '.join(map(str,fields))+'\n');published+=1
                    if published==1:print('Fresh two-cell strip published; fixed +X, 1m by 0.5m',flush=True)
                    continue
                active();dx,dz=offsets[index];x,y,z=host
                segment=[x+dx,y+1,z+dz,x+dx,y-5,z+dz]
                nonce=secrets.randbelow(2**63-1)+1
                if (CHANNEL/'terrain.request').exists():raise RuntimeError('Another query pending')
                issued=time.monotonic()
                atomic(CHANNEL/'terrain.request',f'{nonce} {int(time.time()*1000)+1000} '+' '.join(map(str,segment))+'\n')
                answer=None
                while time.monotonic()-issued<1:
                    active()
                    try:
                        f=(CHANNEL/'terrain.reply').read_text().split()
                        if f and f[0]==str(nonce):answer=f[1:];break
                    except FileNotFoundError:pass
                    time.sleep(.005)
                if not answer or len(answer)!=8 or answer[0]!='hit':
                    raise RuntimeError('No usable ground hit: '+str(answer))
                v=list(map(float,answer[1:7]));hx,hy,hz,nx,ny,nz=v
                if not all(map(math.isfinite,v)) or abs(hx-segment[0])>.001 or abs(hz-segment[2])>.001 or not y-5<=hy<=y+1:
                    raise RuntimeError('Hit outside requested segment')
                if abs(nx*nx+ny*ny+nz*nz-1)>.01 or ny<.999:
                    raise RuntimeError('Flat upward unit normal required')
                hits[index]=(v,issued);records.append({'index':index,'nonce':nonce,'hit':v,'elapsed':time.monotonic()-start})
                index=(index+1)%5
                if all(h is not None for h in hits):
                    now=time.monotonic();ages=[math.ceil((now-h[1])*1000) for h in hits]
                    if max(ages)>=500:raise RuntimeError('Five-ray sweep too slow; collider remains disabled')
                    if max(h[0][1] for h in hits)-min(h[0][1] for h in hits)>.01:
                        raise RuntimeError('Ground is not flat within one centimetre')
                    active();seq=time.monotonic_ns()
                    fields=[1,seq,int(time.time()*1000),token,generation,epoch,*guest,*host]
                    for h,age in zip(hits,ages):fields.extend([*h[0],age])
                    atomic(patch,' '.join(map(str,fields))+'\n');published+=1
                    if published==1:print('Fresh ground patch published; fixed centre, 0.5m square',flush=True)
            reason='bounded duration complete'
        except (OSError,ValueError,RuntimeError) as error:
            reason=str(error)
        finally:
            patch.unlink(missing_ok=True)
            os.close(reader.fd)
            report={'token':token,'generation':generation,'epoch':epoch,'guest_origin':guest,'host_origin':host,
                    'published':published,'reason':reason,'strip':args.strip,'rays':records,'limits':'Flat sampled patch only; collider hook counts and landing still require game evidence'}
            (args.output/'ground-sampler.json').write_text(json.dumps(report,indent=2)+'\n')
            print(json.dumps({k:v for k,v in report.items() if k!='rays'}),flush=True)
    return 0 if published else 1


if __name__=='__main__':
    raise SystemExit(main())
