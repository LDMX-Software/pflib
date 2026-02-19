"""
Plots the s-curves of toa and tot efficencies, on all channels.

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
args = parser.parse_args()

samples = pd.read_csv(args.data)

plt.rcParams['figure.figsize'] = [15, 12]
plt.rcParams['xtick.labelsize'] = 16; plt.rcParams['ytick.labelsize'] = 16

# For some reason, TOA and TOT is always 0 on link 1
# My theory is that TOA_vref isn't set correctly
#samples = samples[samples['channel'] >= 36]

#ana = samples[samples['channel'] == 37]
ana = samples[samples['channel'] < 36]
plt.scatter(ana['time'], ana['adc'], label='adc')
plt.scatter(ana['time'], ana['toa'], label='toa')
plt.scatter(ana['time'], ana['tot'], label='tot')
plt.scatter(ana['time'], 100*ana['Tp'], label=r'100$\times$Tp')
plt.scatter(ana['time'], 100*ana['Tc'], label=r'100$\times$Tc')
plt.legend()
plt.show()
plt.clf()

ana = samples[samples['channel'] >= 36]
plt.scatter(ana['time'], ana['adc'], label='adc')
plt.scatter(ana['time'], ana['toa'], label='toa')
plt.scatter(ana['time'], ana['tot'], label='tot')
plt.scatter(ana['time'], 100*ana['Tp'], label=r'100$\times$Tp')
plt.scatter(ana['time'], 100*ana['Tc'], label=r'100$\times$Tc')
plt.legend()
plt.show()

gp = ana.groupby(['calib'])

#for i, df in gp:
#    plt.scatter(df['time'], df['adc'], label='adc')
#    plt.scatter(df['time'], df['toa'], label='toa')
#    plt.legend()
#    plt.show()

fig_toa, ax_toa = plt.subplots(1, 1)
fig_tot, ax_tot = plt.subplots(1, 1)
fig_adc, ax_adc = plt.subplots(1, 1)

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
samples = (samples
            .groupby(['calib', 'channel'], as_index=False) # implement channels
            .agg(toa_eff=('toa', efficiency), 
                 tot_eff=('tot', efficiency),
                 max_adc=('adc', condition_max)))

#ax_toa.plot(samples['CALIB'], samples['toa_eff'])
#ax_tot.plot(samples['CALIB'], samples['tot_eff'])
# Plot data
ch_group = samples.groupby('channel')
max_non_saturated = 0
for ch_id, ch_df in ch_group:
    if ch_id >= 0:
        ax_toa.plot(ch_df['max_adc'], ch_df['toa_eff'], label=f'ch {ch_id}')
        ax_tot.plot(ch_df['max_adc'], ch_df['tot_eff'], label=f'ch {ch_id}')
        ax_adc.scatter(ch_df['calib'], ch_df['max_adc'], label=f'ch {ch_id}')
    #else:
    #    ax_toa.plot(ch_df['calib'], ch_df['toa_eff'])
    #    ax_tot.plot(ch_df['calib'], ch_df['tot_eff'])
    #    ax_adc.scatter(ch_df['calib'], ch_df['max_adc'])
    temp_df = ch_df[ch_df['max_adc'] != 1023]
    max_val = temp_df['max_adc'].max()
    if (max_val > max_non_saturated):
        max_non_saturated = max_val

ax_adc.axhline(y = max_non_saturated, linestyle='--', color='b', label=f'max non-saturated at {max_non_saturated}')

ax_toa.legend(fontsize=12, ncols=6)
ax_tot.legend(fontsize=12, ncols=6)
ax_adc.legend(fontsize=12, ncols=6)
plt.show()
#fig_toa.savefig('toa_efficiency.png', dpi=400, bbox_inches='tight')
#fig_tot.savefig('tot_efficiency.png', dpi=400, bbox_inches='tight')
