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
parser.add_argument('-ol', '--one_link', type=bool, help='chose if you want to only plot one link (True)')
args = parser.parse_args()

if args.output is None:
    if len(args.pedestals) == 1:
        args.output = args.pedestals[0].parent / (args.pedestals[0].stem + '-violinplot.png')
    else:
        args.output = 'violinplot.png'

if (args.one_link == True):
    fig, ax = plt.subplots(1, 1, figsize=(14, 8))
    ax.set_xlabel('Channel', fontsize=18)
    ax.set_ylabel('Median ADC [a.u.]', fontsize=18)

    labels = []
    for pedestals in args.pedestals:
        samples = pd.read_csv(pedestals)
        channels = ['calib']+[str(c) for c in range(36)]
        link_df = samples[(samples['i_link'] == 0)]
        samples = samples[samples['channel'] != 'calib']
        samples['channel'] = pd.to_numeric(samples['channel'])

        sorted_df = (samples
            .groupby(['channel'], as_index=False)
            .agg(adc=('adc', 'median'), std=('adc', 'std'))
            .sort_values(by='channel', ascending=True)
            .reset_index(drop=True))
        ax.errorbar(sorted_df['channel'], sorted_df['adc'], yerr=sorted_df['std'], 
                    fmt='o', markersize=10)
        ax.set_xticks(sorted_df['channel'])
        ax.tick_params(axis='both', which='both', labelsize=12)
        ax.grid(which='major', axis='y')
        fig.savefig('pedestals.png', dpi=400, bbox_inches='tight')

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
