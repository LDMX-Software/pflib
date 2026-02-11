"""
heatmap of pedestal data separated by link_channel

Given a dataset which contains both inv_vref and noinv_vref parameters (pedestal scan),
gives two 1D plots of ADC vs each parameters, and a 2D heatmaps of the parameters and ADC.

"""

import pandas as pd
import numpy as np
import hist
import matplotlib.pyplot as plt
from pathlib import Path
import argparse

parser = argparse.ArgumentParser()
parser.add_argument('pedestals', type=Path, help='decoded pedestal CSV file to summarize')
parser.add_argument('-o','--output', type=Path, help='file to which to print, default is pedestal file with "-heatmap" suffix and extension changed to ".png"')
parser.add_argument('--full-range', action='store_true', help='use full 10-bit range rather than dynamically change the binning to fit between the min/max of the data')
args = parser.parse_args()

if args.output is None:
    args.output = args.pedestals.parent / (args.pedestals.stem + '-heatmap.png')

samples = pd.read_csv(args.pedestals)

print(samples.columns.values)

fig, axes = plt.subplots(1, 2)

# pedestal per link vs noinv

link0_df = samples[(samples['ch'] < 32) & (samples['inv_vref'] == 620)]
link1_df = samples[(samples['ch'] >= 32) & (samples['inv_vref'] == 620)]

median_adc_0 = link0_df.groupby('noinv_vref')['adc'].median().sort_index()
median_adc_1 = link1_df.groupby('noinv_vref')['adc'].median().sort_index()

axes[0].plot(median_adc_0.index, median_adc_0.values)
axes[1].plot(median_adc_1.index, median_adc_1.values)
axes[0].set_ylabel('median pedestal [ADC]')
axes[0].set_xlabel('noinv_vref')
axes[1].set_xlabel('noinv_vref')
axes[0].set_title('link 0')
axes[1].set_title('link 1')

plt.savefig('noinv_scan.png', dpi=400, bbox_inches='tight')
plt.close()

# pedestal per link vs inv

fig, axes = plt.subplots(1, 2)

link0_df = samples[(samples['ch'] < 32) & (samples['noinv_vref'] == 620)]
link1_df = samples[(samples['ch'] >= 32) & (samples['noinv_vref'] == 620)]

median_adc_0 = link0_df.groupby('inv_vref')['adc'].median().sort_index()
median_adc_1 = link1_df.groupby('inv_vref')['adc'].median().sort_index()

axes[0].plot(median_adc_0.index, median_adc_0.values)
axes[1].plot(median_adc_1.index, median_adc_1.values)
axes[0].set_ylabel('median pedestal [ADC]')
axes[0].set_xlabel('inv_vref')
axes[1].set_xlabel('inv_vref')
axes[0].set_title('link 0')
axes[1].set_title('link 1')

plt.savefig('inv_scan.png', dpi=400, bbox_inches='tight')
plt.close()

# LH and RH derivatives

fig, axes = plt.subplots(2, 2, height_ratios=[2, 1], sharex=True)

LH_deriv = []
RH_deriv = []
x_vals = []

for i in range(1,len(median_adc_0.index)-1):
    LH = (median_adc_0.values[i] - median_adc_0.values[i-1]) / (median_adc_0.index[i] - median_adc_0.index[i-1])
    RH = (median_adc_0.values[i] - median_adc_0.values[i+1]) / (median_adc_0.index[i] - median_adc_0.index[i+1]) 
    LH_deriv.append((LH))
    RH_deriv.append((RH))
    x_vals.append(median_adc_0.index[i])

axes[0][0].plot(x_vals, LH_deriv)
axes[0][1].plot(x_vals, RH_deriv)
fig.supylabel('Difference to nearest neighbour [ADC]')
axes[0][0].set_title('LH')
axes[0][1].set_title('RH')

axes[1][0].plot(x_vals, LH_deriv)
axes[1][1].plot(x_vals, RH_deriv)
axes[1][0].set_xlabel('inv_vref')
axes[1][0].set_xlabel('inv_vref')
axes[1][0].set_ylim(-2,2)
axes[1][1].set_ylim(-2,2)

plt.savefig('inv_derivs.png', dpi=400, bbox_inches='tight')
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
    plt.colorbar(label="median pedestal [ADC]")
    plt.xlabel("inv_vref")
    plt.ylabel("noinv_vref")
    plt.savefig(f'heatmap_ch_{ch_id}.png', dpi=400, bbox_inches='tight')
    plt.close()
