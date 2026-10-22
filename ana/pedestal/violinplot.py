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
parser.add_argument('--n-rocs', type=int, default=1, help='number of ROCs active during collection')
args = parser.parse_args()

if args.output is None:
    if len(args.pedestals) == 1:
        args.output = args.pedestals[0].parent / (args.pedestals[0].stem + '-violinplot.png')
    else:
        args.output = 'violinplot.png'

# inspect first pedestal file to see if we are in i_link/i_ch or roc/ch indexing
with open(args.pedestals[0]) as f:
    header = f.readline()

i_link_i_ch_indexing = 'i_link' in header
label = 'Link' if i_link_i_ch_indexing else 'ROC'
major_column = 'i_link' if i_link_i_ch_indexing else 'i_roc'
nrows = args.n_rocs*(2 if i_link_i_ch_indexing else 1)

fig, axes = plt.subplots(
    nrows=nrows,
    figsize=(12, 5*nrows),
    sharex=True,
    gridspec_kw = dict(
        hspace = 0.1
    )
)

labels = []
for pedestals in args.pedestals:
    samples = pd.read_csv(pedestals)
    ids = samples[major_column].unique()
    ids.sort()
    channels = (
        ['calib']+[str(c) for c in range(36)]
        if i_link_i_ch_indexing else
        ['calib_0','calib_1']+[str(c) for c in range(72)]
    )
    for i_row, ax in enumerate(axes):
        major_id = ids[i_row]
        ax.set_title(f'{label} {major_id}')
        art = ax.violinplot(
            [ samples[(samples[major_column]==major_id)&(samples.channel==ch)].adc for ch in channels],
        )
        if i_row == 0:
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
axes[-1].tick_params("x", rotation=90, rotation_mode="xtick")
axes[-1].set_xlabel('Channel')
axes[0].legend(*zip(*labels))
fig.savefig(args.output, bbox_inches='tight')
plt.clf()
