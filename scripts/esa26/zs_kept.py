"""Kept channels per event in a ZS check pedestal CSV: python3 zs_kept.py zs-l2-check.csv [n_events]"""
import csv
import sys

n = int(sys.argv[2]) if len(sys.argv) > 2 else 1000
rows = [r for r in csv.DictReader(open(sys.argv[1])) if r['channel'].isdigit()]
print(f'{sys.argv[1]}: {sum(int(r["adc"]) > 0 for r in rows) / n:.3f} kept per event')
