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
parser.add_argument('filepath', type=Path, help='JSON file with histograms to plot')
parser.add_argument('-o', '--output', type=Path, help='output file to save image into')
parser.add_argument('--error-bars', action='store_true', help='show error bars in plot to represent statistical uncertainty')
args = parser.parse_args()

if args.output is None:
    args.output = args.filepath.stem + '.png'

# data is either a list of histograms for the different STC sums (output by TRIG.HISTO.DUMP)
# or a single STC sum histogram dictionary (output by TRIG.HISTO.READ)
with open(args.filepath) as file:
    data = json.load(file, object_hook=uhi.io.json.object_hook) 

h = hist.Hist(data)
h.plot(yerr = args.error_bars)
plt.yscale('log')
plt.ylabel('Events / bin')
plt.savefig(args.output, bbox_inches='tight')
