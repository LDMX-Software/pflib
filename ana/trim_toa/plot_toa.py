"""
Script to plot TOA versus CALIB values for each channel. Outputs 1 multi-plot.
Input is 2 CSV files from toa_scan: first is default settings and second is after alignment.
Use this to check before/after the trim_toa_scan and align_trim_toa.py.
"""

import pandas as pd
import matplotlib.pyplot as plt
import argparse
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).parent.parent))

#runtime arguments, read data
parser = argparse.ArgumentParser(
    prog = 'plot_toa.py',
    description='takes 2 CSVs from tasks.toa_scan and outputs plots of TOA efficiency versus calib and TOA versus calib for default and aligned scans.'
)
parser.add_argument('toa_scans', type=Path, nargs='+', help='2 CSV files from tasks.toa_scan to be plotted. First is default, second is aligned.')
args = parser.parse_args()

# Plotting the data sets.

default = pd.read_csv(
    args.toa_scans[0],
    skiprows = 1
)
aligned = pd.read_csv(
    args.toa_scans[1],
    skiprows = 1
)

# setting parameters takes time away during your scans! your bx.
fig, axes = plt.subplots(2, 2, figsize = (10, 10))
axes = axes.flatten()
for chan in range(36):
    axes[0].plot(default['CALIB'], default[str(chan)])
    axes[1].plot(aligned['CALIB'], aligned[str(chan)])
for chan in range(36, 72):
    axes[2].plot(default['CALIB'], default[str(chan)])
    axes[3].plot(aligned['CALIB'], aligned[str(chan)])
for ax in axes[0:4]:
    ax.set_xlabel('calib')
    ax.set_ylabel('toa')
    ax.grid()
axes[0].set_title('link 0 default')
axes[1].set_title('link 0 aligned')
axes[2].set_title('link 1 default')
axes[3].set_title('link 1 aligned')
plt.suptitle('TOA VERSUS CALIB')
plt.tight_layout()
plt.savefig('toa_comparison.png')
plt.close()

fig, axes = plt.subplots(2, 2, figsize = (10, 10))
axes = axes.flatten()
unique_default_calibs = default['CALIB'].unique()
unique_aligned_calibs = aligned['CALIB'].unique()
for chan in range(36):
    toa_efficiency = []
    for calib in unique_default_calibs:
        current_calib = default[default['CALIB'] == calib]
        toa_triggers = len(current_calib[current_calib[str(chan)] != 0])
        chan_toa_efficiency = toa_triggers / len(current_calib)
        toa_efficiency.append(chan_toa_efficiency)
    axes[0].plot(unique_default_calibs, toa_efficiency, label = f'CH_{chan}', linestyle = '-', marker = 'none')
    toa_efficiency = []
    for calib in unique_aligned_calibs:
        current_calib = aligned[aligned['CALIB'] == calib]
        toa_triggers = len(current_calib[current_calib[str(chan)] != 0])
        chan_toa_efficiency = toa_triggers / len(current_calib)
        toa_efficiency.append(chan_toa_efficiency)
    axes[1].plot(unique_aligned_calibs, toa_efficiency, label = f'CH_{chan}', linestyle = '-', marker = 'none')
for chan in range(36, 72):
    toa_efficiency = []
    for calib in unique_default_calibs:
        current_calib = default[default['CALIB'] == calib]
        toa_triggers = len(current_calib[current_calib[str(chan)] != 0])
        chan_toa_efficiency = toa_triggers / len(current_calib)
        toa_efficiency.append(chan_toa_efficiency)
    axes[2].plot(unique_default_calibs, toa_efficiency, label = f'CH_{chan}', linestyle = '-', marker = 'none')
    toa_efficiency = []
    for calib in unique_aligned_calibs:
        current_calib = aligned[aligned['CALIB'] == calib]
        toa_triggers = len(current_calib[current_calib[str(chan)] != 0])
        chan_toa_efficiency = toa_triggers / len(current_calib)
        toa_efficiency.append(chan_toa_efficiency)
    axes[3].plot(unique_aligned_calibs, toa_efficiency, label = f'CH_{chan}', linestyle = '-', marker = 'none')
axes[0].set_title('link 0 default')
axes[1].set_title('link 0 aligned')
axes[2].set_title('link 1 default')
axes[3].set_title('link 1 aligned')
for ax in axes:
    ax.set_xlabel('calib')
    ax.set_ylabel('toa_efficiency')
    ax.grid()
plt.suptitle('TOA EFFICIENCY VERSUS CALIB')
plt.tight_layout()
plt.savefig('efficiency_comparison.png')
plt.close()

print('plots saved')