"""print a violin plot summarizing the adc values from the input pedestal CSV"""

import argparse
from pathlib import Path

import pandas as pd
import matplotlib.pyplot as plt
import matplotlib as mpl

parser = argparse.ArgumentParser()
parser.add_argument('pedestals', type=Path, nargs='+', help='decoded pedestal CSV file to summarize')
parser.add_argument('-o','--output', type=Path, help='file to which to print, default is pedestal file with "-violinplot" suffix and extension changed to ".png"')
parser.add_argument('--ylim', nargs=2, type=int, default=[100,300], help='limits of y-axis to use')
args = parser.parse_args()

if args.output is None:
    if len(args.pedestals) == 1:
        args.output = args.pedestals.parent / (args.pedestals.stem + '-violinplot.png')
    else:
        args.output = 'violinplot.png'

fig, axes = plt.subplots(
    nrows=2,
    figsize=(12,10),
    sharex=True,
    gridspec_kw = dict(
        hspace = 0.1
    )
)

labels = []
for pedestals in args.pedestals:
    samples = pd.read_csv(pedestals)
    channels = ['calib']+[str(c) for c in range(36)]
    for link, ax in enumerate(axes):
        ax.set_title(f'Link {link}')
        art = ax.violinplot(
            [ samples[(samples.i_link==link)&(samples.channel==ch)].adc for ch in channels],
        )
        if link == 0:
            labels.append((
                mpl.patches.Patch(color=art['bodies'][0].get_facecolor().flatten()),
                pedestals.stem))

for ax in axes:
    ax.grid()
    ax.set_ylabel('ADC')
    ax.set_ylim(*args.ylim)
    
axes[-1].set_xticks(
    [i+1 for i in range(len(channels))],
    channels
)
axes[-1].set_xlabel('Channel')
axes[0].legend(*zip(*labels))
fig.savefig(args.output, bbox_inches='tight')
plt.clf()
