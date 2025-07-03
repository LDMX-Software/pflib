""" 
plot the pulses gathered from a parameter-time-scan run
heavily influenced by time_scan.py. Thanks Tom!

Every individual plotting function in this script calls on plt_gen, which defines 
general plotting parameters.

"""

import update_path
from plt_gen import plt_gen
from get_params import get_params

from pathlib import Path
import argparse

import matplotlib.pyplot as plt
import numpy as np

from read import read_pflib_csv

from functools import partial

parser = argparse.ArgumentParser()
parser.add_argument('time_scan', type=Path, help='time scan data, only one event per time point')
parser.add_argument('-ex', '--extra_csv_files', type=Path, nargs='+', help='time scan data, if you want to plot data from multiple csv files. Can be used to compare different parameter settings on the same plot. Adds a legend that takes the parameter settings.')
parser.add_argument('-o','--output', type=Path, help='file to which to print, default is input file with extension changed to ".png"')
plot_types = ['SCATTER', 'HEATMAP']
plot_funcs = ['ADC-TIME', 'TOT-TIME', 'TOT', 'TOT-EFF', 'PARAMS']
parser.add_argument('-pt','--plot_type', choices=plot_types, default=plot_types[0], type=str, help=f'Plotting type. Options: {", ".join(plot_types)}')
parser.add_argument('-pf','--plot_function', choices=plot_funcs, default=plot_types[0], type=str, help=f'Plotting function. Options: {", ".join(plot_types)}')
parser.add_argument('-xl','--xlabel', default='time [ns]', type=str, help=f'What to label the x-axis with.')
parser.add_argument('-yl', '--ylabel', default='ADC', type=str, help=f'What to label the y-axis with.')
args = parser.parse_args()

if args.output is None:
    args.output = args.time_scan.with_suffix(".png")

multicsv = bool(args.extra_csv_files)

samples, run_params = read_pflib_csv(args.time_scan)
if multicsv:
    samples_collection = [samples]
    run_params_collection = [run_params]
    for csv in args.extra_csv_files:
        samples, run_params = read_pflib_csv(csv)
        samples_collection.append(samples)
        run_params_collection.append(run_params)

"""
    Set xticks
"""
def set_xticks (
    xvar,
    ax
):
    xmin, xmax = xvar.min(), xvar.max()
    xmin = 25*np.floor(xmin/25)
    xmax = 25*np.ceil(xmax/25)
    ax.set_xticks(ticks = np.arange(xmin, xmax+1, 25), minor=False)
    ax.set_xticks(ticks = np.arange(xmin, xmax, 25/16), minor=True) 

#################### PLOTTING FUNCTIONS ########################

"""
    Plot a selected parameter vs time.
"""

def time(
    samples,
    run_params,
    ax,
    xticks = False,
    yval = 'adc'
):
    if xticks:
        set_xticks(samples['time'], ax)
    groups, param_name = get_params(samples)
    cmap = plt.get_cmap('viridis')
    n = len(groups)
    for i, (group_id, group_df) in enumerate(groups):
        val = group_df[param_name].iloc[0]
        color = cmap(i/n)
        key = param_name.split('.')[1]
        plt.scatter(group_df['time'], (group_df[yval]), label=f'{key} = {val}', 
                    s=5, color=color)
        if (len(groups) < 10):
            plt.legend()

"""
    Plot the tot vs calib
"""

def tot(
    samples,
    run_params,
    ax,
    multiple_pulses = False,
    tot_vref = 500,
    toa_vref = 250
):
    groups, param_name = get_params(samples)
    filtered = samples[samples['tot'] > 0] # if tot <= 0 it didn't trigger
    plt.scatter(filtered[param_name], filtered['tot'], s=5)
    plt.title(' '.join([f'{key} = {val}' for key, val in run_params.items()]) 
              + f', TOA_VREF = {toa_vref}, TOT_VREF = {tot_vref}')

"""
    Plot the tot efficiency vs given parameter. 
    For x samples per timepoint, the tot efficiency is the number of
    events with triggered tot, divided by x.
"""

def tot_eff(
    samples,
    run_params,
    ax,
    multiple_pulses = False,
):
    groups, param_name = get_params(samples)
    # This method to get the number of samples per timepoint is horrible
    # requires assistance of a pandas wizard
    key = list(groups.groups.keys())[0]
    group = groups.get_group(key)
    nr_bx = len(group.groupby('time'))
    samples['new_cycle'] = (samples['time'].shift(1) > 0) & (samples['time'] == 0)
    samples['cycle'] = samples['new_cycle'].cumsum()
    # this is the number of samples per timepoint
    nr_tot = int(sum(samples['cycle'] < 1) / nr_bx)
    x = []
    y = []
    for i, (group_id, group_df) in enumerate(groups):
        time_groups = group_df.groupby('time')
        tot_eff = 0
        for time_id, time_df in time_groups:
            for tot in time_df['tot']:
                if tot != -1:
                    tot_eff += 1
        tot_eff /= nr_tot
        val = group_df[param_name].iloc[0]
        key = param_name.split('.')[1]
        x.append(group_df[param_name].iloc[0])
        y.append(tot_eff)
    plt.plot(x, y, 
             marker='o', color='black', linestyle='--', 
             markerfacecolor='red', markeredgecolor = 'red', markersize = 5)
    plt.title(' '.join([f'{key} = {val}' for key, val in run_params.items()]) 
              + ', TOA_VREF = 250, TOT_VREF = 500')

"""
    Plot the ADC vs a selected parameter on a scatter plot. 
    If multicsv, then the legend is set to the params for the given csv file.
"""

def param(
    samples,
    run_params,
    ax
):
    groups, param_name = get_params(samples)
    x = []
    y = []
    for _, group_df in groups:
        y.append(group_df['adc'].max())
        val = group_df[param_name].iloc[0]
        x.append(val)
    if multicsv:
        ax.scatter(x,y,label=(' '.join([f'{key} = {val}' for key, val in run_params.items()])))
        ax.legend()
    else:
        ax.scatter(x,y)

"""
    Two options for heatmaps, when having multiple events per timepoint.
    The first is creating a heatmap for each individual CALIB voltage, 
    the second is a stacked plot to visualize and compare the pulse shapes.

    Takes input csv from tasks.parameter_timescan
"""

def heatmaps(
    samples,
    run_params,
    ax
):
    groups, param_name = get_params(samples)

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
        val = group_df[param_name].iloc[0]
        H_norm = norm_heatmap(group_df['time'], group_df['adc'], x_edges, y_edges)

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


    #stacked heatmap plot
    H_combined = np.zeros((len(x_edges) - 1, len(y_edges) - 1))

    for group_id, group_df in groups:
        val = group_df[param_name].iloc[0] 
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

    calib_vals = [group_df[param_name].iloc[0] for _, group_df in groups]
    calib_str = ", ".join(str(v) for v in calib_vals)

    plt.colorbar(label='Normalized Counts')
    plt.xlabel('time / ns = (charge_to_l1a - 20 + 1)*25 - samples.phase_strobe*25/16')
    plt.ylabel('ADC')
    plt.title(f'Combined Normalized Heatmap\nCALIB values: {calib_str}')
    plt.tight_layout()
    plt.savefig(f'{args.output}_CombinedHeatmap.png', dpi=200)
    plt.close()

"""
    If we want to plot multiple csv files!

    Takes collections (lists) instead of single sample, run_params, and loops over them
    for a given plot function. 

    So far only tested for PARAMS.
"""

def multiparams(
    samples_collection,
    run_params_collection,
    ax
):
    for i in range(len(samples_collection)):
        if args.plot_function == 'TIME' and args.plot_type == 'SCATTER':
            time(samples_collection[i], run_params_collection[i], ax)
        elif args.plot_function == 'TIME' and args.plot_type == 'HEATMAP':
            heatmap(samples_collection[i], run_params_collection[i], ax)
        elif args.plot_function == 'PARAMS':
            param(samples_collection[i], run_params_collection[i], ax)

############################## MAIN ###################################

if args.plot_function == 'ADC-TIME' and args.plot_type == 'SCATTER':
    plt_gen(partial(time, yval = 'adc'), samples, run_params, args.output,
            xlabel = args.xlabel, ylabel = args.ylabel)

if args.plot_function == 'TOT-TIME':
    plt_gen(partial(time, yval = 'tot'), samples, run_params, args.output, 
            ylim = False, ylim_range = [200, 500], 
            xlabel = args.xlabel, ylabel = args.ylabel)

if args.plot_function == 'TOT':
    plt_gen(partial(tot, toa_vref=250, tot_vref=600), 
            samples, run_params, args.output, 
            xlabel = args.xlabel, ylabel = args.ylabel)

if args.plot_function == 'TOT-EFF':
    plt_gen(tot_eff, samples, run_params, args.output, 
            xlabel = args.xlabel, ylabel = args.ylabel)

if args.plot_function == 'PARAMS':
    plt_gen(param, samples, run_params, args.output,
            xlabel = args.xlabel, ylabel = args.ylabel)

if args.plot_function == 'TIME' and args.plot_type == 'HEATMAP':
    plt_gen(heatmap, samples, run_params, args.output,
            xlabel = args.xlabel, ylabel = args.ylabel)

if multicsv:
    plt_gen(multiparams, samples_collection, run_params_collection, args.output, 
            multicsv = True,
            xlabel = args.xlabel, ylabel = args.ylabel)
