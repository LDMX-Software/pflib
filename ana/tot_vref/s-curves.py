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

fig_toa, ax_toa = plt.subplots(1, 1)
fig_tot, ax_tot = plt.subplots(1, 1)

# For some reason, TOA and TOT is always 0 on link 1
# My theory is that TOA_vref isn't set correctly
samples = samples[samples['channel'] < 36]

ana = samples[samples['channel'] == 13]

gp = ana.groupby(['calib'])

for i, df in gp:
    plt.scatter(df['time'], df['tot'])
    plt.show()

# Efficiency calculation
def efficiency(vals):
    counts = 0
    for val in vals:
        if (val > 0):
            counts += 1
    return counts/len(vals)

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
for ch_id, ch_df in ch_group:
    ax_toa.scatter(ch_df['calib'], ch_df['toa_eff'], label=f'ch {ch_id}')
    ax_tot.scatter(ch_df['calib'], ch_df['tot_eff'], label=f'ch {ch_id}')

ax_toa.legend(fontsize=12, ncols=6)
ax_tot.legend(fontsize=12, ncols=6)
plt.show()
#fig_toa.savefig('toa_efficiency.png', dpi=400, bbox_inches='tight')
#fig_tot.savefig('tot_efficiency.png', dpi=400, bbox_inches='tight')
