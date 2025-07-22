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
samples['uid'] = samples['i_link'].astype(str)+'_'+samples['channel']

fig, axes = plt.subplots(
    nrows = 3,
    sharex = True,
    figsize = (10,8),
)

for ax, xname, xlabel in zip(
    axes,
    ('adc', 'adc_tm1', 'toa'),
    ('ADC', 'ADC t-1', 'TOA')
):
    xmax = 1023 if args.full_range else samples[xname].max()
    xmin = 0 if args.full_range else samples[xname].min()
    (
        hist.Hist.new
        .StrCategory(samples['uid'].unique())
        .Reg(xmax-xmin+1,xmin,xmax+1, label=xlabel)
        .Double()
    ).fill(
        samples['uid'],
        samples[xname]
    ).plot2d(
        ax=ax,
        cmin=1,
        cbarsize=0.2,
        cbarextend=False
    ).cbar.set_label('Counts')
    ax.set_xlabel(None)

axes[-1].set_xlabel('Link_Channel')
plt.xticks(rotation=90, size='x-small')
axes[0].set_title(args.pedestals.stem)
fig.savefig(args.output, bbox_inches='tight')
plt.clf()
