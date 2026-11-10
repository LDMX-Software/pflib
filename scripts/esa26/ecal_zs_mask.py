"""Mask ECal channels in an ECON-D ZS yaml: python3 ecal_zs_mask.py in.yaml out.yaml roc:ch [roc:ch ...]"""
import sys

fin, fout, chans = sys.argv[1], sys.argv[2], sys.argv[3:]
# EcalModule ROC_ERX_MAPPING_DAQ
erx = [(9, 10), (5, 6), (0, 1), (11, 8), (7, 4), (3, 2)]
lines = open(fin).read().splitlines()
zs = lines.index('ZS:')
for rc in chans:
    roc, ch = map(int, rc.split(':'))
    half, c = divmod(ch, 36)
    e, s = erx[roc][half], (c if c < 18 else c + 1)
    key = f'  {e}_ZS_C_I_{s}:'
    i = next(i for i, l in enumerate(lines) if l.startswith(key))
    lines[i] = f'{key} 255'
    lines.insert(zs + 1, f'  {e}_ZS_MASK_I_{s}: 1')
open(fout, 'w').write('\n'.join(lines) + '\n')
print(f'{fout}: masked {" ".join(chans)}')
