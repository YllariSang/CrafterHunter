#!/usr/bin/env python3
"""Select diagnostic-only player shading; never touches channels or calibration."""
import argparse
import importlib.util
from pathlib import Path
spec=importlib.util.spec_from_file_location('paired_control',Path(__file__).with_name('control-paired-preview.py'))
paired=importlib.util.module_from_spec(spec)
spec.loader.exec_module(paired)
atomic_write=paired.atomic_write

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('mode',choices=['real','uv','net'])
parser.add_argument('--render',type=Path,default=Path.home()/'.local/share/Steam/steamapps/common/Monster Hunter World/nativePC/plugins/CSharp/CrafterHunter/render')
args=parser.parse_args()
flags={'uv':'player-uv-debug.enabled','net':'player-skin-net-debug.enabled'}
for mode,name in flags.items():
    flag=args.render/name
    if args.mode==mode:
        atomic_write(flag,'debug only\n')
    else:
        flag.unlink(missing_ok=True)
print('Player shading:',args.mode,'(real asset, pose, anchor and depth guards unchanged)')
