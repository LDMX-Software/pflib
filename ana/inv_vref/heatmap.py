"""
Plots the median ADC per timepoint given a nr of samples per timepoint.

Plots:
    1D plots:
        inv_vref for constant noinv_vref
        noinv_vref for constant inv_vref
        Nearest neighbour derivatives for inv_vref with constant noinv_vref
        (constants set with the -pv argument)

    2D heatmap of inv_vref and noinv_vref

Takes a dataset which contains both inv_vref and noinv_vref parameters of all channels,
from vref_2d_scan in app/tool/tasks.
"""

import pandas as pd
import numpy as np
import hist
import matplotlib.pyplot as plt
from pathlib import Path
import argparse

parser = argparse.ArgumentParser()
parser.add_argument('pedestals', type=Path, help='decoded pedestal CSV file to summarize')
parser.add_argument('-pv', '--parameter-value', type=int, help='constant parameter values for 1D plots', default=620)
args = parser.parse_args()

samples = pd.read_csv(args.pedestals)

plt.rcParams['figure.figsize'] = [15, 12]
plt.rcParams['xtick.labelsize'] = 16
plt.rcParams['ytick.labelsize'] = 16

fig, ax = plt.subplots(1, 1)

samples = (samples
            .groupby(['noinv_vref', 'inv_vref', 'ch'], as_index=False)
            .agg(adc=('adc', 'median'), std=('adc', 'std')))

# Pedestals vs noinv
selected_df = samples[samples['inv_vref'] == args.parameter_value]

chs = selected_df.groupby('ch')
for ch_id, ch_df in chs:
    ax.errorbar(ch_df['noinv_vref'], ch_df['adc'], 
                    yerr=ch_df['std'], label=f'ch {ch_id}')
ax.legend(fontsize=12, ncols=4)
ax.set_ylabel('Median ADC [a.u.]', fontsize=24)
ax.set_xlabel(r'noinv$_{\text{vref}}$', fontsize=24)
ax.grid(which='major', axis='y')

plt.savefig('noinv_scan.png', dpi=400, bbox_inches='tight')
plt.close()

# Pedestals vs inv

fig, ax = plt.subplots(1, 1)

selected_df = samples[samples['noinv_vref'] == args.parameter_value]

chs = selected_df.groupby('ch')
for ch_id, ch_df in chs:
    ax.errorbar(ch_df['inv_vref'], ch_df['adc'], 
                    yerr=ch_df['std'], label=f'ch {ch_id}')
ax.legend(fontsize=12, ncols=4)
ax.set_ylabel('Median ADC [a.u.]', fontsize=24)
ax.set_xlabel(r'inv$_{\text{vref}}$', fontsize=24)
ax.grid(which='major', axis='y')

plt.savefig('inv_scan.png', dpi=400, bbox_inches='tight')
plt.close()

# Plot nearest neighbour differential

def compute_slopes(g):
    x = g['inv_vref'].to_numpy()
    y = g['adc'].to_numpy()
    y_dev = g['std'].to_numpy()

    LH = np.full(len(g), np.nan, dtype=float)
    RH = np.full(len(g), np.nan, dtype=float)
    LH_dev = np.full(len(g), np.nan, dtype=float)
    RH_dev = np.full(len(g), np.nan, dtype=float)

    LH[1:-1] = (y[1:-1] - y[:-2]) / (x[1:-1] - x[:-2])
    RH[1:-1] = (y[1:-1] - y[2:])  / (x[1:-1] - x[2:])
    LH_dev[1:-1] = (y_dev[1:-1] + y_dev[:-2])
    RH_dev[1:-1] = (y_dev[1:-1] + y_dev[2:])

    g = g.copy()
    g['LH'] = LH
    g['RH'] = RH
    g['LH-std'] = abs(LH_dev)
    g['RH-std'] = abs(RH_dev)
    return g

sorted_df = selected_df.sort_values(['ch', 'inv_vref']).copy()
result = (
    sorted_df
    .groupby('ch', group_keys=False)
    .apply(compute_slopes)
)

fig, ax = plt.subplots(1,1)
fig_err, ax_err = plt.subplots(2,1, sharex=True)

ch_group = result.groupby('ch')
for ch_id, ch_df in ch_group:
    ax_err[0].errorbar(ch_df['inv_vref'], ch_df['LH'], yerr=ch_df['LH-std'], fmt='o', label=f'ch {ch_id}')
    ax_err[1].errorbar(ch_df['inv_vref'], ch_df['LH'], yerr=ch_df['LH-std'], fmt='o', label=f'ch {ch_id}')
    ax.plot(ch_df['inv_vref'], ch_df['LH'], label=f'ch {ch_id}')

ax.legend(fontsize=12, ncols=6)
ax_err[0].legend(fontsize=10, ncols=8)

ax.set_xlabel(r'inv$_{\text{vref}}$', fontsize=32)
ax.set_ylabel(r'$\Delta ADC / \Delta \text{inv}_{\text{vref}}$ [a.u.]', fontsize=32)

ax_err[1].set_xlabel(r'inv$_{\text{vref}}$', fontsize=32)
fig_err.supylabel(r'$\Delta ADC / \Delta \text{inv}_{\text{vref}}$ [a.u.]', fontsize=32, x=0.04)
ax_err[1].set_ylim(-5,5)

fig_err.savefig('inv_derivs_with_err.png', dpi=400, bbox_inches='tight')
fig.savefig('inv_derivs.png', dpi=400, bbox_inches='tight')

plt.close()

# Heatmap

channels = samples.groupby('ch')

for ch_id, ch_df in channels:
    print("channel ", ch_id)
    fig = plt.figure()

    heat = (
        ch_df
        .groupby(['noinv_vref', 'inv_vref'])['adc']
        .median()
        .unstack('inv_vref')
        .sort_index()
        .sort_index(axis=1)
    )
    plt.imshow(
        heat.values,
        extent=[
            heat.columns.min(), heat.columns.max(),
            heat.index.min(), heat.index.max()
        ],
        origin="lower",
        aspect="auto",
        #cmap="tab20"
    )
    cbar = plt.colorbar()
    cbar.set_label('Median ADC [a.u.]', fontsize=32, labelpad=20)
    cbar.ax.tick_params(labelsize=16)
    plt.xlabel(r"inv$_{\text{vref}}$", fontsize=32)
    plt.ylabel(r"noinv$_{\text{vref}}$", fontsize=32)
    plt.tick_params(axis='both', labelsize=16) 
    plt.savefig(f'heatmap_ch_{ch_id}.png', dpi=400, bbox_inches='tight')
    plt.close()
