"""print a violin plot summarizing the adc values from the input pedestal CSV"""

import argparse
from pathlib import Path

import pandas as pd
import matplotlib.pyplot as plt

parser = argparse.ArgumentParser()
parser.add_argument('pedestals', type=Path, help='decoded pedestal CSV file to summarize')
parser.add_argument('-o','--output', type=Path, help='file to which to print, default is pedestal file with "-violinplot" suffix and extension changed to ".png"')
args = parser.parse_args()

samples = pd.read_csv(args.pedestals)

if args.output is None:
    args.output = args.pedestals.parent / (args.pedestals.stem + '-violinplot.png')

fig, axes = plt.subplots(
    nrows=2,
    figsize=(12,10),
    sharex=True,
    gridspec_kw = dict(
        hspace = 0.1
    )
)
channels = ['calib']+[str(c) for c in range(35)]
for link, ax in enumerate(axes):
    ax.violinplot(
        [ samples[(samples.i_link==link)&(samples.channel==ch)].adc for ch in channels]
    )
    ax.set_ylabel('ADC')
    ax.set_title(f'Link {link}')
    ax.grid()
axes[-1].set_xticks(
    [i+1 for i in range(len(channels))],
    channels
)
axes[-1].set_xlabel('Channel')
fig.suptitle(args.pedestals.stem)
fig.savefig(args.output, bbox_inches='tight')
plt.clf()
