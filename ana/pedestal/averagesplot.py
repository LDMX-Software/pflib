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

# ------ PLOTTING ------

def plot_averages():

    ymin = []
    ymax = []

    if args.lowering_comparison:
        labels = ['pre-lowering', 'post-lowering']
    else:
        labels = np.arange(len(args.dataset))

    fig, (ax1, ax2) = plt.subplots(2,1,figsize=(12,8), height_ratios=[2,1])

    for n in range(0,len(args.dataset)):

        ymin.append([min(sample_avg_adc[n][0].ravel()),min(sample_avg_adc[n][1].ravel())])
        ymax.append([max(sample_avg_adc[n][0].ravel()),max(sample_avg_adc[n][1].ravel())])
        ax1.errorbar(channels, sample_avg_adc[n][0].ravel(), yerr=sample_error_adc[n][0].ravel(), label = f'Pedestals {labels[n]}', fmt='o')
        ax2.scatter(channels, sample_error_adc[n][0].ravel(),  label = f'Pedestals {labels[n]} RMS', marker='^')
        ax2.plot(channels, sample_error_adc[n][0].ravel(), linestyle='-', alpha=0.2)

    ymin = np.array(ymin)
    ymax = np.array(ymax)
    ax1.set_ylim([min(ymin[:,0]-10), max(ymax[:,0])+10])

    ax1.set_ylabel('Mean ADC [ADC units]', fontsize = 12)
    ax2.set_ylabel('RMS ADC [ADC units]', fontsize = 12)
    fig.supxlabel('Channels', fontsize=15)


    ax1.grid()
    ax2.grid(axis='x')

    ax1.legend(bbox_to_anchor=(1.1, 1), loc = 'upper right')
    ax2.legend(bbox_to_anchor=(1.1, 1), loc = 'upper right')

    fig.suptitle("Mean pedestals and their RMS on link 0", fontsize=14)

    if args.plot_directory:
        plt.savefig(os.path.join(args.plot_directory,'pedestal_avg_link0.png'), dpi=200)
    else:
        plt.show()

    fig2, (ax3, ax4) = plt.subplots(2,1,figsize=(12,8), height_ratios=[2,1])

    for n in range(0,len(args.dataset)):

        ax3.errorbar(channels, sample_avg_adc[n][1].ravel(), yerr=sample_error_adc[n][1].ravel(), label = f'Pedestals {labels[n]}', fmt='o')
        ax4.scatter(channels, sample_error_adc[n][1].ravel(),  label = f'Pedestals {labels[n]} RMS', marker='^')
        ax4.plot(channels, sample_error_adc[n][1].ravel(), linestyle='-', alpha=0.2)

    ax1.set_ylim([min(ymin[:,1]-10), max(ymax[:,1])+10])

    ax3.set_ylabel('Mean ADC [ADC units]', fontsize = 12)
    ax4.set_ylabel('RMS ADC [ADC units]', fontsize = 12)
    fig.supxlabel('Channels', fontsize=15)

    ax3.grid()
    ax4.grid(axis='x')

    ax3.legend(bbox_to_anchor=(1.1, 1), loc = 'upper right')
    ax4.legend(bbox_to_anchor=(1.1, 1), loc = 'upper right')

    fig2.suptitle("Mean pedestals and their RMS on link 1", fontsize=14)

    if args.plot_directory:
        plt.savefig(os.path.join(args.plot_directory,'pedestal_avg_link1.png'), dpi=200)
    else:
        plt.show()

def save_rms():

    if args.lowering_comparison:

        rms_results = {'link_0_pre_lowering' : [np.std(sample_avg_adc[0][0][0][1:])], 'link_1_pre_lowering' : [np.std(sample_avg_adc[0][1][0][1:])], # The last [0][1:] are unravelling a nested list and excluding the calib channel
                       'link_0_post_lowering' : [np.std(sample_avg_adc[1][0][0][1:])], 'link_1_post_lowering' : [np.std(sample_avg_adc[1][1][0][1:])]}
    else:
        rms_results = dict()
        for i in range(len(args.dataset)):
            rms_dataset = {f'link_0_pedestals_{i}' : [np.std(sample_avg_adc[i][0][0][1:])], f'link_1_pedestals_{i}' : [np.std(sample_avg_adc[i][1][0][1:])]}
            rms_results = rms_results | rms_dataset

    df = pd.DataFrame(rms_results)
    df.to_csv(args.rms_csv)

# ------ MAIN ------

plot_averages()

if args.rms_csv: save_rms()