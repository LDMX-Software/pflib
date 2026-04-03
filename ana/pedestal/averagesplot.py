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

channels = ['calib']+[str(i) for i in range(36)]

sample_avg_adc = []
sample_error_adc = []

for set in args.dataset:
    pedestal = pd.read_csv(set)

    link0_avg_adc = []
    link1_avg_adc = []
    link0_error_adc = []
    link1_error_adc = []

    for link in range(2):
        avg_adc_channel = []
        error_adc_channel = []

        for channel in channels:
            avg_adc_channel.append(np.mean(pedestal[(pedestal.i_link==link)&(pedestal.channel==channel)].adc.to_numpy()))
            error_adc_channel.append(np.std(pedestal[(pedestal.i_link==link)&(pedestal.channel==channel)].adc.to_numpy()))

        if link == 0:
            link0_avg_adc.append(np.array(avg_adc_channel))
            link0_error_adc.append(np.array(error_adc_channel))
        else:
            link1_avg_adc.append(np.array(avg_adc_channel))
            link1_error_adc.append(np.array(error_adc_channel))
    sample_avg_adc.append(np.array((link0_avg_adc,link1_avg_adc)))
    sample_error_adc.append(np.array((link0_error_adc,link1_error_adc)))

sample_avg_adc = np.array(sample_avg_adc)
sample_error_adc = np.array(sample_error_adc)

ymin = []
ymax = []

fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(12,8))
for n in range(0,len(args.dataset)):
    
    ymin.append([min(sample_avg_adc[n][0].ravel()),min(sample_avg_adc[n][1].ravel())])
    ymax.append([max(sample_avg_adc[n][0].ravel()),max(sample_avg_adc[n][1].ravel())])

    ax1.errorbar(channels, sample_avg_adc[n][0].ravel(), yerr=sample_error_adc[n][0].ravel(), label = f'Pedestals {n}, link 0', fmt='o')
    ax2.errorbar(channels, sample_avg_adc[n][1].ravel(), yerr=sample_error_adc[n][1].ravel(), label = f'Pedestals {n}, link 1', fmt='o')
ax1.grid()
ax2.grid()

ymin = np.array(ymin)
ymax = np.array(ymax)

ax1.set_ylim([min(ymin[:,0]-50), max(ymax[:,0])+50])
ax2.set_ylim([min(ymin[:,1]-50), max(ymax[:,1])+50])
ax1.set(xlabel='Channels', ylabel = 'ADC [a.u.]')
ax2.set(xlabel='Channels', ylabel = 'ADC [a.u.]')
ax1.legend()

plt.show()


