#!/usr/bin/env python3
"""Headless guest terrain channel and original missing-path regression."""
from pathlib import Path
import subprocess
import tempfile
root = Path(__file__).resolve().parents[1]
src = root / 'minecraft/fabric/src/client/java/dev/crafterhunter/client'
with tempfile.TemporaryDirectory() as out:
    subprocess.run(['javac', '-d', out, str(src/'TerrainQuery.java'),
                    str(root/'minecraft/fabric/tests/TerrainQueryTest.java')], check=True)
    subprocess.run(['java', '-cp', out, 'dev.crafterhunter.client.TerrainQueryTest'], check=True)
bridge = (src/'BridgeClient.java').read_text()
assert 'terrain.poll(now, System.currentTimeMillis())' in bridge
assert 'send(activeSocket, Protocol.KIND_TERRAIN_REQUEST, query)' in bridge
assert 'terrain.receive(packet.payload(), System.nanoTime())' in bridge
assert 'terrain.disconnect()' in bridge
print('Guest send/receive/lifecycle wiring present (absent in the original endpoint)')
