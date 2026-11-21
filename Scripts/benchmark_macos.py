#!/usr/bin/env python3
"""Capture a short, isolated packaged-game CSV sample or summarize an existing one."""
import argparse
import csv
import json
import math
from pathlib import Path
import statistics
import subprocess

parser = argparse.ArgumentParser()
parser.add_argument('--label', required=True)
parser.add_argument('--csv', type=Path)
args = parser.parse_args()
project = Path(__file__).resolve().parents[1]
profile = Path('/private/tmp') / ('hf-performance-' + args.label + '-profile')
if args.csv:
    capture = args.csv
else:
    executable = project / 'Build/Mac-Walkthrough/HumanFishing.app/Contents/MacOS/HumanFishing'
    subprocess.run([str(executable), '/Game/HumanFishing/Maps/L_DeepOcean',
                    '-Windowed', '-ResX=1280', '-ResY=720', '-nosound', '-unattended',
                    '-UserDir=' + str(profile), '-csvCaptureFrames=600', '-ExitAfterCsvProfiling',
                    '-abslog=/private/tmp/hf-performance-' + args.label + '.log'],
                   stdout=subprocess.DEVNULL, stderr=subprocess.STDOUT, check=True, timeout=180)
    capture = max((profile / 'Saved/Profiling/CSV').glob('*.csv'), key=lambda p: p.stat().st_mtime)
metrics = ['FrameTime', 'GameThreadTime', 'RenderThreadTime', 'RHIThreadTime', 'GPUTime']
rows = []
with capture.open() as stream:
    for row in csv.DictReader(stream):
        try:
            values = {metric: float(row[metric]) for metric in metrics}
        except (ValueError, KeyError, TypeError):
            continue
        if values['FrameTime'] > 0 and all(math.isfinite(v) and v >= 0 for v in values.values()):
            rows.append(values)
steady = rows[120:600]
if not steady:
    raise SystemExit('No steady-state timing samples in ' + str(capture))
report = {'csv': str(capture), 'discarded_warmup_frames': 120, 'samples': len(steady), 'milliseconds': {}}
for metric in metrics:
    values = sorted(row[metric] for row in steady)
    report['milliseconds'][metric] = {'mean': statistics.mean(values), 'p95': values[math.ceil(.95 * len(values)) - 1]}
destination = project / 'Saved/Validation' / ('performance-' + args.label + '.json')
destination.parent.mkdir(parents=True, exist_ok=True)
destination.write_text(json.dumps(report, indent=2) + '\n')
print(json.dumps(report, indent=2))
