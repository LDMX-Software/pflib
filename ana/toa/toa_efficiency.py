# Calculates TOA_EFFICIENCY for a given dataset,
# requires a "toa_vref_scan.csv" file to run correctly.
# Then, plots the efficiency versus toa_vref and saves the plot.

import pandas as pd
import numpy as np
import yaml
import matplotlib.pyplot as plt
import os
import argparse
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).parent.parent))

from read import read_pflib_csv

#runtime arguments
parser = argparse.ArgumentParser(
    prog = 'toa_efficiency.py',
    description='takes a csv from tasks.toa_vref_scan and outputs a plot of TOA efficiency per channel against TOA VREF values'
)
parser.add_argument('-f', required = True, help='csv file containing scan from tasks.toa_vref_scan')
parser.add_argument('-r', '--roc', type = int, choices = [0, 1, 2, 3], required = True, help = 'ROC index to filter data (e.g., 0, 1, 2, 3)')
args = parser.parse_args()

if not os.path.isfile(args.f):
    print(args.f + ' does not exist')
    sys.exit()
if not args.f.lower().endswith('csv'):
    print(args.f + ' is not a csv file')
    sys.exit()
data, head = read_pflib_csv(args.f)

data = data.iloc[args.roc::4].copy()

data = data.sort_values(by='TOA_VREF').reset_index(drop = True)
print(f"Extracted {len(data)} voltage points for ROC {args.roc}")

"""
Example plot of how TOA_VREF plot should look. I credit the toa_vref_analysis.py
script from the TileboardQC Repository for HGCAL Tileboard Quality Control for this
graph and the idea to comment it into the script.

toa_efficiency vs Toa_vref
|        ++ +
|       +++ +
|       +++++
|       +++++
|       ++ ++
|       +++ +
|_______+++++_o______
|____________________

We want the TOA_VREF for each link to be just a bit higher than the pedestal variations
for that link. That way, we ensure only incoming signals trigger TOA, and we maximize
the signal-to-noise ratio. Select the point 'o' on the graph.
"""

vref_axis = data['TOA_VREF'].values

channel_lists = []
for chan in range(72):
    channel_lists.append(data[str(chan)].values)

# Plotting the results
plt.figure(figsize=(10, 6))
for chan in range(72):
    plt.plot(channel_lists[chan], 
             marker = 'o', 
             label=f'Ch. {chan}', 
             linestyle='none')
plt.title('TOA Efficiency vs TOA VREF per channel')
plt.xlabel('TOA VREF')
plt.ylabel('TOA Efficiency')
plt.tight_layout()
plt.savefig(f"toa_efficiency_plot_roc{args.roc}.png")
plt.show()
print(f"Plot saved as 'toa_efficiency_plot_roc{args.roc}.png'")

# Printing the values of highest non-zero TOA_VREF per link
link0 = np.array(channel_lists[:36]).T
link1 = np.array(channel_lists[36:]).T

link0_count = []
link1_count = []
for i in range(len(link0)):
    if not all(x == 0 for x in link0[i]):
        link0_count.append(i)
    if not all(x == 0 for x in link1[i]):
        link1_count.append(i)

if not link0_count or not link1_count:
    print("Error: Efficiency data contains only zeros for one or both links.")
    sys.exit()

max_vref_l0 = vref_axis[link0_count[-1]]
max_vref_l1 = vref_axis[link1_count[-1]]

optimal_vref_l0 = max_vref_l0 + 10
optimal_vref_l1 = max_vref_l1 + 10

print(f"for link 0, the highest non-zero toa_vref is {max_vref_l0} so set TOA_VREF to {optimal_vref_l0}")
print(f"for link 1, the highest non-zero toa_vref is {max_vref_l1} so set TOA_VREF to {optimal_vref_l1}")

#Outputting the values to a yaml file

df_max = pd.DataFrame({
    'link': [0, 1],
    'max_toa': [optimal_vref_l0, optimal_vref_l1]
}
)

#output to yaml file
yaml_data = {}
for _, row in df_max.iterrows():
    page_name = f"REFERENCEVOLTAGE_{int(row['link'])}"
    yaml_data[page_name] = {
        'TOA_VREF': int(row['max_toa'])
    }

yaml_filename = f"output_roc{args.roc}.yaml"
with open(yaml_filename, "w") as f:
    yaml.dump(yaml_data, f, sort_keys=False)

print(f"Output saved to '{yaml_filename}'")
