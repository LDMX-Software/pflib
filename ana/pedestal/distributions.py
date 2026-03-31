"""Plot the distribution of pedestals on both links"""

import argparse
from pathlib import Path

import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.colors
import matplotlib.cm as cm
from mpl_toolkits.axes_grid1.inset_locator import inset_axes

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

norm = matplotlib.colors.Normalize(vmin=0, vmax=35)
sm = cm.ScalarMappable(norm=norm, cmap=cmap)
sm.set_array([])
axins0 = inset_axes(ax0, width='20%', height='4%', loc='upper right', borderpad=1.4)
cbar0 = fig.colorbar(sm, cax=axins0, orientation='horizontal', ticks=[0, 35])
cbar0.ax.set_xticklabels(['0', '35'])
axins1 = inset_axes(ax1, width='20%', height='4%', loc='upper right', borderpad=1.4)
cbar1 = fig.colorbar(sm, cax=axins1, orientation='horizontal', ticks=[0, 35])
cbar1.ax.set_xticklabels(['0', '35'])
axins0.xaxis.set_ticks_position('bottom')
axins1.xaxis.set_ticks_position('bottom')
axins0.set_title('channels', fontsize=10)
axins1.set_title('channels', fontsize=10)

filename = 'pedestal_distribution.png'
fig.savefig(filename)
print("Plot saved to", filename)

