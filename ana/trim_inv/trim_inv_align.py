#Python script to find optimal trim_inv values given a trim_inv scan
import pandas as pd
import yaml
from scipy.stats import linregress as lr
import numpy as np
import argparse
import os
import sys


#runtime arguments
parser = argparse.ArgumentParser(
    prog = 'trim_inv_align.py',
    description='takes a csv from tasks.trim_inv_scan and outputs yaml file with optimal trim_inv values for pedestal trimming within each link'
)
parser.add_argument('-f', required = True, help='csv file containing scan from tasks.trim_inv_scan')
parser.add_argument('--output', help='name of output yaml file', default = 'trim_inv_output.yaml')
parser.add_argument('--min', type = int, help='minumum elink to be trimmed', default = 0)
parser.add_argument('--max', type = int, help='maximum elink to be trimmed', default = 1)
parser.add_argument('--target', type = float, help='number of standard deviations that target offset can be from median; play around with this value to find ideal pedestal trimming', default = 1)
args = parser.parse_args()

f = args.f
if not os.path.isfile(f):
    print(f + ' does not exist')
    sys.exit()
if not f.lower().endswith('csv'):
    print(f + ' is not a csv file')
    sys.exit()
output = args.output
if not output.lower().endswith('yaml'):
    output += '.yaml'
min_link = args.min
max_link = args.max
t_std = args.target
df = pd.read_csv(args.f, skiprows = 1)

#only 1 event per parameter value per channel
stats = pd.DataFrame(columns=['channel', 'slope', 'offset'])

#find slope, offset of each channel doing a fit of trim_inv vs adc
#hardcoded with channel ranges as of now, in future add i_link to csv in tasks.trim_inv_scan
for channel in range(min_link * 36, max_link * 36 + 36):
    if df[str(channel)].empty:
        continue
    adc = (df[str(channel)].to_numpy())
    trim_inv = (df['TRIM_INV']).to_numpy()
    slope, offset, r_value, p_value, slope_err = lr(trim_inv, adc)
    stats.loc[len(stats)] = [channel, slope, offset]

new_trim_inv = pd.DataFrame(columns=['channel', 'TRIM_INV'])

#find optimal trim_inv value for each channel using fit
for i in range (min_link, max_link + 1):
    mCH = i * 36
    nCH = i * 36 + 36
    link = stats.loc[(stats['channel'] >= mCH) & (stats['channel'] <nCH)]

    #target offset is max within t_std standard deviations of the median offset
    median_offset = link['offset'].median()
    std_offset = link['offset'].std()
    offset_mask = link.loc[abs(link['offset'] - median_offset) < t_std * std_offset]
    max_offset = offset_mask['offset'].max()

    #optimal trim_inv value for channel using fit and target offset
    for channel in range(mCH, nCH):
        if df[str(channel)].empty:
            continue
        offset = link.loc[link['channel'] == channel, 'offset'].values[0]
        slope = link.loc[link['channel'] == channel, 'slope'].values[0]

        if slope >0:
            trim_inv = round(abs(max_offset - offset) / slope)
        else:
            trim_inv = 0

        if trim_inv > 63:
            trim_inv=63
        

        new_trim_inv.loc[len(new_trim_inv)] = [channel, trim_inv]

#into yaml config file
trim_inv_dict = {
    f'CH_{int(row.channel)}': {'TRIM_INV': int(row.TRIM_INV)}
    for _, row in new_trim_inv.iterrows()
}
with open(output, 'w') as f:
    yaml.dump(trim_inv_dict, f, sort_keys=False)


