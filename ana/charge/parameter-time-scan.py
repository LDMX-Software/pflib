""" 
plot the pulses gathered from a parameter-time-scan run
heavily influenced by time_scan.py. Thanks Tom!
"""

import update_path

from pathlib import Path
import argparse

import matplotlib.pyplot as plt
import numpy as np

from read import read_pflib_csv

parser = argparse.ArgumentParser()
parser.add_argument('time_scan', type=Path, help='time scan data, only one event per time point')
parser.add_argument('-o','--output', type=Path, help='file to which to print, default is input file with extension changed to ".png"')
plot_types = ['SCATTER', 'HEATMAP']
parser.add_argument('-pt','--plot_type', choices=plot_types, default=plot_types[0], type=str, help=f'which type to plot with. Options: {", ".join(plot_types)}')

args = parser.parse_args()

if args.output is None:
    args.output = args.time_scan.with_suffix(".png")

samples, run_params = read_pflib_csv(args.time_scan)

parameter_names = []

for column in samples:
    if column.find('.') != -1:
        parameter_names.append(column)

groups = samples.groupby(parameter_names[0])

print(type(groups))

plt.figure()
#plt.xlabel('time [ns]')
#plt.ylabel('ADC')
plt.title(' '.join([f'{key} = {val}' for key, val in run_params.items()]))
plt.grid()

cmap = plt.get_cmap('viridis')
n = len(groups)

for i, (group_id, group_df) in enumerate(groups):
    val = group_df[parameter_names[0]].iloc[0]
    color = cmap(i/n)
    #plt.scatter(group_df['time'], group_df['adc'], label=f'CALIB = {val}', s=5, color=color)
    if (n < 10):
        plt.legend()

#plt.savefig(args.output, bbox_inches='tight')
#plt.clf()

# Seperate function here for plotting max ADC vs calib

x = []
y = []

for _, group_df in groups:
    y.append(group_df['adc'].max())
    val = group_df[parameter_names[0]].iloc[0]
    x.append(val)


plt.xlabel('calib')
plt.ylabel('ADC')
plt.scatter(x,y)
plt.savefig(args.output, bbox_inches='tight')
plt.clf()

