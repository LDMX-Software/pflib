"""
Plots the s-curves of toa and tot efficencies, on all channels.
Meant for data from channel_wise_calib_scan.cxx in tasks
Also plots the linearity of the max_adc

The s-curves of all channels should idealy line up, and we can use this fact to test if the
tot and toa scripts are behaving as they should, and to see if the boards is OK.

"""

import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
from pathlib import Path
import argparse

parser = argparse.ArgumentParser()
parser.add_argument('data', type=Path, help='decoded CSV file to analyze')
parser.add_argument("-pa", "--plot-adc", 
                    help='Plot the adc, with toa and tot, all channels for each link')
parser.add_argument("-ptw", "--plot_time_walk", 
                    help='Plot the TOA time walk')
parser.add_argument("-st", "--style", type=Path,
                    help='Adress of matplotlib style sheet')
args = parser.parse_args()

samples = pd.read_csv(args.data)

print(samples)

if (args.style != None):
    plt.style.use(args.style)

def toa_conv(df):
    return (df['toa'] / 1024) * 50

if (args.plot_time_walk):
    fig, ax = plt.subplots(1,2)
    ana = samples[(samples['time'] == 25.0) & (samples['toa'] != 0.0)]
    chs = ana.groupby('channel')
    for ch_id, ch_df in chs:
        if (ch_id < 36):
            ax[0].plot(ch_df['calib'], toa_conv(ch_df))
        else:
            ax[1].plot(ch_df['calib'], toa_conv(ch_df))
    plt.show()

#quit()

if (args.plot_adc):
    fig, ax = plt.subplots(1,1)
    ana = samples[samples['channel'] < 36]
    ax.scatter(ana['time'], ana['adc'], label='adc')
    ax.scatter(ana['time'], ana['toa'], label='toa')
    ax.scatter(ana['time'], ana['tot'], label='tot')
    ax.legend()
    ax.set_xlabel('Time [ns]', fontsize=24)
    ax.set_ylabel('Output data', fontsize=24)
    fig.savefig('adc_link0.png', dpi=400, bbox_inches='tight')
    plt.close()

    fig, ax = plt.subplots(1,1)
    ana = samples[samples['channel'] >= 36]
    ax.scatter(ana['time'], ana['adc'], label='adc')
    ax.scatter(ana['time'], ana['toa'], label='toa')
    ax.scatter(ana['time'], ana['tot'], label='tot')
    ax.legend()
    ax.set_xlabel('Time [ns]', fontsize=24)
    ax.set_ylabel('Output data', fontsize=24)
    fig.savefig('adc_link1.png', dpi=400, bbox_inches='tight')
    plt.close()

fig_toa, ax_toa = plt.subplots(1, 2, sharey=True)
fig_tot, ax_tot = plt.subplots(1, 2, sharey=True)
fig_adc, ax_adc = plt.subplots(1, 1)
fig_ch, ax_ch = plt.subplots(8,9, sharex=True)

# Efficiency calculation
def efficiency(vals):
    counts = 0
    for val in vals:
        if (val > 0):
            counts += 1
    return counts/len(vals)

# When TOT is enabled, the ADC drops to -1. The real value is not necessarily 1023, so this
# condition is approximate
def condition_max(vals):
    if (min(vals) == -1):
        return 1023
    else:
        return max(vals)
# Make the median out of samples per timepoint, and get the efficiencies

# First chose a timepoint where we know that the toa and tot will trigger

time_trig = np.median(samples[samples['calib'] == samples['calib'].max()]
            .groupby(['channel', 'time'])
            .filter(lambda g: (g['tot'] > -1).all())
            ['time'].unique()
            )
samples = samples[samples['time'] == time_trig]
samples = (samples
            .groupby(['calib', 'channel'], as_index=False) # implement channels
            .agg(toa_eff=('toa', efficiency), 
                 tot_eff=('tot', efficiency),
                 max_adc=('adc', condition_max)))

# Plot data
ch_group = samples.groupby('channel')
max_non_saturated = 0
for ch_id, ch_df in ch_group:
    if ch_id == 0:
        ax_toa[0].plot(ch_df['calib'], ch_df['toa_eff'], label=f'channels', linewidth = 0.2)
        ax_tot[0].plot(ch_df['calib'], ch_df['tot_eff'], label=f'channels')
        ax_toa[1].plot(ch_df['max_adc'], ch_df['toa_eff'], label=f'channels')
        ax_tot[1].plot(ch_df['max_adc'], ch_df['tot_eff'], label=f'channels')
        ax_adc.scatter(ch_df['calib'], ch_df['max_adc'], label=f'channels')
    else:
        ax_toa[0].plot(ch_df['calib'], ch_df['toa_eff'])
        ax_tot[0].plot(ch_df['calib'], ch_df['tot_eff'])
        ax_toa[1].plot(ch_df['max_adc'], ch_df['toa_eff'])
        ax_tot[1].plot(ch_df['max_adc'], ch_df['tot_eff'])
        ax_adc.scatter(ch_df['calib'], ch_df['max_adc'])
    temp_df = ch_df[ch_df['max_adc'] != 1023]
    max_val = temp_df['max_adc'].max()
    if (max_val > max_non_saturated):
        max_non_saturated = max_val
    ax_ch[ch_id // 9, ch_id % 9].plot(ch_df['calib'], ch_df['toa_eff'], label=ch_id)

ax_adc.axhline(y = max_non_saturated, linestyle='--', color='b', label=f'max non-saturated at {max_non_saturated}')

ax_toa[0].legend(fontsize=12, ncols=6)
ax_tot[0].legend(fontsize=12, ncols=6)
ax_toa[1].legend(fontsize=12, ncols=6)
ax_tot[1].legend(fontsize=12, ncols=6)
ax_toa[0].set_xlabel('Calib [a.u.]')
ax_tot[0].set_xlabel('Calib [a.u.]')
ax_toa[1].set_xlabel('Max ADC [a.u.]')
ax_tot[1].set_xlabel('Max ADC [a.u.]')
ax_toa[0].set_ylabel('TOA efficiency')
ax_tot[0].set_ylabel('TOT efficiency')
ax_adc.legend(fontsize=12, ncols=6)
ax_adc.set_xlabel('Calib [a.u.]')
ax_adc.set_ylabel('Max ADC [a.u.]')
fig_toa.savefig("toa_s_curve.png", dpi=400)
fig_tot.savefig("tot_s_curve.png", dpi=400)
fig_adc.savefig("adc_linearity.png", dpi=400)
#fig_ch.savefig("toa_s_curve_channel.png", dpi=400)
plt.show()
#fig_toa.savefig('toa_efficiency.png', dpi=400, bbox_inches='tight')
#fig_tot.savefig('tot_efficiency.png', dpi=400, bbox_inches='tight')
