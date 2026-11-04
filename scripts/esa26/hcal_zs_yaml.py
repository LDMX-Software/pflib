"""HCal ECON-D ZS thresholds for TASKS SET_ZS_THRESHOLDS (YAML source) from a pass-through pedestal CSV:
c = round(mean) + T per channel, calib slot 18 = 255, lambda = kappa = 0.
  python3 hcal_zs_yaml.py ped.csv T out.yaml
"""
import csv
import statistics as st
import sys
from collections import defaultdict

f, T, out = sys.argv[1], int(sys.argv[2]), sys.argv[3]
by = defaultdict(list)
for r in csv.DictReader(open(f)):
    if r['channel'].isdigit():
        by[(int(r['i_roc']), int(r['channel']))].append(int(r['adc']))
# HCal DAQ eRx per ROC half
erx = {0: (3, 2), 2: (4, 5), 3: (1, 0)}
L = ['ROCDAQCTRL:', '  GLOBAL_PASS_THRU_MODE: 0', 'ZS:']
cs = []
for roc, (e0, e1) in erx.items():
    for half, e in ((0, e0), (1, e1)):
        for s in range(37):
            c = 255 if s == 18 else min(255, int(st.mean(by[(roc, 36 * half + (s if s < 18 else s - 1))]) + 0.5) + T)
            if s != 18:
                cs.append(c)
            L += [f'  {e}_ZS_C_I_{s}: {c}', f'  {e}_ZS_LAMBDA_{s}: 0', f'  {e}_ZS_KAPPA_{s}: 0']
open(out, 'w').write('\n'.join(L) + '\n')
print(f'{out}: c {min(cs)} to {max(cs)}')
