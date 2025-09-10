"""
This script inputs a CSV file the results from a trim_toa_scan.
The script outputs a plot of TOA efficiency as a function of calib,
a plot of the threshold points for each channel at each trim_toa, and
finally a YAML file with the ideal trim_toa value for each channel.

Currently, the script is set to use a calib value of 25, but you can 
change it to any value you want.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TOA efficiency is calculated as the ratio of the number of toa triggers to the number of events.
We perform this calculation for each channel and at each trim_toa value.
Then, we find the threshold points as the first point for each channel where the TOA efficiency
is non-zero. 

TODO: We can try to improve the threshold point calculation by doing linear interpolation on 
all the points with non-zero TOA efficiency for each channel to get the average center (50%)
point. Or, we could consider just doing number of events = 1 for the trim_toa_scan, so that we
can just take the first non-zero TOA efficiency point as the threshold point, since there's only
1 point. If the simple way works, we can just do that.

Example plot of how the trim_toa_scan should look.
This should be shaped like the letter S, with
higher values of calib yielding a higher efficiency
close to or equal to 1.

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
for each channel's trim_toa value. Again, we get the 
data for this from the trim_toa_scan CSV file.
"""

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
from scipy.stats import siegelslopes as ss
import matplotlib as mpl

from read import read_pflib_csv

#runtime arguments, read data
parser = argparse.ArgumentParser(
    prog = 'get_trim_toa.py',
    description='Takes a csv from tasks.trim_toa_scan and outputs plots of TOA efficiency per channel and trim_toa against calib values. \n' \
    'Also plots threshold, or turnaround, points for each channel for given calib and trim_toa. Outputs YAML with ideal trim_toa values.'
)
parser.add_argument('-f', required = True, help='csv file containing scan from tasks.trim_toa_scan')
parser.add_argument('-c', default = 100, type = int, help='target lowrange calib value to which we align the trim_toa; default is calib = 100.')
parser.add_argument('-p', action = 'store_true', help = 'if included, saves efficiency and threshold plots. Otherwise, just saves YAML file.')
args = parser.parse_args()

if not os.path.isfile(args.f):
    print(args.f + ' does not exist')
    sys.exit()
if not args.f.lower().endswith('csv'):
    print(args.f + ' is not a csv file')
    sys.exit()

target_calib = args.c
do_plots = args.p
data, head = read_pflib_csv(args.f)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# First, plot TOA_EFFICIENCY as a function of CALIB for each e-link.
# Will simultaneously calculate the threshold points.

unique_trim_toas = data['TRIM_TOA'].unique()
unique_calibs = data['CALIB'].unique()
array = np.array([], dtype = 'int').reshape(0, 3)
if do_plots:
    print('plotting toa_efficiency vs calib')
print('locating threshold points')
for trim in unique_trim_toas:
    print(f'on trim_toa = {trim}')
    current_trim_toa = data[data['TRIM_TOA'] == trim]
    if do_plots:
        fig, (ax1, ax2) = plt.subplots(1, 2, figsize = (10, 5))
    for chan in range(36):
        toa_efficiency = []
        for calib in unique_calibs:
            current_calib = current_trim_toa[current_trim_toa['CALIB'] == calib]
            toa_triggers = len(current_calib[current_calib[str(chan)] != 0])
            chan_toa_efficiency = toa_triggers / len(current_calib)
            toa_efficiency.append(chan_toa_efficiency)
        if do_plots:
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
        if do_plots:
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
    if do_plots:
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

if do_plots:
    print('working on threshold plots')

stats = pd.DataFrame(columns=['channel', 'slope', 'offset'])
# Want colors to match lines and dots, so need a defined sequence. But, matplotlib's longest
# is only 20, so I'm just concatenating 4 of them into a list of 80, so I don't go index out
# of range.

if do_plots:
    colors = mpl.color_sequences['tab20'] + mpl.color_sequences['tab20'] + mpl.color_sequences['tab20'] + mpl.color_sequences['tab20']
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize = (10, 5))

for chan in range(36):
    chan_data = array[array[:, 2] == chan] # picks out the third column
    if do_plots:
        ax1.plot(chan_data[:, 0], chan_data[:, 1], linestyle = 'none', marker = 'o', label = 'CH_'+str(chan), color = colors[chan])
    calib = (chan_data[:,1])
    trim_toa = (chan_data[:,0])
    sie = ss(calib, trim_toa, 'hierarchical')
    stats.loc[len(stats)] = [chan, sie[0], sie[1]]
    if do_plots:
        ax1.plot(trim_toa, trim_toa * sie[0] + sie[1], color = colors[chan], linestyle = '--')
for chan in range(36, 72):
    chan_data = array[array[:, 2] == chan] # picks out the third column
    if do_plots:
        ax2.plot(chan_data[:, 0], chan_data[:, 1], linestyle = 'none', marker = 'o', label = 'CH_'+str(chan), color = colors[chan])
    calib = (chan_data[:,1])
    trim_toa = (chan_data[:,0])
    sie = ss(calib, trim_toa, 'hierarchical')
    stats.loc[len(stats)] = [chan, sie[0], sie[1]]
    if do_plots:
        ax2.plot(trim_toa, trim_toa * sie[0] + sie[1], color = colors[chan], linestyle = '--')
if do_plots:
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

print(f'picking calib = {target_calib} as desired value')
target_trim = []
# setting values outside the bounds of 0-63 as 0 or 63.
for chan in range(72):
    if int((([target_calib] - stats[stats['channel'] == chan]['offset']) / stats[stats['channel'] == chan]['slope']).iloc[0]) < 0:
        target_trim.append(0)
    elif int((([target_calib] - stats[stats['channel'] == chan]['offset']) / stats[stats['channel'] == chan]['slope']).iloc[0]) > 63:
        target_trim.append(63)
    else:
        target_trim.append(int((([target_calib] - stats[stats['channel'] == chan]['offset']) / stats[stats['channel'] == chan]['slope']).iloc[0]))
print('target trims are ', target_trim)

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