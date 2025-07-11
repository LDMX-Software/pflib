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
plot_funcs = ['ADC-TIME', 'TOT-TIME', 'TOT', 'TOT-EFF', 'PARAMS', 'HR_CORR']
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

#################### PLOTTING FUNCTIONS ########################

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



def highrange_correction(
    samples_list,
    run_params_list,
    ax,
    adcmax_linear = 600 #cut to disregard non-linear data
): 

    """
    This function does two linear fits to the highrange and lowrange ADC vs charge plot. Assuming that the lowrange is "true", the highrange data is corrected so that it aligns with lowrange. 
    This only happens in the linear region for the lowrange and highrange, meaning it excludes everything above an ADC of 600. Then both the lowrange and highrange slopes are plotted against the channel number.
    Requires a two csv input, first lowrange then highrange!
    """

    low_samples, high_samples = samples_list
    low_run_params, high_run_params = run_params_list
    
    channels = sorted(low_samples['channel'].unique())

    m_high_all_ch = []
    m_low_all_ch = []

    for ch in channels:
        low_ch  = low_samples[low_samples['channel'] == ch]
        high_ch = high_samples[high_samples['channel'] == ch]

        def extract_x_y(samples, run_params):
            groups, param_name = get_params(samples, 0)
            cap = 8e-12 if run_params['highrange'] else 500e-15 #for conversion of calib to charge Q[C]

            x, y = [],[]
            for _, group_df in groups:
                q = group_df[param_name].iloc[0]*cap
                adc_max = group_df['adc'].max()

                if adc_max <= adcmax_linear:
                    x.append(q)
                    y.append(adc_max)

            return np.array(x), np.array(y)

        low_x, low_y = extract_x_y(low_ch, low_run_params)
        high_x, high_y = extract_x_y(high_ch, high_run_params)

        m_low, b_low = np.polyfit(low_x, low_y, deg=1) #slope=m, intercept=b
        m_high, b_high = np.polyfit(high_x, high_y, deg=1)
        m_low_all_ch.append(m_low)
        m_high_all_ch.append(m_high)

        high_x_corr = (high_y - b_low)/m_low
        m_high_corr, b_high_corr = np.polyfit(high_x_corr, high_y, deg=1)

        #points for linear fits
        xs_low  = np.linspace(0, max(low_x),  2)
        xs_high = np.linspace(0, max(high_x), 2)

        fig, ax = plt.subplots()
        #lowrange data and fit
        ax.scatter(low_x, low_y, color='lightskyblue', label='lowrange data')
        ax.plot(xs_low, m_low*xs_low + b_low, '--', color='black',  label='lowrange fit')

        #highrange raw data and fit
        ax.scatter(high_x, high_y, color='orange', label='highrange raw')
        ax.plot(xs_high, m_high*xs_high + b_high, '--', color='orange', label='highrange fit')

        #highrange corrected
        ax.scatter(high_x_corr, high_y,  color='red', marker='x', label=f'highrange corrected')

        ax.set_xlabel('Q [C]')
        ax.set_ylabel('ADC')
        ax.set_title(f'Highrange correction for CH{ch}')
        ax.legend()   

        outdir = args.output.parent / "hrcorr"
        outdir.mkdir(exist_ok=True)
        out_file = outdir / f"HR_CORR_CH{ch}.png"
        plt.savefig(out_file)
        plt.close(fig)

    #m_high against channel number plot
    fig, ax = plt.subplots()
    ax.plot(channels, m_high_all_ch)
    ax.set_xlabel('Channel number')
    ax.set_ylabel('m_high')
    ax.set_title('Slope of highrange against channel number')
    scale_factor_outfile = args.output.parent / "m_high.png"
    plt.savefig(scale_factor_outfile)
    plt.close(fig)

    #m_low against channel number plot
    fig, ax = plt.subplots()
    ax.plot(channels, m_low_all_ch)
    ax.set_xlabel('Channel number')
    ax.set_ylabel('m_low')
    ax.set_title('Slope of lowrange against channel number')
    scale_factor_outfile = args.output.parent / "m_low.png"
    plt.savefig(scale_factor_outfile)
    plt.close(fig)


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
        val = group_df[param_name].iloc[0]*8e-12
        x.append(val)
    if multicsv:
        ax.scatter(x,y,label=(' '.join([f'{key} = {val}' for key, val in run_params.items()])))
        ax.legend()
    else:
        ax.scatter(x,y)
        ax.set_xlim(0.3e-8,0.5e-8)

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
    plt_gen(partial(tot_eff, multiple_params = True), samples, run_params, args.output, 
            xlabel = args.xlabel, ylabel = args.ylabel)

if args.plot_function == 'PARAMS':
    plt_gen(param, samples, run_params, args.output,
            xlabel = args.xlabel, ylabel = args.ylabel)

if args.plot_function == 'TIME' and args.plot_type == 'HEATMAP':
    plt_gen(heatmap, samples, run_params, args.output,
            xlabel = args.xlabel, ylabel = args.ylabel)

if args.plot_function == 'HR_CORR':
    plt_gen(highrange_correction, samples_collection, run_params_collection, args.output, multicsv=True)

if multicsv:
    plt_gen(multiparams, samples_collection, run_params_collection, args.output, 
            multicsv = True,
            xlabel = args.xlabel, ylabel = args.ylabel)
