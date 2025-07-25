"""
going to be the final script for how to get the fitted trim_toa values from a trim_toa_scan on pflib tool

TOA efficiency is calculated as the ratio of the number of toa triggers to the number of events.
We will calculate this for each unique element in the parameter space (channel number, calib, trim_toa).

Example plot of how the trim_toa_scan should look.
This should be shaped like a logistic curve, with
higher values of calib yielding a higher efficiency
closer to 1.

For each channel & trim_toa value:

toa_efficiency
        ^
        |       __________
        |      /
        |      |
        |      |
        |      |
        |_____/   <- turn-on point
        +-------------------> calib

Ideally, we're looking for the calib value that
corresponds to the start of non-zero toa_efficiency
for each channel's trim_toa value.
"""
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# import data

import pandas as pd
import numpy as np
import yaml
import matplotlib.pyplot as plt
import os
import argparse
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).parent.parent))
from scipy.stats import linregress as lr
import matplotlib as mpl

from read import read_pflib_csv

#runtime arguments, read data
parser = argparse.ArgumentParser(
    prog = 'get_trim_toa.py',
    description='Takes a csv from tasks.trim_toa_scan and outputs plots of TOA efficiency per channel and trim_toa against calib values. \n' \
    'Also plots threshold, or turnaround, points for each channel for given calib and trim_toa.'
)
parser.add_argument('-f', required = True, help='csv file containing scan from tasks.trim_toa_scan')
args = parser.parse_args()

if not os.path.isfile(args.f):
    print(args.f + ' does not exist')
    sys.exit()
if not args.f.lower().endswith('csv'):
    print(args.f + ' is not a csv file')
    sys.exit()
data, head = read_pflib_csv(args.f)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# First, plot TOA_EFFICIENCY as a function of CALIB for each e-link.
# Will simultaneously calculate the threshold points.

unique_trim_toas = data['TRIM_TOA'].unique()
unique_calibs = data['CALIB'].unique()
# trim = unique_trim_toas[0]
print('working on plotting toa_efficiency vs calib')
array = np.array([], dtype = 'int').reshape(0, 3)
for trim in unique_trim_toas:
    print(f'on trim_toa = {trim}')
    current_trim_toa = data[data['TRIM_TOA'] == trim]
    # for chan in range(36, 72):
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize = (10, 5))
    for chan in range(36):
        toa_efficiency = []
        for calib in unique_calibs:
            current_calib = current_trim_toa[current_trim_toa['CALIB'] == calib]
            toa_triggers = len(current_calib[current_calib[str(chan)] != 0])
            chan_toa_efficiency = toa_triggers / len(current_calib)
            toa_efficiency.append(chan_toa_efficiency)
        # plt.plot(unique_calibs, toa_efficiency, label=f'CH_{chan}', linestyle = '-', marker = 'none')
        ax1.plot(unique_calibs, toa_efficiency, label = f'CH_{chan}', linestyle = '-', marker = 'none')
        ax1.set_title('link 0')
        ax1.set_xlabel('calib')
        ax1.set_ylabel('toa_efficiency')
        # plt.plot(current_trim_toa['CALIB'], current_trim_toa['0'], linestyle = 'none', marker = 'o', alpha = .1)
        for i in range(len(toa_efficiency)):
            if toa_efficiency[i] != 0.0:
                val = i
                break
        # append the data to the array
        new_row = np.array([[trim, unique_calibs[val], chan]])
        array = np.concatenate((array, new_row), axis = 0)
    for chan in range(36, 72):
        toa_efficiency = []
        for calib in unique_calibs:
            current_calib = current_trim_toa[current_trim_toa['CALIB'] == calib]
            toa_triggers = len(current_calib[current_calib[str(chan)] != 0])
            chan_toa_efficiency = toa_triggers / len(current_calib)
            toa_efficiency.append(chan_toa_efficiency)
        # plt.plot(unique_calibs, toa_efficiency, label=f'CH_{chan}', linestyle = '-', marker = 'none')
        ax2.plot(unique_calibs, toa_efficiency, label = f'CH_{chan}', linestyle = '-', marker = 'none')
        ax2.set_title('link 1')
        ax2.set_xlabel('calib')
        ax2.set_ylabel('toa_efficiency')
        for i in range(len(toa_efficiency)):
            if toa_efficiency[i] != 0.0:
                val = i
                break
        # append the data to the array
        new_row = np.array([[trim, unique_calibs[val], chan]])
        array = np.concatenate((array, new_row), axis = 0)
    plt.suptitle(f'TOA EFFICIENCY vs CALIB for TRIM_TOA = {trim}, all channels', size=16)
    ax1.grid(True)
    plt.grid()
    plt.tight_layout()
    plt.savefig(f'toa_efficiency_TRIM_TOA_{trim}.png')
    # plt.show()
    plt.close()  # close the plot to avoid displaying it immediately
print('plots saved')

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Second, plot the threshold point for each channel at each trim_toa. This is the calib at which
# the graph of toa_efficiency first begins to slope upward. We already calculated this in "array" before.
# Additionally, we want to observe linear behavior, so we'll throw in a linear fit.

# for linear regression of data points

print('working on threshold plots')

stats = pd.DataFrame(columns=['channel', 'slope', 'offset'])
# Want colors to match lines and dots, so need a defined sequence. But, matplotlib's longest
# is only 20, so I'm just concatenating 4 of them into a list of 80, so I don't go index out
# of range.
colors = mpl.color_sequences['tab20'] + mpl.color_sequences['tab20'] + mpl.color_sequences['tab20'] + mpl.color_sequences['tab20']

fig, (ax1, ax2) = plt.subplots(1, 2, figsize = (10, 5))
for chan in range(36):
    chan_data = array[array[:, 2] == chan] # picks out the third column
    ax1.plot(chan_data[:, 0], chan_data[:, 1], linestyle = 'none', marker = 'o', label = 'CH_'+str(chan), color = colors[chan])
    calib_2v5 = (chan_data[:,1])
    trim_toa = (chan_data[:,0])
    slope, offset, r_value, p_value, slope_err = lr(trim_toa, calib_2v5)
    stats.loc[len(stats)] = [chan, slope, offset]
    # plt.plot(trim_toa, slope * trim_toa + offset, color = colors[chan])
    ax1.plot(trim_toa, slope * trim_toa + offset, color = colors[chan])
for chan in range(36, 72):
    chan_data = array[array[:, 2] == chan] # picks out the third column
    ax2.plot(chan_data[:, 0], chan_data[:, 1], linestyle = 'none', marker = 'o', label = 'CH_'+str(chan), color = colors[chan])
    calib_2v5 = (chan_data[:,1])
    trim_toa = (chan_data[:,0])
    slope, offset, r_value, p_value, slope_err = lr(trim_toa, calib_2v5)
    stats.loc[len(stats)] = [chan, slope, offset]
    ax2.plot(trim_toa, slope * trim_toa + offset, color = colors[chan])
ax1.set_title('link 0')
ax2.set_title('link 1')
ax1.set_ylabel('calib')
ax2.set_ylabel('calib')
ax1.set_xlabel('trim_toa')
ax2.set_xlabel('trim_toa')
ax1.grid(True)
plt.grid()
plt.suptitle("TRIM_TOA vs CALIB for each channel's toa turnaround points\nand linear fits")
# plt.legend(loc = 'upper left', bbox_to_anchor = (1, 1.15))
plt.savefig('threshold_points.png')
# plt.show()
plt.close()
# Goal is to see a descending line for each channel. But I'm kinda not seeing that yet.

print('saved figs')