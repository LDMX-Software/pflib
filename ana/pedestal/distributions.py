"""Plot the distribution of pedestals on both links"""

import argparse
from pathlib import Path

import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
from matplotlib.patches import Patch

parser = argparse.ArgumentParser()
parser.add_argument('pedestals', type=Path, nargs='+', help='decoded pedestal CSV file to summarize')
parser.add_argument('-st', '--style_sheet', type=Path, help='Style sheet for plots in .mplstyle format')
args = parser.parse_args()

if args.style_sheet:
    plt.style.use(args.style_sheet)

samples = pd.read_csv(args.pedestals[0])

link0 = samples[(samples['i_link'] == 0) & (samples['channel'] != 'calib')]
link1 = samples[(samples['i_link'] == 1) & (samples['channel'] != 'calib')]

fig, ax = plt.subplots(1,2,sharey=True)
ax0 = ax[0]; ax1 = ax[1]

cmap = plt.get_cmap('plasma')
bins = np.array([(i+0.5) for i in range(0,1024)])

for i in range(36):
    color = cmap(i/36)
    ax0.hist(link0[link0['channel'] == str(i)]['adc'], bins=bins, color=color)
    ax1.hist(link1[link1['channel'] == str(i)]['adc'], bins=bins, color=color)

fig.supxlabel('ADC [a.u.]')
ax0.set_ylabel('Counts')
ax0.set_title('link0')
ax1.set_title('link1')
ax0.set_xlim(link0['adc'].min()-3, link0['adc'].max()+3)
ax1.set_xlim(link1['adc'].min()-3, link1['adc'].max()+3)

filename = 'pedestal_distribution.png'
fig.savefig(filename)
print("Plot saved to", filename)

