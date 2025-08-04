#Python script to find optimal trim_inv values given a trim_inv scan
import pandas as pd
import yaml
from scipy.stats import linregress as lr
import numpy as np
import argparse
import os
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).parent.parent))

from read import read_pflib_csv


#runtime arguments
parser = argparse.ArgumentParser(
    prog = 'trim_inv_align.py',
    description='takes a csv from tasks.trim_inv_scan and outputs yaml file with optimal trim_inv values for pedestal trimming within each link'
)
parser.add_argument('-f', required = True, help='csv file containing scan from tasks.trim_inv_scan')
parser.add_argument('--output', help='name of output yaml file', default = 'trim_inv_output.yaml')
args = parser.parse_args()

if not os.path.isfile(args.f):
    print(args.f + ' does not exist')
    sys.exit()
if not args.f.lower().endswith('csv'):
    print(args.f + ' is not a csv file')
    sys.exit()
output = args.output
if not output.lower().endswith('yaml'):
    output += '.yaml'
df, head = read_pflib_csv(args.f)

#only 1 event per parameter value per channel
stats = pd.DataFrame(columns=['channel', 'slope', 'offset'])

#find slope, offset of each channel doing a fit of trim_inv vs adc with dacb = 0
df1 = df.loc[df['DACB'] == 0]
for channel in range(0, 72):
    if df1[str(channel)].empty:
        continue
    adc = (df1[str(channel)].to_numpy())
    trim_inv = (df1['TRIM_INV']).to_numpy()
    slope, offset, r_value, p_value, slope_err = lr(trim_inv, adc)
    stats.loc[len(stats)] = [channel, slope, offset]


new_trim_inv = pd.DataFrame(columns=['channel', 'TRIM_INV'])

#find optimal trim_inv value for each channel using fit
targets = np.empty(2)

for i in range (0, 2):
    mCH = i * 36
    nCH = i * 36 + 36
    link = stats.loc[(stats['channel'] >= mCH) & (stats['channel'] <nCH)]

    #only select from non-outlier channels
    median_offset = link['offset'].median()
    std_offset = link['offset'].std()
    offset_mask = link.loc[abs(link['offset'] - median_offset) < 2 * std_offset]

    #target adc is adc of lowest channel set at max trim_inv
    lowest = offset_mask.loc[offset_mask['offset'] == offset_mask['offset'].min()]
    target = lowest['offset'].values[0] + 63 * lowest['slope'].values[0]
    targets[i] = target


    #optimal trim_inv value for channel using fit and target offset
    for channel in range(mCH, nCH):
        if df[str(channel)].empty:
            continue
        offset = link.loc[link['channel'] == channel, 'offset'].values[0]
        slope = link.loc[link['channel'] == channel, 'slope'].values[0]

        if slope >0:
            if offset > target:
                trim_inv = 0
            else:
                trim_inv = round((target - offset) / slope)
        else:
            trim_inv = 0

        if trim_inv > 63:
            trim_inv=63
        

        new_trim_inv.loc[len(new_trim_inv)] = [channel, trim_inv]

dacb_stats = pd.DataFrame(columns=['channel', 'slope', 'offset'])

#find slope, offset of each channel doing a fit of trim_inv vs adc with dacb = 0
dacb_channels = new_trim_inv.loc[new_trim_inv['TRIM_INV'] == 0, 'channel'].to_numpy()
df1 = df.loc[df['TRIM_INV'] == 0]
for channel in dacb_channels:
    if df1[str(channel)].empty:
        continue

    #linear regime since adc bottoms out at dacb~=40
    linear_region = df1.loc[df1[str(channel)] > 0]
    adc = linear_region[str(channel)].to_numpy()
    dacb = linear_region['DACB'].to_numpy()
    if len(adc) == 0:
        continue
    slope, offset, r_value, p_value, slope_err = lr(dacb, adc)
    dacb_stats.loc[len(dacb_stats)] = [channel, slope, offset]

#from here get channels that have 0 set as trim_inv and find appropriate dacb
dacb_values = pd.DataFrame(columns=['channel', 'DACB', "SIGN_DAC"])
for i, channel in enumerate(dacb_channels):
    if channel < 36:
        target = targets[0]
    else:
        target = targets[1]
    
    row = dacb_stats.loc[dacb_stats['channel'] == channel]
    if row.empty:
        continue

    offset = row['offset'].values[0]
    slope = row['slope'].values[0]

    if slope == 0:
        continue
    new_dacb = -1* round(abs(target - offset) / slope)
    dacb_values.loc[len(dacb_values)] = [channel, new_dacb, 1]

#into yaml config file
import yaml
# Convert dacb_values to dicts for quick lookup
dacb_dict = dacb_values.set_index('channel')['DACB'].to_dict()
sign_dac_dict = dacb_values.set_index('channel')['SIGN_DAC'].to_dict()

combined_dict = {}

for _, row in new_trim_inv.iterrows():
    ch_key = f'CH_{int(row.channel)}'
    combined_dict[ch_key] = {'TRIM_INV': int(row.TRIM_INV)}
    
    ch = int(row.channel)
    if ch in dacb_dict:
        combined_dict[ch_key]['DACB'] = int(dacb_dict[ch])
        combined_dict[ch_key]['SIGN_DAC'] = int(sign_dac_dict[ch])

with open(output, 'w') as f:
    yaml.dump(combined_dict, f, sort_keys=False)

