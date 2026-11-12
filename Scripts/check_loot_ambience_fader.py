"""Run after the audio PIE: python check_loot_ambience_fader.py <PIE log>."""
import json
import re
import sys
from pathlib import Path

log = Path(sys.argv[1])
samples = re.findall(r'LootLoopFader phase=(\w+) gain=([\d.]+)', log.read_text(encoding='utf-8'))
report = {}
for phase in ('near', 'return', 'retry'):
    gains = [float(value) for name, value in samples if name == phase]
    assert gains and any(0.001 < value < 0.95 for value in gains), (phase, 'Missing gradual fade')
    assert gains[-1] >= 0.99, (phase, 'Never reached full gain')
    report[phase] = dict(samples=len(gains), minimum=min(gains), final=gains[-1])
out = log.parent / 'LootAmbienceContinuity' / 'fader.json'
out.write_text(json.dumps(report, indent=2), encoding='utf-8')
print(json.dumps(report, indent=2))
