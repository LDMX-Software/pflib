import pandas as pd
import yaml
import argparse
import os
import sys
from pathlib import Path
from read import read_pflib_csv

#runtime arguments
parser = argparse.ArgumentParser()

parser.add_argument('-f', required = True, help='csv file containing toa_vref scan')
args = parser.parse_args()

if not os.path.isfile(args.f):
    print(args.f + ' does not exist')
    sys.exit()

if not args.f.lower().endswith('csv'):
    print(args.f + ' is not a csv file')
    sys.exit()

df, head = read_pflib_csv(args.f)

#Calculate toa efficiency of each parameter point in each channel
efficiency = pd.DataFrame(columns=['channel', 'TOA_VREF', 'TOA_EFF'])
for channel in range (72):
    for toa_vref in range(2**8):
        select = df.loc[df['TOA_VREF'] == toa_vref][str(channel)]
        num_non_zero = len(select[select != 0])
        efficiency.loc[len(efficiency)] = [channel, toa_vref, num_non_zero / len(select)]

#find highest non zero TOA_VREF value per link
link0 = efficiency[efficiency['channel'] < 36]
link1 = efficiency[efficiency['channel'] > 35]
df_max = pd.DataFrame(columns = ['link', 'max_toa'])

link_num=0

for link in [link0, link1]:
    non_zero_eff = link[link['TOA_EFF'] != 0]
    max_toa = max(non_zero_eff['TOA_VREF'])
    df_max.loc[len(df_max)] = [link_num, max_toa]
    link_num += 1

#output to yaml file
yaml_data = {}
for _, row in df_max.iterrows():
    page_name = f"REFERENCEVOLTAGE_{int(row['link'])}"
    yaml_data[page_name] = {
        'TOA_VREF': int(row['max_toa'])
    }

with open("output.yaml", "w") as f:
    yaml.dump(yaml_data, f, sort_keys=False)


