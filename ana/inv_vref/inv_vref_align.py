#Python script to find optimal inv_vref values given an inv_vref scan
#to be done after already leveling pedestals
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
    prog = 'inv_vref_align.py',
    description='takes a csv from tasks.inv_vref_scan and outputs yaml file with optimal inv_vref values for pedestal trimming between links' \
    '; Do after leveling pedestals'
)
parser.add_argument('-f', required = True, help='csv file containing scan from tasks.inv_vref_scan')
parser.add_argument('-o', help='name of output yaml file', default = 'inv_vref_output.yaml')
parser.add_argument('-t', help='target adc', default = 100, type = int)
args = parser.parse_args()

if not os.path.isfile(args.f):
    print(args.f + ' does not exist')
    sys.exit()
if not args.f.lower().endswith('csv'):
    print(args.f + ' is not a csv file')
    sys.exit()
output = args.o
if not output.lower().endswith('yaml'):
    output += '.yaml'
df, head = read_pflib_csv(args.f)

target = args.t

channels = (df.columns[df.columns != 'INV_VREF'].to_numpy()).astype(int)

#perform linear regression on adc vs inv_vref for each channel (one channel per link)
stats = pd.DataFrame(columns=['channel', 'slope', 'offset'])
#linear regime between 10% and 80% in the range of adc values
for channel in channels:
    max = df[str(channel)].max()
    min = df[str(channel)].min()
    df1 = df.loc[(df[str(channel)] > (.1 * (max-min)) + min ) & (df[str(channel)] < (.8 * (max-min)) + min )]
    adc = df1[str(channel)].to_numpy()
    inv_vref = df1['INV_VREF'].to_numpy()
    #print
    slope, offset, r_value, p_value, slope_err = lr(inv_vref, adc)
    stats.loc[len(stats)] = [channel, slope, offset]

#find optimal inv_
optimal = pd.DataFrame(columns=['channel', 'inv_vref'])
for channel in channels:
    offset = stats.loc[stats['channel'] == channel, 'offset'].values[0]
    slope = stats.loc[stats['channel'] == channel, 'slope'].values[0]
    optimal_inv_vref = round((target - offset) / slope)
    optimal.loc[len(optimal)] = [channel, optimal_inv_vref]

# Construct final YAML dictionary
inv_vref_dict = {}

for _, row in optimal.iterrows():
    ch = int(row['channel'])
    page = f'REFERENCEVOLTAGE_{0 if ch <= 35 else 1}'
    inv_vref_dict[page] = {'INV_VREF': int(row['inv_vref'])}

# Write to YAML
with open(output, 'w') as f:
    yaml.dump(inv_vref_dict, f, sort_keys=False)