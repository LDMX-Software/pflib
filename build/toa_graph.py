import os
import argparse
from pathlib import Path

import pandas as pd
import matplotlib.pyplot as plt
import matplotlib as mpl
import numpy as np


parser = argparse.ArgumentParser()
parser.add_argument('data', type=Path, help='decoded CSV file to analyze')
parser.add_argument('ch', type=int, help='channel to analyze')
#only takes data from roc = 1
args = parser.parse_args()

data_raw = pd.read_csv(args.data, header = None)

ch = args.ch

trim_toa = []
calib = []
efficiency = []


print(data_raw.head())
for _, row in data_raw.iterrows():
  #if row[0] == 0:
    if (row[1] == ch):
      trim_toa.append(row[2])
      calib.append(row[3])
      efficiency.append(row[4])
 #else:
  #   continue


# set up the figure and Axes
fig = plt.figure(figsize=(8, 3))
ax = fig.add_subplot(projection='3d')
# 3. Plot the points (color mapped by their Z values)
scatter = ax.scatter(calib, trim_toa, efficiency, c=efficiency, cmap='viridis', s=40, edgecolors='w')

# 4. Add labels, a colorbar, and titles
ax.set_xlabel('Calib')
ax.set_ylabel('Trim_toa')
ax.set_zlabel('Efficiency')
ax.set_title(f'Plot of Efficiency {ch}' )
fig.colorbar(scatter, ax=ax, label='Z Depth')

# 5. Display the window
plt.show()
