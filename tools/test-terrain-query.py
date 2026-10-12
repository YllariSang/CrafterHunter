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
    # Original fixed idle interval must fail the sequential-writer regression.
    mutant = Path(out)/'TerrainQuery.java'
    mutant.write_text((src/'TerrainQuery.java').read_text().replace(
        '(now < responsiveUntil ? ACTIVE_POLL : IDLE_POLL)', 'IDLE_POLL'))
    subprocess.run(['javac', '-d', out, str(mutant),
                    str(root/'minecraft/fabric/tests/TerrainQueryTest.java')], check=True)
    result = subprocess.run(['java', '-cp', out, 'dev.crafterhunter.client.TerrainQueryTest'],
                            text=True, capture_output=True)
    assert result.returncode != 0 and 'sequential ray charged idle delay' in result.stderr, result.stderr
    print('Original idle-delay mutant rejected by sequential ray regression')
    mutant.write_text((src/'TerrainQuery.java').read_text().replace('count > 8', 'count > 1'))
    subprocess.run(['javac', '-d', out, str(mutant),
                    str(root/'minecraft/fabric/tests/TerrainQueryTest.java')], check=True)
    result = subprocess.run(['java', '-cp', out, 'dev.crafterhunter.client.TerrainQueryTest'],
                            text=True, capture_output=True)
    assert result.returncode != 0 and 'batch was not fully admitted' in result.stderr, result.stderr
    print('Original single-ray admission mutant rejected by batch regression')
bridge = (src/'BridgeClient.java').read_text()
assert 'terrain.poll(now, System.currentTimeMillis())' in bridge
assert 'send(activeSocket, Protocol.KIND_TERRAIN_REQUEST, query)' in bridge
assert 'terrain.receive(packet.payload(), System.nanoTime())' in bridge
assert 'terrain.disconnect()' in bridge
print('Guest send/receive/lifecycle wiring present (absent in the original endpoint)')
