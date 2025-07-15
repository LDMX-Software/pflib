# Calculates TOA_EFFICIENCY for a given dataset,
# requires a "toa_vref_scan.csv" file to run correctly.
# Then, plots the efficiency versus toa_vref and saves the plot.

import matplotlib.pyplot as plt
import os
import argparse
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).parent.parent))

from read import read_pflib_csv

#runtime arguments
parser = argparse.ArgumentParser(
    prog = 'toa_efficiency.py',
    description='takes a csv from tasks.toa_vref_scan and outputs a plot of TOA efficiency per channel against TOA VREF values'
)
parser.add_argument('-f', required = True, help='csv file containing scan from tasks.toa_vref_scan')
args = parser.parse_args()

if not os.path.isfile(args.f):
    print(args.f + ' does not exist')
    sys.exit()
if not args.f.lower().endswith('csv'):
    print(args.f + ' is not a csv file')
    sys.exit()
data, head = read_pflib_csv(args.f)

"""
Example plot of how TOA_VREF plot should look. I credit the toa_vref_analysis.py
script from the TileboardQC Repository for HGCAL Tileboard Quality Control for giving
me the idea to put a comment of the graph in the script.

toa_efficiency vs Toa_vref
|      + +
|      +++
|      +++
|      +++   
|      + + This is the value of toa vref we calculate
|      ++  ^
|______+++_|_________
|____________________

We want the TOA_VREF for each link to be just a bit higher than the pedestal variations
for that link. That way, we ensure only incoming signals trigger TOA, and we maximize
the signal-to-noise ratio.
"""

channel_lists = []
for chan in range(72):
    channel_lists.append([])

unique_toa_vrefs = data['TOA_VREF'].unique()

for toa_vref in unique_toa_vrefs:
    filtered_data = data[data['TOA_VREF'] == toa_vref]
    for chan in range(72):
        non_zero = filtered_data[filtered_data[str(chan)] != 0]
        chan_triggers = len(non_zero)
        chan_toa_efficiency = chan_triggers / len(filtered_data)
        channel_lists[chan].append(chan_toa_efficiency)

# Plotting the results
plt.figure(figsize=(10, 6))
for chan in range(72):
    plt.plot(channel_lists[chan], 
             marker = 'o', 
             label=f'Ch. {chan}', 
             linestyle='none')
plt.title('TOA Efficiency vs TOA VREF per channel')
plt.xlabel('TOA VREF')
plt.ylabel('TOA Efficiency')
plt.tight_layout()
plt.savefig('toa_efficiency_plot.png')
plt.show()
print("Plot saved as 'toa_efficiency_plot.png'")