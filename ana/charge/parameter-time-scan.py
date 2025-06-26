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
plot_funcs = ['TIME', 'PARAMS']
parser.add_argument('-pt','--plot_type', choices=plot_types, default=plot_types[0], type=str, help=f'which type to plot with. Options: {", ".join(plot_types)}')
parser.add_argument('-pf','--plot_function', choices=plot_funcs, default=plot_types[0], type=str, help=f'which function to plot with. Options: {", ".join(plot_types)}')

args = parser.parse_args()

if args.output is None:
    args.output = args.time_scan.with_suffix(".png")

samples, run_params = read_pflib_csv(args.time_scan)

#################### FUNCS ########################

def set_xtix (
    samples,
    ax
):
    xmin, xmax = df[time].min(), df[time].max()
    xmin = 25*np.floor(xmin/25)
    xmax = 25*np.ceil(xmax/25)
    ax.set_xticks(ticks = np.arange(xmin, xmax+1, 25), minor=False)
    ax.set_xticks(ticks = np.arange(xmin, xmax, 25/16), minor=True) 

def plt_gen(
    plotting_func,
    samples,
    set_xticks = True,
    ax = None,
    xlabel = 'time [ns]',
    ylabel = 'ADC',
    **kwargs
):
    plt.figure()
    plt.xlabel(xlabel)
    plt.ylabel(ylabel)
    plt.title(' '.join([f'{key} = {val}' for key, val in run_params.items()]))
    plt.grid()
    #Run the specified plot
    ax = plt.gca()
    plotting_func(samples, ax)
    plt.savefig(args.output, bbox_inches='tight')
    plt.clf()

def get_params(
    samples,
):
    parameter_names = []
    for column in samples:
        if column.find('.') != -1:
            parameter_names.append(column)
    groups = samples.groupby(parameter_names[0])
    return groups, parameter_names[0]


"""
    Plot the requested parameter vs time.

    Takes input csv from tasks.parameter_timescan
"""

def plt_time(
    samples,
    ax
):
    groups, param_name = get_params(samples)
    cmap = plt.get_cmap('viridis')
    n = len(groups)
    for i, (group_id, group_df) in enumerate(groups):
        val = group_df[param_name].iloc[0]
        color = cmap(i/n)
        plt.scatter(group_df['time'], group_df['adc'], label=f'CALIB = {val}', s=5, color=color)
        plt.legend()

if args.plot_function == 'TIME' and args.plot_type == 'SCATTER':
    plt_gen(plt_time, samples)

"""
    Plot the ADC vs a requested parameter

    Takes input csv from tasks.parameter_timescan
"""

def plt_param(
    samples,
    ax
):
    groups, param_name = get_params(samples)
    x = []
    y = []
    for _, group_df in groups:
        y.append(group_df['adc'].max())
        val = group_df[param_name].iloc[0]
        x.append(val)
    ax.scatter(x,y)

if args.plot_function == 'PARAMS':
    plt_gen(plt_param, samples)

"""
    Two options for heatmaps, when having multiple events per timepoint.
    The first is creating a heatmap for each individual CALIB voltage, 
    the second is a stacked plot to visualize and compare the pulse shapes.

    Takes input csv from tasks.parameter_timescan
"""

if args.plot_type == 'HEATMAP':
    x_min = min(group_df['time'].min() for _, group_df in groups)
    x_max = max(group_df['time'].max() for _, group_df in groups)
    y_min = min(group_df['adc'].min() for _, group_df in groups) - 50
    y_max = max(group_df['adc'].max() for _, group_df in groups) + 50

    x_bins = samples['time'].nunique() #amount of timepoints
    y_bins = 128 #1024/8 = 128 for 1024 ADC bins

    x_edges = np.linspace(x_min, x_max, x_bins + 1)
    y_edges = np.linspace(y_min, y_max, y_bins + 1)

    def norm_heatmap(x, y, x_edges, y_edges):
        H,_,_ = np.histogram2d(x,y, bins=[x_edges, y_edges])

        #normalize each column (i.e. ADC axis for each time slice)
        H_norm = np.zeros_like(H)
        col_max = H.max(axis=0, keepdims=True) #max count per timeslice
        nonzero_mask = col_max != 0 #to handle division by zero
        H_norm[:, nonzero_mask[0]] = H[:, nonzero_mask[0]] / col_max[:, nonzero_mask[0]]

        return H_norm

    #individual heatmap for each CALIB
    for group_id, group_df in groups:
        val = group_df[parameter_names[0]].iloc[0]
        H_norm = norm_heatmap(group_df['time'], group_df['adc'], x_edges, y_edges)

        plt.figure()
        plt.imshow(
            H_norm.T,  #transpose to have time on x-axis, ADC on y-axis
            extent=[x_min, x_max, y_min, y_max],
            aspect='auto',
            cmap='Blues',
            origin='lower',
            vmin=0,
            vmax=1
        )
        plt.xlabel('time / ns = (charge_to_l1a - 20 + 1)*25 - samples.phase_strobe*25/16')
        plt.ylabel('ADC')
        plt.colorbar(label='Normalized Counts')
        plt.title(f'Normalized Heatmap for CALIB = {val}')
        plt.savefig(f'{args.output}_NormalizedHeatmap_calib_{val}.png', dpi=200)
        plt.close()


    #stacked heatmap plot
    H_combined = np.zeros((len(x_edges) - 1, len(y_edges) - 1))

    for group_id, group_df in groups:
        val = group_df[parameter_names[0]].iloc[0] 
        H_norm = norm_heatmap(group_df['time'], group_df['adc'], x_edges, y_edges)
        H_combined += H_norm

    plt.figure()
    plt.imshow(
        H_combined.T,
        extent=[x_min, x_max, y_min, y_max],
        origin='lower',
        aspect='auto',
        cmap='Blues',
        vmin=0,
        vmax=1
    )

    calib_vals = [group_df[parameter_names[0]].iloc[0] for _, group_df in groups]
    calib_str = ", ".join(str(v) for v in calib_vals)

    plt.colorbar(label='Normalized Counts')
    plt.xlabel('time / ns = (charge_to_l1a - 20 + 1)*25 - samples.phase_strobe*25/16')
    plt.ylabel('ADC')
    plt.title(f'Combined Normalized Heatmap\nCALIB values: {calib_str}')
    plt.tight_layout()
    plt.savefig(f'{args.output}_CombinedHeatmap.png', dpi=200)
    plt.close()

if args.plot_function == 'TIME' and args.plot_type == 'HEATMAP':
    plt_gen(plt_heatmap, samples)
