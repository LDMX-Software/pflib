import pandas as pd
import matplotlib.pyplot as plt
import argparse
import numpy as np
from pathlib import Path

parser = argparse.ArgumentParser()
parser.add_argument('dataset', type=Path, nargs='+', help='Pedestal dataset csv path (allows multiple)')
parser.add_argument('-o', '--output', type=Path, help='Save the figure with the given path')
args = parser.parse_args()

if not args.dataset:
    print('Dataset not provided! Use -h to view script options.')

link0_avg_adc = []
link1_avg_adc = []

link0_error_adc = []
link1_error_adc = []

for set in args.dataset:
    pedestal = pd.read_csv(set)
    for link in range(2):
        avg_adc = {'calib': []} | {str(i): [] for i in range(36)}
        error_adc = {'calib': []} | {str(i): [] for i in range(36)}
        for channel in avg_adc.keys():
            avg_adc[channel].append(np.mean(pedestal[(pedestal.i_link==link)&(pedestal.channel==channel)].adc.to_numpy()))
            error_adc[channel].append(np.std(pedestal[(pedestal.i_link==link)&(pedestal.channel==channel)].adc.to_numpy()))
        if link == 0:
            link0_avg_adc.append(avg_adc)
            link0_error_adc.append(error_adc)
        else:
            link1_avg_adc.append(avg_adc)
            link1_error_adc.append(error_adc)

fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(12,8), sharex=True)

for n in range(0,len(args.dataset)):

    ax1.errorbar(avg_adc.keys(), link0_avg_adc[n].values(), yerr=link0_error_adc[n].values(), label = f'Pedestals {n}, link 0')
    ax2.errorbar(avg_adc.keys(), link1_avg_adc[n].values(), yerr=link1_error_adc[n].values(), label = f'Pedestals {n}, link 1')

ax1.set(xlabel='Channels', ylabel = 'ADC [a.u.]')
ax2.set(xlabel='Channels', ylabel = 'ADC [a.u.]')
ax1.legend()

plt.show()


