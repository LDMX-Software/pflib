"""heatmap of pedestal data separated by link_channel"""

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

fig, axes = plt.subplots(1, 2)

# pedestal per link vs noinv

link0_df = samples[(samples['ch'] < 32) & (samples['inv_vref'] == 600)]
link1_df = samples[(samples['ch'] >= 32) & (samples['inv_vref'] == 600)]

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

link0_df = samples[(samples['ch'] < 32) & (samples['noinv_vref'] == 600)]
link1_df = samples[(samples['ch'] >= 32) & (samples['noinv_vref'] == 600)]

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
