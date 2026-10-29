"""Assemble one calibration set for a pftool target

Merges the per-ROC settings files written by the tuning tasks (later files
win), adds masks, an optional FC calib delay, bookkeeping and the expected
pedestal per channel. Load the result with ROC APPLY_CALIB.

    python3 ana/calib/make_calib.py config/calib/ecal-smm-esa-l2/2026-10-28.yaml \\
        --base config/hgcroc/ecal-hdfull-cms-mmts.yaml \\
        --settings global_pedestal_level-roc-*-settings_20261028_172251.yaml \\
                   local_pedestal_level-roc-*-settings_20261028_1722*.yaml \\
        --mask 0:MASK_TOA:19,20 --calib-delay 17 \\
        --pedestal pedestal_20261028_205500.csv --meta note="L2, no HV"
"""

import argparse
import csv
import datetime
import re
import statistics
import subprocess
from collections import defaultdict
from pathlib import Path

import yaml

parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
parser.add_argument('output', type=Path, help='calibration file to write')
parser.add_argument('--base', help='settings file applied to every ROC first')
parser.add_argument('--settings', nargs='+', default=[], type=Path,
                    help='per-ROC settings files, ROC index taken from "roc<N>" or "roc-<N>" in the name')
parser.add_argument('--mask', action='append', default=[],
                    help='ROC:PARAM:CH,CH,... e.g. 0:MASK_TOA:19,20')
parser.add_argument('--calib-delay', type=int, help='FC calib to L1A delay to set')
parser.add_argument('--pedestal', type=Path, help='pedestal CSV taken with this set, for expected_pedestal')
parser.add_argument('--meta', action='append', default=[], help='key=value bookkeeping')
args = parser.parse_args()

rocs = defaultdict(dict)
sources = []
for f in args.settings:
    m = re.search(r'roc-?(\d+)', f.name)
    if m is None:
        parser.error(f'no ROC index in {f}')
    for page, params in (yaml.safe_load(f.read_text()) or {}).items():
        rocs[f'roc{m.group(1)}'].setdefault(page.upper(), {}).update(
            {k.upper(): v for k, v in params.items()})
    sources.append(str(f))

masks = defaultdict(dict)
for spec in args.mask:
    i_roc, param, chans = spec.split(':')
    masks[f'roc{i_roc}'][param.upper()] = [int(c) for c in chans.split(',')]

meta = {'date': datetime.date.today().isoformat()}
try:
    meta['pflib'] = subprocess.run(['git', 'describe', '--always', '--dirty'],
                                   capture_output=True, text=True).stdout.strip()
except OSError:
    pass
meta.update(kv.split('=', 1) for kv in args.meta)
meta['sources'] = sources

calib = {'meta': meta, 'base': args.base}
if args.calib_delay is not None:
    calib['fc'] = {'calib_delay': args.calib_delay}
if masks:
    calib['masks'] = dict(sorted(masks.items()))


def ch_key(page):
    m = re.match(r'CH_(\d+)$', page)
    return (1, int(m.group(1))) if m else (0, page)


for roc in sorted(rocs, key=lambda r: int(r[3:])):
    calib[roc] = {page: rocs[roc][page] for page in sorted(rocs[roc], key=ch_key)}

if args.pedestal:
    adc = defaultdict(lambda: defaultdict(list))
    with open(args.pedestal) as f:
        for row in csv.DictReader(f):
            if row['channel'].isdigit():
                adc[int(row['i_roc'])][int(row['channel'])].append(int(row['adc']))
    calib['expected_pedestal'] = {
        f'roc{r}': [round(statistics.median(adc[r][c]), 1) for c in range(72)]
        for r in sorted(adc)}
    meta['pedestal'] = str(args.pedestal)


class Flow(list):
    """list written inline"""


class FlowMap(dict):
    """map written inline"""


yaml.add_representer(Flow, lambda d, v: d.represent_sequence('tag:yaml.org,2002:seq', v, flow_style=True))
yaml.add_representer(FlowMap, lambda d, v: d.represent_mapping('tag:yaml.org,2002:map', v, flow_style=True))
for roc in rocs:
    for page in calib[roc]:
        calib[roc][page] = FlowMap(calib[roc][page])
for roc in calib.get('masks', {}).values():
    for p in roc:
        roc[p] = Flow(roc[p])
for r in calib.get('expected_pedestal', {}):
    calib['expected_pedestal'][r] = Flow(calib['expected_pedestal'][r])

args.output.parent.mkdir(parents=True, exist_ok=True)
with open(args.output, 'w') as f:
    f.write('# calibration set, load with ROC APPLY_CALIB (written by ana/calib/make_calib.py)\n')
    yaml.dump(calib, f, sort_keys=False, default_flow_style=False, width=120)
print(f'wrote {args.output}: {len(rocs)} ROCs, {sum(len(v) for v in masks.values())} mask entries')
