"""Mask and per-channel +1 on a SET_ZS_THRESHOLDS ECal output yaml: python3 ecal_zs_tweak.py l2|l3 in.yaml out.yaml"""
import re
import sys

layer, fin, fout = sys.argv[1:4]
# EcalModule ROC_ERX_MAPPING_DAQ
erx = [(9, 10), (5, 6), (0, 1), (11, 8), (7, 4), (3, 2)]
# hot in every sample since 05-Nov
mask = {"l2": {(5, 0)}, "l3": set()}[layer]
# noise tails in 3 of 4 runs
plus1 = {
    "l2": {0: [9, 36, 39], 2: [8, 9, 11, 15, 21, 52, 57, 63], 3: [9, 15, 18, 19, 59, 60, 64, 68, 70, 71],
           4: [4, 17, 18, 36, 44, 65], 5: [0, 4, 32, 45, 52]},
    "l3": {0: [4], 2: [0, 1, 2, 4, 9, 15, 21, 35, 64, 69, 71], 3: [68]},
}[layer]


def key(roc, ch):
    half, c = divmod(ch, 36)
    return erx[roc][half], c if c < 18 else c + 1


lines = open(fin).read().splitlines()
vals = {}
for i, l in enumerate(lines):
    m = re.match(r"\s+(\d+)_ZS_C_I_(\d+): (\d+)$", l)
    if m:
        vals[(int(m[1]), int(m[2]))] = (i, int(m[3]))
out = list(lines)
n1 = 0
for roc, chs in plus1.items():
    for ch in chs:
        if (roc, ch) in mask:
            continue
        e, s = key(roc, ch)
        i, c = vals[(e, s)]
        out[i] = f"  {e}_ZS_C_I_{s}: {min(255, c + 1)}"
        n1 += 1
zs = out.index("ZS:")
for roc, ch in sorted(mask):
    e, s = key(roc, ch)
    i, c = vals[(e, s)]
    out[i] = f"  {e}_ZS_C_I_{s}: 255"
    out.insert(zs + 1, f"  {e}_ZS_MASK_I_{s}: 1")
open(fout, "w").write("\n".join(out) + "\n")
print(f"{fout}: {len(mask)} masked, {n1} channels +1")
