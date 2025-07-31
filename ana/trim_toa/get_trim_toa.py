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
print('working on plotting toa_efficiency vs calib')
array = np.array([], dtype = 'int').reshape(0, 3)

# select only from calib [100, 600] and trim_toa [4, 20]
# data = data[(data['CALIB'] >= 00) & (data['CALIB'] <= 800) & (data['TRIM_TOA'] >= 0) & (data['TRIM_TOA'] <= 32)]

for trim in unique_trim_toas:
    print(f'on trim_toa = {trim}')
    current_trim_toa = data[data['TRIM_TOA'] == trim]
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize = (10, 5))
    for chan in range(36):
        toa_efficiency = []
        for calib in unique_calibs:
            current_calib = current_trim_toa[current_trim_toa['CALIB'] == calib]
            toa_triggers = len(current_calib[current_calib[str(chan)] != 0])
            chan_toa_efficiency = toa_triggers / len(current_calib)
            toa_efficiency.append(chan_toa_efficiency)
        ax1.plot(unique_calibs, toa_efficiency, label = f'CH_{chan}', linestyle = '-', marker = 'none')
        ax1.set_title('link 0')
        ax1.set_xlabel('calib')
        ax1.set_ylabel('toa_efficiency')
        for i in range(len(toa_efficiency)):
            if toa_efficiency[i] != 0.0:
                val = i
                break
            val = 0
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
plt.savefig('threshold_points.png')
plt.close()

print('saved figs')

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Third, let's calculate the ideal trim_toa value for each channel. Since our 
# regression gave us the calib as a function of trim_toa, we should use the equation of a
# line and solve for trim as a function of slope, offset, and calib. 
# calib = slope * trim_toa + offset
# trim_toa = (calib - offset) / slope
# Round everything to the nearest integer. 
# Finally, set the channel's trim_toa to that result.

# stats object holds the channel slope and intercept information

print('picking calib = 300 as desired value')
calib = 300
target_trim = []
# going to set any value not between 0 and 64 = 0.
for chan in range(72):
    if int((([calib] - stats[stats['channel'] == chan]['offset']) / stats[stats['channel'] == chan]['slope']).iloc[0]) < 0:
        target_trim.append(0)
    elif int((([calib] - stats[stats['channel'] == chan]['offset']) / stats[stats['channel'] == chan]['slope']).iloc[0]) > 64:
        target_trim.append(0)
    else:
        target_trim.append(int((([calib] - stats[stats['channel'] == chan]['offset']) / stats[stats['channel'] == chan]['slope']).iloc[0]))
        # target_trim.append(int((stats[stats['channel'] == chan]['slope'] * calib + stats[stats['channel'] == chan]['offset']).iloc[0]))

print(target_trim)

print('packing possible target_trim values into YAML')

df_max = pd.DataFrame({
    'channel': range(72),
    'trim_toa': target_trim
}
)

#output to yaml file
yaml_data = {}
for _, row in df_max.iterrows():
    page_name = f"CH_{int(row['channel'])}"
    yaml_data[page_name] = {
        'TRIM_TOA': int(row['trim_toa'])
    }

with open("output.yaml", "w") as f:
    yaml.dump(yaml_data, f, sort_keys=False)

print("Output saved to 'output.yaml'")