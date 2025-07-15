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
parser.add_argument('vt50_scan', type=Path, help='vt50 scan data')
parser.add_argument('-o','--output', type=Path, help='file to which to print, default is input file with extension changed to ".png"')
args = parser.parse_args()

samples, run_params = read_pflib_csv(args.vt50_scan)

def vt50_calib(
    samples,
    run_params,
    ax,
):
    """Plot the calib value of the vt50 vs tot_vref."""
    groups, param_name = get_params(samples, 0)
    nr_tot = 20 # Number of samles per timepoint that was used
    cmap = plt.get_cmap('viridis')
    n = len(groups)
    #plt.xlim(0,1000)
    for i, (vref_id, vref_df) in enumerate(groups): #iterates through vref
        x = []
        y = []
        color = cmap(i/n)
        second_group, second_param_name = get_params(vref_df, 1)  
        for k, (calib_id, calib_df) in enumerate(second_group): #iterates through calib
            time_groups = calib_df.groupby('time')
            tot_eff = 0
            # nr of samples per timepoint. Can be larger than the number of 
            # timepoints given if the scan checks the same position multiple times
            nr_tot = 0
            for time_id, time_df in time_groups:
                nr_tot += len(time_df)
                for tot in time_df['tot']:
                    if tot > 0:
                        tot_eff += 1
            tot_eff /= nr_tot # normalize
            calib = calib_df[second_param_name].iloc[0]
            key = param_name.split('.')[1]
            x.append(calib_df[second_param_name].iloc[0])
            y.append(tot_eff)
        ax.plot(x, y, 
                marker='o', color='black', linestyle='--', 
                markerfacecolor=color, markeredgecolor=color, markersize=5,
                label=f"tot_vref = {vref_df[param_name].iloc[0]}")
        ax.legend(ncols=2, fontsize='xx-small', loc='lower right')
        #if sum(y) == 0 or sum(y) == 0.0: # vrefs that don't have a tot_eff
        #    continue
    plt.title(' '.join([f'{key} = {val}' for key, val in run_params.items()]) 
            + ', TOA_VREF = 250, Binary search')

plt_gen(vt50_calib, samples, run_params, args.output,
            xlabel = "calib", ylabel = "tot efficiency")
