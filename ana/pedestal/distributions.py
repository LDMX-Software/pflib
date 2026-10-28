"""Plot the distribution of pedestals on each link or ROC"""

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
samples['channel'] = samples['channel'].astype(str)

# i_link/i_ch or roc/ch indexing
i_link_i_ch_indexing = 'i_link' in samples.columns
label = 'link' if i_link_i_ch_indexing else 'ROC '
major_column = 'i_link' if i_link_i_ch_indexing else 'i_roc'
n_ch = 36 if i_link_i_ch_indexing else 72

ids = np.sort(samples[major_column].unique())
fig, ax = plt.subplots(1,len(ids),sharey=True,squeeze=False,figsize=(3.2*len(ids),4.8))
ax = ax[0]

cmap = plt.get_cmap('plasma')
bins = np.array([(i+0.5) for i in range(0,1024)])
norm = matplotlib.colors.Normalize(vmin=0, vmax=n_ch-1)
sm = cm.ScalarMappable(norm=norm, cmap=cmap)
sm.set_array([])

for a, i in zip(ax, ids):
    group = samples[(samples[major_column] == i) & (~samples['channel'].str.startswith('calib'))]
    for ch in range(n_ch):
        a.hist(group[group['channel'] == str(ch)]['adc'], bins=bins, color=cmap(ch/n_ch))
    a.set_title(f'{label}{i}')
    a.set_xlim(group['adc'].min()-3, group['adc'].max()+3)

    axins = inset_axes(a, width='20%', height='4%', loc='upper right', borderpad=1.4)
    cbar = fig.colorbar(sm, cax=axins, orientation='horizontal', ticks=[0, n_ch-1])
    cbar.ax.set_xticklabels(['0', str(n_ch-1)])
    axins.xaxis.set_ticks_position('bottom')
    axins.set_title('channels', fontsize=10)

fig.supxlabel('ADC [a.u.]')
ax[0].set_ylabel('Counts')

filename = 'pedestal_distribution.png'
fig.savefig(filename)
print("Plot saved to", filename)

