"""plot histograms readout from firmware and saved into UHI JSON

The JSON serializability in UHI was standardized in v1 (I believe),
so you may need to update your local Python environment to get access
to the `uhi.io` module. I tested this with scikit-hep 2026.7.1.
"""

from pathlib import Path
import json
import hist
import uhi.io.json
import matplotlib.pyplot as plt
import argparse

parser = argparse.ArgumentParser()
parser.add_argument('filepath', type=Path, help='JSON file from TRIG.DUMP or TRIG.READ to plot')
parser.add_argument('-o', '--output', type=Path, help='output file to save image into')
parser.add_argument('--error-bars', action='store_true', help='show error bars in plot to represent statistical uncertainty')
parser.add_argument('--stc', type=int, help='only plot input STC if given')
parser.add_argument('--roc-adc-th', type=int, help='digitalhalf_#.adc_th setting on the ROCs being used')
parser.add_argument('--trigger-th', type=int, help='trigger threshold being used')
parser.add_argument('--raw-counts', action='store_true', help='plot raw counts instead of scaling by collection time to estimate the rate')
args = parser.parse_args()

if args.output is None:
    args.output = args.filepath.stem + '.png'

with open(args.filepath) as file:
    data = json.load(file, object_hook=uhi.io.json.object_hook) 

h = hist.Hist(data)
if not args.raw_counts:
    h /= data['metadata']['collection_time']
if args.stc is not None and len(h.axes) > 1:
    h[f'STC{args.stc}',:].plot(yerr = args.error_bars)
    plt.annotate(
        f'STC{args.stc}',
        (0.95, 0.95),
        xycoords = 'axes fraction',
        ha = 'right',
        va = 'top',
    )
else:
    h.plot(yerr = args.error_bars)

vlines = []
if args.roc_adc_th is not None:
    vlines.append((args.roc_adc_th, 'roc.adc_th', 'lightgray'))
if args.trigger_th is not None:
    vlines.append((args.trigger_th, 'trigger_th', 'gray'))
for val, name, color in vlines:
    plt.axvline(val, color = color)
    plt.text(
        val, h['STC0',4j], name,
        ha = 'right',
        va = 'top',
        rotation = 90,
    )
plt.yscale('log')
if args.raw_counts:
    plt.ylabel('Events / bin')
else:
    plt.ylabel('Rate / Hz')
plt.savefig(args.output, bbox_inches='tight')
