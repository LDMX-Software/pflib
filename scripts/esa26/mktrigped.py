# Per-ROC yaml: trigger-path Adc_pedestal = HV-on DAQ pedestal median (+offset)
# python3 mktrigped.py ped-summary.csv [offset] [outdir]  (csv columns roc, ch, median)
import csv, os, sys
src = sys.argv[1] if len(sys.argv) > 1 else 'ped-hvon-summary.csv'
off = int(sys.argv[2]) if len(sys.argv) > 2 else 0
out = sys.argv[3] if len(sys.argv) > 3 else '.'
rows = list(csv.DictReader(open(src)))
for roc in ('0', '2', '3'):
    with open(os.path.join(out, f'trigped-roc{roc}.yaml'), 'w') as f:
        f.write(f'# HGCROC trigger Adc_pedestal from {src}, offset {off}\n')
        for r in rows:
            if r['roc'] == roc:
                v = max(0, min(255, round(float(r['median'])) + off))
                f.write(f"CH_{r['ch']}:\n  ADC_PEDESTAL: {v}\n")
