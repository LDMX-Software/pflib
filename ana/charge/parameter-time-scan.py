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

def plt_pulse(
    samples,
    time = 'time',
    amplitude = 'adc',
    set_xticks = True,
    ax = None,
    **kwargs
):
    df = samples.samples.sort_values(by=time)
    if ax is None:
        ax = plt.gca()
    if args.plot_type == 'SCATTER':
        art = ax.plot(df[time], df[amplitude], **kwargs, s=4,)
    elif args.plot_type == 'HEATMAP':
        art = ax.imshow(df[time], df[amplitude], **kwargs)
    if set_xticks:
        xmin, xmax = df[time].min(), df[time].max()
        xmin = 25*np.floor(xmin/25)
        xmax = 25*np.ceil(xmax/25)
        ax.set_xticks(ticks = np.arange(xmin, xmax+1, 25), minor=False)
        ax.set_xticks(ticks = np.arange(xmin, xmax, 25/16), minor=True)
    return art

samples, run_params = read_pflib_csv(args.time_scan)

parameter_names = []

for column in samples:
    if column.find('.') != -1:
        parameter_names.append(column)

groups = samples.groupby(parameter_names[0])

plt.figure()
plt.xlabel('time [ns]')
plt.ylabel('ADC')
plt.title(' '.join([f'{key} = {val}' for key, val in run_params.items()]))
plt.grid()

cmap = plt.get_cmap('viridis')
n = len(groups)

for i, (group_id, group_df) in enumerate(groups):
    val = group_df[parameter_names[0]].iloc[0]
    color = cmap(i/n)
    plt.scatter(group_df['time'], group_df['adc'], label=f'CALIB = {val}', s=5, color=color)
    plt.legend()

plt.savefig(args.output, bbox_inches='tight')
plt.clf()
