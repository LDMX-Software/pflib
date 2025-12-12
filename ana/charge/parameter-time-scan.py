""" 
plot the pulses gathered from a parameter-time-scan run
heavily influenced by time_scan.py. Thanks Tom!

Every individual plotting function in this script calls on plt_gen, which defines 
general plotting parameters.

"""

import update_path
from plt_gen import plt_gen
from get_params import get_params
from astropy.stats import sigma_clipped_stats

import os

from pathlib import Path
import argparse

import matplotlib.pyplot as plt
import matplotlib.ticker as ticker

from scipy.optimize import curve_fit
import numpy as np
import pandas as pd

from read import read_pflib_csv

from functools import partial

parser = argparse.ArgumentParser()
parser.add_argument('time_scan', type=Path, help='time scan data, only one event per time point')
parser.add_argument('-ex', '--extra_csv_files', type=Path, nargs='+', help='time scan data, if you want to plot data from multiple csv files. Can be used to compare different parameter settings on the same plot. Adds a legend that takes the parameter settings.')
parser.add_argument('-o','--output', type=Path, help='file to which to print, default is input file with extension changed to ".png"')
parser.add_argument('-od', '--output_directory', type=Path, help='directory to which to print.')
parser.add_argument('-r', '--repository', type=bool, help='True if you would like to indicate that the input is a repository with csv files rather than a single csv file.')
plot_types = ['SCATTER', 'HEATMAP']
plot_funcs = ['ADC-TIME', 'ADC-ALL-CHANNELS', 'TOT-TIME', 'TOT', 'TOT-EFF', 'PARAMS', 'MULTI-CHANNEL']
parser.add_argument('-pt','--plot_type', choices=plot_types, default=plot_types[0], type=str, help=f'Plotting type. Options: {", ".join(plot_types)}')
parser.add_argument('-pf','--plot_function', choices=plot_funcs, default=plot_types[0], type=str, help=f'Plotting function. Options: {", ".join(plot_types)}')
parser.add_argument('-xl','--xlabel', default='time [ns]', type=str, help=f'What to label the x-axis with.')
parser.add_argument('-yl', '--ylabel', default='ADC', type=str, help=f'What to label the y-axis with.') 
args = parser.parse_args()

if args.output is None:
    args.output = args.time_scan.with_suffix(".png")

multicsv = bool(args.extra_csv_files)

if args.repository:
    samples_collection = []
    run_params_collection = []
    for name in os.listdir(args.time_scan):
        if not name.endswith(".csv"):
            continue # skip non csv files or directories
        dir_str = str(args.time_scan) + '/' + name
        samples, run_params = read_pflib_csv(dir_str)
        samples_collection.append(samples)
        run_params_collection.append(run_params)
else:
    samples, run_params = read_pflib_csv(args.time_scan)
    if multicsv:
        samples_collection = [samples]
        run_params_collection = [run_params]
        for csv in args.extra_csv_files:
            samples, run_params = read_pflib_csv(csv)
            samples_collection.append(samples)
            run_params_collection.append(run_params)

def set_xticks (
    xvar,
    ax
):
    """Set xticks"""
    xmin, xmax = xvar.min(), xvar.max()
    xmin = 25*np.floor(xmin/25)
    xmax = 25*np.ceil(xmax/25)
    ax.set_xticks(ticks = np.arange(xmin, xmax+1, 25), minor=False)
    ax.set_xticks(ticks = np.arange(xmin, xmax, 25/16), minor=True)

#################### HELPER FUNCTIONS ########################

def linear(x, k, m):
    return k*x + m

#################### PLOTTING FUNCTIONS ########################

def adc_all_channels(
        samples, 
        run_params, 
        ax_holder,
        plot_params="all",   # "all" or "one"
        param_index=6        # used only when plot_params="one"
):
    """
    Plot max ADC vs channels, compute RMS trends, and fit dependence on parameters.
    """

    def compute_rms(ch_df, pedestal):
        """RMS for a single channel dataframe."""
        return np.sqrt(((ch_df['adc'] - pedestal)**2).mean())

    def compute_all_rms(df, pedestal):
        """Return {channel: rms}"""
        rms = df.groupby('channel').apply(lambda c: compute_rms(c, pedestal))
        return rms.reset_index(name='rms')

    def conditional_legend(ax, max_labels=37, **kwargs):
        """Only draw legend if below threshold."""
        handles, labels = ax.get_legend_handles_labels()
        if len(labels) < max_labels:
            ax.legend(**kwargs)

    def save_and_close(fig, fname):
        fig.savefig(fname, dpi=400)
        plt.close(fig)

    charges_group = samples.groupby("nr channels")
    fig_rms, ax_rms = plt.subplots(1, 1)
    ax_rms.set_xlabel('Channels activated')
    ax_rms.set_ylabel('Average RMS of non-activated channels')

    link = 0
    central_val = 54 if link == 1 else 18

    activated_channels_list = []
    avg_rms_list = []

    runs = len(charges_group)
    activated_channels_list = [[] for _ in range(runs - 1)]
    avg_rms_list = [[] for _ in range(runs - 1)]

    parameter_values = set()

    # Loop over charge groups
    for i, (charge_id, charge_df) in enumerate(charges_group):
        print(f'Running {i+1} out of {runs}')
        # Pedestal case
        if i == 0:
            fig, ax = plt.subplots()
            df = charge_df[charge_df['nr channels'] == 0]

            df = df[df['channel'] < 36] if link == 0 else df[df['channel'] >= 36]

            mean, med, std = sigma_clipped_stats(df['adc'], sigma=3)
            pedestal = mean

            ax.scatter(df['channel'], df['adc'], s=5, color='r', lw=1)
            ax.set_ylabel('ADC')
            ax.set_xlabel('Channel')

            save_and_close(fig, 'Pedestal.png')
            continue

        fig, ax1 = plt.subplots()
        fig_time, ax_time = plt.subplots()

        ax1.set_ylabel(r'$\Delta$ADC')
        ax1.set_xlabel('Channel')
        ax1.xaxis.set_minor_locator(ticker.AutoMinorLocator())

        ax_time.set_ylabel('ADC')
        ax_time.set_xlabel('Time')

        # Parameter grouping
        try:
            param_group, param_name = get_params(charge_df, 0)
        except Exception:
            param_group = [(None, charge_df)]

        cmap = plt.get_cmap('viridis')

        for j, (param_id, param_df) in enumerate(param_group):

            # --- Parameter selection ---
            if plot_params == "one":
                if j != param_index:
                    continue

            try:
                key = param_name.split('.')[1]
                val = param_df[param_name].iloc[0]
            except Exception:  
                key = "Channels flashed, voltage [mVpp]"
                val = (4, 2000)

            parameter_values.add(val)

            max_diff = (
                param_df
                .groupby('channel')['adc']
                .apply(lambda s: (s - pedestal).abs().max())
                .reset_index(name='max_diff')
            )
            
            # Get RMS
            rms_df = compute_all_rms(param_df, pedestal)
            rms_vals = rms_df['rms'].tolist()
            avg_rms = 0
            count = 0
            for k in range(central_val - 18, central_val + 18):
                if central_val - (i - 1) <= k <= central_val + (i - 1):
                    continue
                avg_rms += rms_vals[k]
                count += 1

            if count > 0:
                avg_rms /= count
                avg_rms_list[i - 1].append(avg_rms)
                activated_channels_list[i - 1].append(i)

            merged = max_diff.merge(rms_df, on='channel')
            ax1.set_ylim(-10, merged['max_diff'].max() + 10)
            ax1.scatter(
                merged['channel'], merged['max_diff'],
                label=f'{key} = {val}',
                s=5, color=cmap(j / len(param_group)), lw=1
            )
            if link == 0:
                link_df = param_df[param_df['channel'] < 36]
            elif link == 1:
                link_df = param_df[param_df['channel'] >= 36]
            for k, (ch_id, ch_df) in enumerate(link_df.groupby('channel')):
                ax_time.scatter(
                    ch_df['time'], ch_df['adc'],
                    label=f'ch{ch_id}',
                    s=5, color=plt.get_cmap('tab20')(k / 20)
                    )

        # Pedestal and link lines
        ax1.axhline(y=0, color='k', linestyle='--', linewidth=0.8)
        ax1.axvline(x=35.5, color='r', linestyle='--', alpha=0.5)

        conditional_legend(ax1, fontsize=8)
        conditional_legend(ax_time, fontsize=4, ncols=3)

        save_and_close(fig, f'adc_channels_{i}.png')
        save_and_close(fig_time, f'adc_time_{i}.png')

    # slope/intercept plots
    activated_T = list(map(list, zip(*activated_channels_list)))
    rms_T = list(map(list, zip(*avg_rms_list)))
    parameter_values = sorted(parameter_values)

    if len(activated_T) > 1:
        slopes = []
        intercepts = []

        for xvals, yvals in zip(activated_T, rms_T):
            popt, _ = curve_fit(linear, xvals, yvals)
            slopes.append(popt[0])
            intercepts.append(popt[1])

            ax_rms.scatter(xvals, yvals, s=8)
            xfit = np.linspace(min(xvals), max(xvals), 100)
            ax_rms.plot(xfit, linear(xfit, *popt),
                        label=f'Fit: y={popt[0]:.3f}x + {popt[1]:.3f}, CALIB={parameter_values[len(slopes)-1]}')

        conditional_legend(ax_rms, fontsize=8)
        save_and_close(fig_rms, 'avg_rms.png')

        fig_par, ax_par = plt.subplots()
        ax_par.scatter(parameter_values, slopes, label='slopes')
        ax_par.scatter(parameter_values, intercepts, label='intercepts')
        ax_par.legend()
        ax_par.set_xlabel('CALIB')
        ax_par.set_title('Slope and intercepts from fits')
        save_and_close(fig_par, 'slope_intercept.png')

def time(
    samples,
    run_params,
    ax,
    xticks = False,
    yval = 'adc'
):
    """Plot a selected parameter vs time."""
    if xticks:
        set_xticks(samples['time'], ax)
    groups, param_name = get_params(samples, 0)
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

def time_multichannel(
    samples_collection,
    run_params_collection
):
    """
    Plot the ADC vs a selected parameter on a scatter plot.
    Do this for each channel. We run multiple csv files that have the format
    param-timescan-channel-i-option.csv' where i is the channel number
    and option is highrange, lowrange or preCC.
    """
    for samples, run_params in zip(samples_collection, run_params_collection):
        highrange = run_params['highrange']
        preCC = run_params['preCC']
        param_group, param_name = get_params(samples,0)
        cmap = plt.get_cmap('viridis')
        n = len(param_group)
        ch = samples['channel'][0]
        plt.figure()
        plt.xlabel('Time [ns]')
        plt.ylabel('ADC')
        plt.title(' '.join([f'{key} = {val} ' for key,val in run_params.items()]) + ' ' + 'channel ' + str(ch))
        for i, (group_id, group_df) in enumerate(param_group):
            val = group_df[param_name].iloc[0]
            color = cmap(i/n)
            key = param_name.split('.')[1]
            plt.scatter(group_df['time'], group_df['adc'], label=f'{key} = {val}', 
                        s=5, color=color)
        plt.legend()
        if highrange:
            plt.ylim(0,1100)
            fig_name = str(args.output_directory) + '/parameter_timescan_channel' + str(ch) + '_highrange.png'
        elif preCC:
            plt.ylim(180,1000)
            fig_name = str(args.output_directory) + '/parameter_timescan_channel' + str(ch) + '_preCC.png'
        else:
            plt.ylim(180,800)
            fig_name = str(args.output_directory) + '/parameter_timescan_channel' + str(ch) + '_lowrange.png'
        plt.savefig(fig_name, dpi=400)
        plt.close()
        if highrange:
            print("Channel " + str(ch) + " highrange plotted and saved!")
        elif preCC:
            print("Channel " + str(ch) + " preCC plotted and saved!")
        else:
            print("Channel " + str(ch) + " lowrange plotted and saved!")

def tot(
    samples,
    run_params,
    ax,
    multiple_pulses = False,
    tot_vref = 500,
    toa_vref = 250
):
    """Plot the tot vs calib"""
    groups, param_name = get_params(samples, 0)
    filtered = samples[samples['tot'] > 0] # if tot <= 0 it didn't trigger
    plt.scatter(filtered[param_name], filtered['tot'], s=5)
    plt.title(' '.join([f'{key} = {val}' for key, val in run_params.items()]) 
              + f', TOA_VREF = {toa_vref}, TOT_VREF = {tot_vref}')

def tot_eff(
    samples,
    run_params,
    ax,
    multiple_params = False
):
    """
    Plot the tot efficiency vs given parameter. 
    For x samples per timepoint, the tot efficiency is the number of
    events with triggered tot, divided by x.
    """
    groups, param_name = get_params(samples, 0)
    if not multiple_params:
        # This method to get the number of samples per timepoint is horrible
        # requires assistance of a pandas wizard
        key = list(groups.groups.keys())[0]
        group = groups.get_group(key)
        nr_bx = len(group.groupby('time'))
        samples['new_cycle'] = (samples['time'].shift(1) > 0) & (samples['time'] == 0)
        samples['cycle'] = samples['new_cycle'].cumsum()
        # Number of samples per timepoint
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
    else:
        nr_tot = 20 # Number of samles per timepoint
        cmap = plt.get_cmap('viridis')
        n = len(groups)
        for i, (vref_id, vref_df) in enumerate(groups): #iterates through vref
            color = cmap(i/n)
            second_group, second_param_name = get_params(vref_df, 1)  
            x = []
            y = []
            for k, (calib_id, calib_df) in enumerate(second_group): #iterates through calib
                time_groups = calib_df.groupby('time')
                tot_eff = 0
                for time_id, time_df in time_groups:
                    for tot in time_df['tot']:
                        if tot > 0:
                            tot_eff += 1
                if (tot_eff > 20):
                    tot_eff /= 40
                else:
                    tot_eff /= nr_tot
                calib = calib_df[second_param_name].iloc[0]
                key = param_name.split('.')[1]
                x.append(calib_df[second_param_name].iloc[0])
                y.append(tot_eff)
            #if sum(y) < 0.1: #if we don't want to plot the vrefs that don't have a tot_eff
            #    continue
            vref = vref_df[param_name].iloc[0]
            ax.plot(x, y, 
                     marker='o', color='black', linestyle='--', 
                     markerfacecolor=color, markeredgecolor = color, markersize = 5,
                     label=f"TOT_VREF={vref}")
            #ax.legend(ncols=5, fontsize='xx-small', loc='upper right')
            plt.title(' '.join([f'{key} = {val}' for key, val in run_params.items()]) 
                      + ', TOA_VREF = 250, TOT_VREF = 500')


def param(
    samples,
    run_params,
    ax
):
    """
    Plot the ADC vs a selected parameter on a scatter plot. 
    If multicsv, then the legend is set to the params for the given csv file.
    """
    groups, param_name = get_params(samples, 0)
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

def heatmaps(
    samples,
    run_params,
    ax
):
    """
    Two options for heatmaps, when having multiple events per timepoint.
    The first is creating a heatmap for each individual CALIB voltage, 
    the second is a stacked plot to visualize and compare the pulse shapes.

    Takes input csv from tasks.parameter_timescan
    """
    groups, param_name = get_params(samples, 0)

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

def multiparams(
    samples_collection,
    run_params_collection,
    ax
):
    """
    If we want to plot multiple csv files!

    Takes collections (lists) instead of single sample, run_params, and loops over them
    for a given plot function. 

    So far only tested for PARAMS.
    """
    for i in range(len(samples_collection)):
        if args.plot_function == 'TIME' and args.plot_type == 'SCATTER':
            time(samples_collection[i], run_params_collection[i], ax)
        elif args.plot_function == 'TIME' and args.plot_type == 'HEATMAP':
            heatmap(samples_collection[i], run_params_collection[i], ax)
        elif args.plot_function == 'PARAMS':
            param(samples_collection[i], run_params_collection[i], ax)

############################## MAIN ###################################

if args.plot_function == 'ADC-ALL-CHANNELS':
    plt_gen(adc_all_channels, samples, run_params, args.output,
            xlabel = args.xlabel, ylabel = args.ylabel)

if args.plot_function == 'ADC-TIME' and args.plot_type == 'SCATTER':
    plt_gen(partial(time, yval = 'adc'), samples, run_params, args.output,
            xlabel = args.xlabel, ylabel = args.ylabel)

if args.plot_function == 'MULTI-CHANNEL' and args.repository == True:
    time_multichannel(samples_collection, run_params_collection)

if args.plot_function == 'TOT-TIME':
    plt_gen(partial(time, yval = 'tot'), samples, run_params, args.output, 
            ylim = False, ylim_range = [200, 500], 
            xlabel = args.xlabel, ylabel = args.ylabel)

if args.plot_function == 'TOT':
    plt_gen(partial(tot, toa_vref=250, tot_vref=600), 
            samples, run_params, args.output, 
            xlabel = args.xlabel, ylabel = args.ylabel)

if args.plot_function == 'TOT-EFF':
    plt_gen(partial(tot_eff, multiple_params = True), samples, run_params, args.output, 
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
