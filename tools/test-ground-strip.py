#!/usr/bin/env python3
"""Headless all-or-nothing strip reply admission; never accesses live channels."""
import importlib.util
from pathlib import Path
spec=importlib.util.spec_from_file_location('ground',Path(__file__).with_name('control-ground-contact.py'))
g=importlib.util.module_from_spec(spec);spec.loader.exec_module(g)
offsets=[(0,0),(-.25,-.25),(-.25,.25),(.25,-.25),(.25,.25),(.5,0),(.75,-.25),(.75,.25)]
segments=[[x,1,z,x,-5,z] for x,z in offsets]
lines=['123 batch 8']+[f'{i} hit {x} 0 {z} 0 1 0 1048576' for i,(x,z) in enumerate(offsets)]
assert len(g.validate_strip_reply(lines,123,segments,1,1.2))==8

def refused(candidate,nonce=123,issued=1,now=1.2):
    try:g.validate_strip_reply(candidate,nonce,segments,issued,now)
    except (RuntimeError,ValueError):return
    raise AssertionError('unsafe strip reply accepted: '+repr(candidate))
refused(lines[:-1]);refused(lines,nonce=124);refused(lines,now=1.5);refused(lines,now=.9)
refused(lines+['8 hit 0 0 0 0 1 0 0'])
for replacement in ['7 miss','7 unavailable','7 hit .75 .1 .25 0 1 0 0',
                    '7 hit 99 0 .25 0 1 0 0','7 hit .75 0 .25 0 .01 0 0',
                    '7 hit .75 0 .25 NaN 1 0 0','7 hit .75 0 .25 0 1 0 -1',
                    '6 hit .75 0 .25 0 1 0 0']:
    refused(lines[:-1]+[replacement])
print('Strip missing/mixed/stale/nonflat/off-segment/nonunit/duplicate reply refusal passed')
