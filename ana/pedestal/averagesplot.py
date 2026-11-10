import pandas as pd
import matplotlib.pyplot as plt
import argparse
import numpy as np
import os
from pathlib import Path

# ------ ARGUMENT PARSER ------

parser = argparse.ArgumentParser()
parser.add_argument('dataset', type=Path, nargs='+', help='Pedestal dataset csv path (allows multiple)')
parser.add_argument('-pd', '--plot_directory', type=Path, help='Path to directory for saved figures')
parser.add_argument('-lc', '--lowering_comparison', action='store_true', help='Mark the first pedestal file as pre-lowering and second as post-lowering')
parser.add_argument('-rms', '--rms_csv', type=Path, help='Calculate and save the RMS of the mean pedestals for each link - provide a file path')
args = parser.parse_args()

if not args.dataset:
    print('Dataset not provided! Use -h to view script options.')

# ------ DATA READING ------

# inspect first pedestal file to see if we are in i_link/i_ch or roc/ch indexing
with open(args.dataset[0]) as f:
    header = f.readline()

i_link_i_ch_indexing = 'i_link' in header
label = 'link' if i_link_i_ch_indexing else 'roc'
major_column = 'i_link' if i_link_i_ch_indexing else 'i_roc'
if i_link_i_ch_indexing:
    channels = ['calib']+[str(i) for i in range(36)]
else:
    channels = ['calib_0', 'calib_1']+[str(i) for i in range(72)]
n_calib = sum(ch.startswith('calib') for ch in channels)

ids = np.sort(pd.read_csv(args.dataset[0])[major_column].unique())

# sample_avg_adc[n][id] is the per-channel mean of dataset n
sample_avg_adc = []
sample_error_adc = []

for set in args.dataset:
    pedestal = pd.read_csv(set)
    pedestal['channel'] = pedestal['channel'].astype(str)

    avg_adc = dict()
    error_adc = dict()
    for i in ids:
        avg_adc_channel = []
        error_adc_channel = []

        for channel in channels:
            adc = pedestal[(pedestal[major_column]==i)&(pedestal.channel==channel)].adc.to_numpy()
            avg_adc_channel.append(np.mean(adc))
            error_adc_channel.append(np.std(adc))

        avg_adc[i] = np.array(avg_adc_channel)
        error_adc[i] = np.array(error_adc_channel)
    sample_avg_adc.append(avg_adc)
    sample_error_adc.append(error_adc)

# ------ PLOTTING ------

def plot_averages():

    if args.lowering_comparison:
        labels = ['pre-lowering', 'post-lowering']
    else:
        labels = np.arange(len(args.dataset))

    for i in ids:
        fig, (ax1, ax2) = plt.subplots(2,1,figsize=(12,8), height_ratios=[2,1])

        ymin = []
        ymax = []
        for n in range(0,len(args.dataset)):

            ymin.append(np.nanmin(sample_avg_adc[n][i]))
            ymax.append(np.nanmax(sample_avg_adc[n][i]))
            ax1.errorbar(channels, sample_avg_adc[n][i], yerr=sample_error_adc[n][i], label = f'Pedestals {labels[n]}', fmt='o')
            ax2.scatter(channels, sample_error_adc[n][i],  label = f'Pedestals {labels[n]} RMS', marker='^')
            ax2.plot(channels, sample_error_adc[n][i], linestyle='-', alpha=0.2)

        ax1.set_ylim([min(ymin)-10, max(ymax)+10])

        ax1.set_ylabel('Mean ADC [ADC units]', fontsize = 12)
        ax2.set_ylabel('RMS ADC [ADC units]', fontsize = 12)
        fig.supxlabel('Channels', fontsize=15)
        if not i_link_i_ch_indexing:
            ax1.tick_params(axis='x', labelrotation=90, labelsize='x-small')
            ax2.tick_params(axis='x', labelrotation=90, labelsize='x-small')

        ax1.grid()
        ax2.grid(axis='x')

        ax1.legend(bbox_to_anchor=(1.1, 1), loc = 'upper right')
        ax2.legend(bbox_to_anchor=(1.1, 1), loc = 'upper right')

        fig.suptitle(f"Mean pedestals and their RMS on {label} {i}", fontsize=14)

        if args.plot_directory:
            fig.savefig(os.path.join(args.plot_directory,f'pedestal_avg_{label}{i}.png'), dpi=200)
        else:
            plt.show()

def save_rms():

    # exclude the calib channels
    if args.lowering_comparison:
        rms_results = dict()
        for i in ids:
            rms_results[f'{label}_{i}_pre_lowering'] = [np.std(sample_avg_adc[0][i][n_calib:])]
        for i in ids:
            rms_results[f'{label}_{i}_post_lowering'] = [np.std(sample_avg_adc[1][i][n_calib:])]
    else:
        rms_results = dict()
        for n in range(len(args.dataset)):
            for i in ids:
                rms_results[f'{label}_{i}_pedestals_{n}'] = [np.std(sample_avg_adc[n][i][n_calib:])]

    df = pd.DataFrame(rms_results)
    df.to_csv(args.rms_csv)

# ------ MAIN ------

plot_averages()

if args.rms_csv: save_rms()
