"""ECal ECON-D ZS yaml (TASKS SET_ZS_THRESHOLDS, YAML source) from a pass-through pedestal CSV:
c = round(mean) + max(T, ceil(k * rms)) per channel, calib slot 18 = 255, lambda = kappa = 0, ZS on.
  python3 ecal_zs_from_ped.py ped.csv T out.yaml [k] [clip]
(T: one value, or per ROC as roc:T pairs, e.g. 0:6,1:5,2:6,3:6,4:5)
(clip: mean and rms without readouts > 20 ADC from the median, e.g. rare half-ROC drops)
"""
import csv
import math
import statistics as st
import sys
from collections import defaultdict

f, out = sys.argv[1], sys.argv[3]
# T per ROC; a single value applies to all
if ':' in sys.argv[2]:
    T = {int(r): int(t) for r, t in (x.split(':') for x in sys.argv[2].split(','))}
else:
    T = defaultdict(lambda: int(sys.argv[2]))
k = float(sys.argv[4]) if len(sys.argv) > 4 else 0
clip = len(sys.argv) > 5 and sys.argv[5] == 'clip'


def kept(x):
    if not clip:
        return x
    m = st.median(x)
    return [v for v in x if abs(v - m) <= 20]


by = defaultdict(list)
for r in csv.DictReader(open(f)):
    if r['channel'].isdigit():
        by[(int(r['i_roc']), int(r['channel']))].append(int(r['adc']))
# EcalModule ROC_ERX_MAPPING_DAQ
erx = [(9, 10), (5, 6), (0, 1), (11, 8), (7, 4), (3, 2)]
L = ['ROCDAQCTRL:', '  GLOBAL_PASS_THRU_MODE: 0', 'ZS:']
cs = []
for roc in sorted({k[0] for k in by}):
    for half, e in enumerate(erx[roc]):
        for s in range(37):
            ch = 36 * half + (s if s < 18 else s - 1)
            c = 255 if s == 18 else min(255, int(st.mean(kept(by[(roc, ch)])) + 0.5) + max(T[roc], math.ceil(k * st.pstdev(kept(by[(roc, ch)])))))
            if s != 18:
                cs.append(c)
            L += [f'  {e}_ZS_C_I_{s}: {c}', f'  {e}_ZS_LAMBDA_{s}: 0', f'  {e}_ZS_KAPPA_{s}: 0']
open(out, 'w').write('\n'.join(L) + '\n')
print(f'{out}: c {min(cs)} to {max(cs)}, {len(cs)} channels')
