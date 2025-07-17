"""
Analysis script to calculate the toa_efficiency
for different values of TRIM_TOA. Requires a
trim_toa_scan output csv file to run. Also plots
the efficiency versus TRIM_TOA and saves the plot.
"""

import pandas as pd
import numpy as np
import yaml
import matplotlib.pyplot as plt
import os
import argparse
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).parent.parent))

from read import read_pflib_csv

#runtime arguments, read data
parser = argparse.ArgumentParser(
    prog = 'trim_toa_efficiency.py',
    description='takes a csv from tasks.trim_toa_scan and outputs plots of TOA efficiency per channel and trim_toa against calib values'
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

"""
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

# separate the data into unique trim_toa values
unique_trim_toas = data['TRIM_TOA'].unique()

for trim in unique_trim_toas:
    trim_toa_now = data[data['TRIM_TOA'] == trim]
    unique_calibs = trim_toa_now['CALIB_2V5'].unique()
    for chan in range(72):
        toa_efficiency = []
        for calib in unique_calibs:
            calibs = trim_toa_now[trim_toa_now['CALIB_2V5'] == calib]
            # count the number of non-zero TOA values for this channel
            non_zeros = calibs[calibs[str(chan)] != 0]
            chan_triggers = len(non_zeros)
            chan_toa_efficiency = chan_triggers / len(calibs)
            toa_efficiency.append(chan_toa_efficiency)
        plt.plot(unique_calibs, toa_efficiency, label=f'CH {chan}')
    plt.xlabel('CALIB_2V5')
    plt.ylabel('TOA Efficiency')
    plt.suptitle(f'TOA Efficiency vs CALIB_2V5 for TRIM_TOA = {trim}, all CHs', size=16)
    plt.title('link 0 toa_vref = 176, link 0 toa_vref = 172')
    plt.grid()
    # plt.legend()
    plt.show()
    plt.savefig(f'toa_efficiency_TRIM_TOA_{trim}.png')
    # plt.close()  # close the plot to avoid displaying it immediately