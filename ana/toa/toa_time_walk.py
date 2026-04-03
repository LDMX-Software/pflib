"""
Plots the the toa time walk for all channels on each link 
Written for use of data from channel_wise_calib_scan.cxx in tasks

Extra args:
    - "-st" "--style": Allows the user to include a matplotlib style sheet
    - "-pi" "--plot_individually": Plot the time walk for each channel in
                                    individual plots.
"""

import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
from pathlib import Path
import argparse
import matplotlib.colors
import matplotlib.cm as cm
from mpl_toolkits.axes_grid1.inset_locator import inset_axes

parser = argparse.ArgumentParser()
parser.add_argument('data', type=Path, help='decoded CSV file to analyze')
parser.add_argument("-st", "--style", type=Path,
                    help='Adress of matplotlib style sheet')
parser.add_argument("-pi", "--plot_individually", type=bool,
                    help='plot and save each time walk individually')
args = parser.parse_args()

samples = pd.read_csv(args.data)

if (args.style != None):
    plt.style.use(args.style)

def toa_conv(tdc):
    return (tdc / 1024) * 25

fig, ax = plt.subplots(1,2,sharey=True,sharex=True)
ax0 = ax[0]; ax1 = ax[1]

cmap = plt.get_cmap('plasma')
ana = samples[(samples['time'] == 25.0) & (samples['toa'] != 0.0)]
chs = ana.groupby('channel')
for ch_id, ch_df in chs:
    ch_df = pd.DataFrame(ch_df)
    print(ch_id)
    df = (ch_df
          .groupby('calib', as_index=False)
          .agg(
                toa_median=('toa', 'median'),
                toa_std=('toa', 'std')
          )
    )
    if (ch_id < 36):
        color = cmap(ch_id/36)
        ax0.plot(df['calib'], toa_conv(df['toa_median']), color=color)
    else:
        color = cmap((ch_id-36)/36)
        ax1.plot(df['calib'], toa_conv(df['toa_median']), color=color)
    if (args.plot_individually):
        figi, axi = plt.subplots(1,1)
        axi.errorbar(df['calib'], toa_conv(df['toa_median']), 
                                yerr=toa_conv(df['toa_std']),
                                label=f'channel {ch_id}')
        axi.set_ylabel('TOA [ns]')
        axi.set_xlabel('CALIB [a.u.]')
        axi.legend()
        figi.savefig(f'toa_time_walk_channel_{ch_id}.png')
        plt.close()

fig.supxlabel('CALIB [a.u.]')
ax0.set_ylabel('TOA [ns]')
ax0.set_title('link0')
ax1.set_title('link1')

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

fig.savefig("toa_time_walk.png")
