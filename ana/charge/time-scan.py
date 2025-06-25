"""plot the pulse gathered from a time-scan run"""

import update_path

from pathlib import Path
import argparse

import matplotlib.pyplot as plt
import numpy as np

from read import read_pflib_csv

parser = argparse.ArgumentParser()
parser.add_argument('time_scan', type=Path, help='time scan data, only one event per time point')
parser.add_argument('-o','--output', type=Path, help='file to which to print, default is input file with extension changed to ".png"')
plot_types = ['SCATTER', 'LINE']
parser.add_argument('-pt','--plot_type', choices=plot_types, default=plot_types[0], type=str,help=f'Plotting type. Options: {", ".join(plot_types)}')
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
    df = samples.sort_values(by=time)
    if ax is None:
        ax = plt.gca()
    if args.plot_type == 'SCATTER':
        art = ax.scatter(df[time], df[amplitude], **kwargs, s=4,)
    elif args.plot_type == 'LINE':
        art = ax.plot(df[time], df[amplitude], **kwargs)
    if set_xticks:
        xmin, xmax = df[time].min(), df[time].max()
        xmin = 25*np.floor(xmin/25)
        xmax = 25*np.ceil(xmax/25)
        ax.set_xticks(ticks = np.arange(xmin, xmax+1, 25), minor=False)
        ax.set_xticks(ticks = np.arange(xmin, xmax, 25/16), minor=True)
    return art

samples, run_params = read_pflib_csv(args.time_scan)
plt_pulse(samples, label='ADC')
plt.xlabel('time / ns = (charge_to_l1a - 20 + 1)*25 - samples.phase_strobe*25/16')
plt.ylabel('ADC')
plt.title(' '.join([f'{key} = {val}' for key, val in run_params.items()]))
plt.grid()
plt.savefig(args.output, bbox_inches='tight')
plt.clf()
