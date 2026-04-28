import numpy as np
import matplotlib.pyplot as plt
import pandas as pd
import json
import scipy.stats as stats
import os

from pathlib import Path
import argparse

# ------- PARSER -------

HR_thresholds = {"CALIB_0":10, "CALIB_32":24, "CALIB_64":42, "CALIB_256":100, 
                     "CALIB_512":200, "CALIB_1024":200, "CALIB_2048":200, "CALIB_4096":200}
plot_types = ['CLUSTER', 'SINGULAR', 'EVALUATION']

parser = argparse.ArgumentParser()
parser.add_argument('dataset', type=Path, nargs='+', help='Parameter timescan of one channel, one CALIB, n samples per time-point')
parser.add_argument('-s', '--samples', type=int, help='Number of samples per phase')
parser.add_argument('-cs', '--cluster_scan', action='store_true', help='Perform an outliers cluster-scan (default)')
parser.add_argument('-ts', '--threshold_scan', action='store_true', help='Perform a threshold outliers-scan with default thresholds (only HR works)')
parser.add_argument('-ph', '--phase_analysis', action='store_true', help='Perform and plot phase analysis of the outliers (include -pd for plot directory path)')
parser.add_argument('-be', '--bulk_evaluation', action='store_true', help='Perform analysis to evaluate multiple channels and CALIBs in terms of outliers found')
parser.add_argument('-p', '--plot', choices=plot_types, type=str, help=f'Plot results. Available types: {", ".join(plot_types)}')
parser.add_argument('-pd', '--plot_directory', type=Path, help='Figures directory path. If not provided, figures are not saved automatically')
parser.add_argument('-csv', '--csv', type=Path, help='Save the scan results to a csv with the given directory path')
args = parser.parse_args()

if not args.cluster_scan and not args.threshold_scan : 
    print("Please select the outlier-scan type. For more information, use '-h' or '--help'.")

# ------- MEASUREMENTS CLASS -------

class measurements():

    def __init__(self, dataset, sample, parameters):

        t = dataset[0]
        val = dataset[1]
        c = dataset[2][0]
        ch = dataset[3][0]

        t, val = zip(*sorted(zip(t, val))) # ordering the time and adc from first to last phase

        self.time = np.array(t)
        self.adc = np.array(val)
        self.dadc = []
        self.calib = int(c)
        self.channel = int(ch)
        self.sample = sample
        self.parameters = parameters

        if parameters["preCC"]: self.scan_type = "preCC"
        else: self.scan_type = "highrange"

        self.outliers_time = []
        self.outliers_adc = []
        self.outliers_number = 0
        self.missing_adc = len(self.adc[self.adc == -1])

        self.potential_outliers_time = []
        self.potential_outliers_adc = []
        self.potential_outliers_number = 0

    def delta_adc(self):
        adc = self.adc
        adc_shiftup = np.insert(adc, 0, 0)
        adc_shiftdown = np.insert(adc, len(adc), 0)

        dadc = adc_shiftdown - adc_shiftup
        self.dadc = dadc[1:-1]

# ------- LOADING & SORTING DATA -------

def read_data(path: Path):
    
    with open(path) as file:
        header = file.readline().strip('\n')
    has_pflib_header = header[0] == '#'
    run_params = (
        json.loads(header.strip('#'))
        if has_pflib_header else
        {}
    ) # gives me info on data type (HR/LR/preCC)!
    if has_pflib_header:
        data = pd.read_csv(path, skiprows=1, dtype=np.float64) # will probably add **kwargs later

    if args.samples: run_params["samples"] = args.samples
    else:
        time = data["time"].to_list()
        time_unique = list(np.unique(data["time"].to_numpy()))
        run_params["samples"] = time.count(time_unique[0])

    return data, run_params

def sort_data(raw_data, raw_data_parameters):
    
    sorted_data = []
    raw_adc = raw_data["adc"].to_numpy()
    raw_time = raw_data["time"].to_numpy()
    raw_channels = raw_data["channel"].to_numpy()
    link = round(raw_channels[0]/71)

    if raw_data_parameters["preCC"]: raw_calibs = raw_data[f"REFERENCEVOLTAGE_{link}.CALIB_2V5"]
    else: 
        raw_calibs = raw_data[f"REFERENCEVOLTAGE_{link}.CALIB"]
        

    scan_length = len(np.unique(raw_time))

    raw_time_copy = raw_time
    n = 1
    for i in range(1,len(raw_time_copy)):
        if raw_time_copy[i] == raw_time_copy[i-1]: 
            n += 1
        else:
            if n == raw_data_parameters["samples"]:
                n = 1
                continue
            else:
                dummy_sample_size = raw_data_parameters["samples"] - n
                print(f"Timepoint {raw_time_copy[i]}, missing {dummy_sample_size} samples!")
                dummy_time = raw_time_copy[i-1]
                dummy_calib = raw_calibs[i-1]
                dummy_channel = raw_channels[i-1]
                for j in range(dummy_sample_size):
                    raw_adc = np.insert(raw_adc, i, -1)
                    raw_time = np.insert(raw_time, i, dummy_time)
                    raw_calibs = np.insert(raw_calibs, i, dummy_calib)
                    raw_channels = np.insert(raw_channels, i, dummy_channel)
                n = 1

    for s in range(0, raw_data_parameters["samples"]):
        sample_time_data = []
        sample_adc_data = []
        sample_calib_data = []
        sample_channel_data = []
        for n in range(0, scan_length):
            sample_time_data.append((raw_time[n*raw_data_parameters["samples"]+s]))
            sample_adc_data.append((raw_adc[n*raw_data_parameters["samples"]+s]))
            sample_calib_data.append((raw_calibs[n*raw_data_parameters["samples"]+s]))
            sample_channel_data.append((raw_channels[n*raw_data_parameters["samples"]+s]))
        sorted_data.append(([sample_time_data, sample_adc_data, sample_calib_data, sample_channel_data]))

    return np.array(sorted_data)

def classify_data(sorted_data, sorted_data_parameters):

    classified_data = []

    n = 0
    for data_array in sorted_data: 
        cdata = measurements(data_array, n, sorted_data_parameters)
        classified_data.append(cdata)
        n += 1

    return classified_data

# -------  OUTLIER SEARCH -------

def threshold_outlier_search(dataset : list, threshold : int):

    for sample in dataset:
        sample.delta_adc()
        delta_adc = sample.dadc

        for i in range(0, len(sample.time)-1):
            if np.abs(delta_adc[i]) > threshold:
                sample.outliers_time.append(sample.time[i])
                sample.outliers_adc.append(sample.adc[i])
                sample.outliers_number += 1
    
def cluster_outlier_search(dataset : list):

    if not args.samples : 
        if dataset[0].parameters['samples'] < 10: 
            print("Please ensure at least 10 samples of the pulse are available.")
            return
    
    adc_distributions = []
    counts_distributions = []
    mode_indices = []

    for n in range(0,len(dataset[0].time)):
        points = []
        for sample in dataset:
            points.append(sample.adc[n])
        points_unique = list(np.unique(points))
        counts = []
        for point in points_unique:
            counts.append(points.count(point))
        mode = stats.mode(points).mode
        mode_indices.append(points_unique.index(mode))
        adc_distributions.append(points_unique)
        counts_distributions.append(counts)
    
    for i in range(0,len(adc_distributions)):
        time = dataset[0].time[i]
        print(f"Time-point {time}")
        adc_above = adc_distributions[i][mode_indices[i]]
        adc_below = adc_distributions[i][mode_indices[i]]

        cluster = 0
        gap = 0
        print("RHS")
        while adc_above <= max(adc_distributions[i]):
            if adc_above in adc_distributions[i]: 
                print(f"ADC {adc_above} in same cluster\n")
                if cluster == 1:
                    for sample in dataset:
                        if adc_above == sample.adc[i]: 
                            sample.potential_outliers_adc.append(adc_above)
                            sample.potential_outliers_time.append(time)
                            sample.potential_outliers_number += 1
                elif cluster == 2:
                    for sample in dataset:
                        if adc_above == sample.adc[i]: 
                            sample.outliers_adc.append(adc_above)
                            sample.outliers_time.append(time)
                            sample.outliers_number += 1
                if gap != 0: 
                    if gap == 1:
                        print("Gap is too small - ignoring")
                    elif 2 <= gap < 5:
                        if cluster == 2: # if the previous cluster was marked as an outlier, a disjointed cluster after it is also an outlier
                            for sample in dataset:
                                if adc_above == sample.adc[i]: 
                                    sample.outliers_adc.append(adc_above)
                                    sample.outliers_time.append(time)
                                    sample.outliers_number += 1

                        else:
                            for sample in dataset:
                                if adc_above == sample.adc[i]: 
                                    sample.potential_outliers_adc.append(adc_above)
                                    sample.potential_outliers_time.append(time)
                                    sample.potential_outliers_number += 1
                            cluster = 1
                    elif gap >= 5:
                        for sample in dataset:
                            if adc_above == sample.adc[i]: 
                                sample.outliers_adc.append(adc_above)
                                sample.outliers_time.append(time)
                                sample.outliers_number += 1
                        cluster = 2
                gap = 0
            else: 
                print(f"ADC {adc_above} not in same cluster\n")
                gap += 1
            adc_above += 1
        
        cluster = 0
        print("LHS")
        while adc_below >= min(adc_distributions[i]):
            if adc_below in adc_distributions[i]: 
                print(f"ADC {adc_below} in same cluster\n")
                if cluster == 1:
                    for sample in dataset:
                        if adc_below == sample.adc[i]: 
                            sample.potential_outliers_adc.append(adc_below)
                            sample.potential_outliers_time.append(time)
                            sample.potential_outliers_number += 1
                elif cluster == 2:
                    for sample in dataset:
                        if adc_below == sample.adc[i]: 
                            sample.outliers_adc.append(adc_below)
                            sample.outliers_time.append(time)
                            sample.outliers_number += 1
                if gap != 0: 
                    if gap == 1:
                        print("Gap is too small - ignoring")
                    elif gap == 2:
                        if cluster == 2: # if the previous cluster was marked as an outlier, a disjointed cluster after it is also an outlier
                            for sample in dataset:
                                if adc_below == sample.adc[i]: 
                                    sample.outliers_adc.append(adc_below)
                                    sample.outliers_time.append(time)
                                    sample.outliers_number += 1
                        else:
                            for sample in dataset:
                                if adc_below == sample.adc[i]: 
                                    sample.potential_outliers_adc.append(adc_below)
                                    sample.potential_outliers_time.append(time)
                                    sample.potential_outliers_number += 1
                            cluster = 1
                    elif gap >= 3:
                        for sample in dataset:
                            if adc_below == sample.adc[i]: 
                                sample.outliers_adc.append(adc_below)
                                sample.outliers_time.append(time)
                                sample.outliers_number += 1
                        cluster = 2
                gap = 0
            else: 
                gap += 1
                print(f"ADC {adc_below} not in same cluster\n")
            adc_below -= 1

# -------  DATA ANALYSIS -------

def outlier_phase_analysis(dataset : list):

    outlier_times = []
    pot_outlier_times = []
    outlier_counts = []
    pot_outlier_counts = []
    outlier_phase_counts = []
    pot_outlier_phase_counts = []

    total_outlier_number = 0

    phases = {0: [], 1: [], 2: [], 3: [], 4: [], 5: [], 6: [], 7: [],
              8: [], 9: [], 10: [], 11: [], 12: [], 13: [], 14: [], 15: []}
    
    n = 0
    for time in dataset[0].time:
        phases[n].append(time)
        n += 1
        if n == 16: n = 0

    for sample in dataset:
        outlier_times.append(sample.outliers_time)
        pot_outlier_times.append(sample.potential_outliers_time)
        total_outlier_number += sample.outliers_number + sample.potential_outliers_number
    outlier_times = np.concatenate(outlier_times).tolist()
    pot_outlier_times = np.concatenate(pot_outlier_times).tolist()

    for phase in phases:
        for time in phases[phase]:
            if time in outlier_times:
                outlier_counts.append([phase, time, outlier_times.count(time)])
            else: outlier_counts.append([phase, time, 0])
            if time in pot_outlier_times:
                pot_outlier_counts.append([phase, time, pot_outlier_times.count(time)])
            else: pot_outlier_counts.append([phase, time, 0])
    outlier_counts = np.array(outlier_counts)
    pot_outlier_counts = np.array(pot_outlier_counts)
    
    for phase in phases:
        mask1 = outlier_counts[:,0] == phase
        mask2 = pot_outlier_counts[:,0] == phase
        phase_counts = outlier_counts[mask1][:,2].sum()
        pot_phase_counts = pot_outlier_counts[mask2][:,2].sum()
        outlier_phase_counts.append([phase, phase_counts])
        pot_outlier_phase_counts.append([phase, pot_phase_counts])
    outlier_phase_counts = np.array(outlier_phase_counts)
    pot_outlier_phase_counts = np.array(pot_outlier_phase_counts)

    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(12,8))

    ax1.bar(dataset[0].time, outlier_counts[:,2]/total_outlier_number, color='r', label = 'Outliers')
    ax1.bar(dataset[0].time, pot_outlier_counts[:,2]/total_outlier_number, color='b', alpha = 0.5, label = 'Potential outliers')
    ax2.bar(outlier_phase_counts[:,0], outlier_phase_counts[:,1]/total_outlier_number, color='r')
    ax2.bar(pot_outlier_phase_counts[:,0], pot_outlier_phase_counts[:,1]/total_outlier_number, color='b', alpha = 0.5)
    ax1.set(xlabel='Time [ns]', ylabel = 'Proportion of outliers')
    ax2.set(xlabel='Phase', ylabel = 'Proportion of outliers')
    ax1.legend()
    ax1.grid()
    ax2.grid()
    if args.plot_directory:
        plt.savefig(os.path.join(args.plot_directory,f'outlier_phase_analysis.png'), dpi=400)
        plt.close()
    else:
        plt.show()

def outlier_bulk_evaluation(full_dataset):
    
    outlier_data = []

    for dataset in full_dataset:
        samples = len(dataset)
        calib = dataset[0].calib
        channel = dataset[0].channel
        scan_type = dataset[0].scan_type
        if scan_type == "preCC" : scan_type = 0
        elif scan_type == "lowrange" : scan_type = 1
        else : scan_type = 2

        outlier_count = 0
        outlier_sum = 0
        potential_outlier_sum = 0
        for sample in dataset:
            if sample.outliers_number != 0: outlier_count += 1
            outlier_sum += sample.outliers_number
            potential_outlier_sum += sample.potential_outliers_number
        outlier_fraction = outlier_count/samples
        outlier_data.append(np.array((outlier_fraction, outlier_sum, potential_outlier_sum, calib, channel, samples, scan_type)))
    outlier_data = np.array(outlier_data)
    outlier_dict = {"outlier_fraction" : outlier_data[:,0], "outlier_sum" : outlier_data[:,1], "pot_outlier_sum" : outlier_data[:,2], "calib" : outlier_data[:,3], 
                    "channel" : outlier_data[:,4], "samples" : outlier_data[:,5], "scan_type" : outlier_data[:,6]}
    outlier_df = pd.DataFrame(outlier_dict)

    return outlier_df

# -------  PLOTTING DATA -------

def plot_outliers(dataset : list, plot_type : str):

    outs = []
    potouts = []

    for sample in dataset:
        outs.append((sample.outliers_time, sample.outliers_adc))
        potouts.append((sample.potential_outliers_time, sample.potential_outliers_adc))

    if plot_type == 'CLUSTER':
        plt.figure(figsize=(12, 8))
        for i in range(0,len(dataset)):
            plt.scatter(dataset[i].time, dataset[i].adc, c='b', alpha=0.5)
            plt.scatter(potouts[i][0], potouts[i][1], c='c')
            plt.scatter(outs[i][0], outs[i][1], c='r')
        plt.scatter(potouts[-1][0], potouts[-1][1], c='c', label='Potential outliers')
        plt.scatter(outs[-1][0], outs[-1][1], c='r', label='Outliers')
        plt.ylim(np.min(dataset[0].adc)-10, np.max(dataset[0].adc)+10)
        plt.xlim(np.min(dataset[0].time)-1, np.max(dataset[0].time)+1)
        plt.ylabel('ADC')
        plt.xlabel('time [ns]')
        plt.title(f"highrange = {dataset[i].parameters['highrange']}, preCC = {dataset[i].parameters['preCC']}, channel {dataset[i].channel}, CALIB {dataset[i].calib}; {dataset[i].parameters['samples']} samples")
        plt.legend()
        plt.grid()
        if args.plot_directory:
            plt.savefig(os.path.join(args.plot_directory,f'clustered_outliers.png'), dpi=400)
            plt.close()
        else:
            plt.show()

    else:
        for i in range(0,len(dataset)):
            if len(outs[i][1]) == 0 and len(potouts[i][1]) == 0: # skip non-outlier plots
                continue 
            plt.scatter(dataset[i].time, dataset[i].adc, c='b', alpha=0.5)
            plt.scatter(potouts[i][0], potouts[i][1], c='c', label='Potential outliers')
            plt.scatter(outs[i][0], outs[i][1], c='r', label='Outliers')
            plt.ylim(np.min(dataset[0].adc)-10, np.max(dataset[0].adc)+10)
            plt.xlim(np.min(dataset[0].time)-1, np.max(dataset[0].time)+1)
            plt.ylabel('ADC')
            plt.xlabel('time [ns]')
            plt.title(f"highrange = {dataset[i].parameters['highrange']}, preCC = {dataset[i].parameters['preCC']}, channel {dataset[i].channel}, CALIB {dataset[i].calib}")
            plt.legend()
            plt.grid()
            if args.plot_directory:
                plt.savefig(os.path.join(args.plot_directory,f'sample_{i}_outliers.png'), dpi=300)
                plt.close()
            else:
                plt.show()
                plt.close()

def plot_evaluation(dataframe):
    
    multi_type = False
    encoded_types = np.unique(dataframe.scan_type.to_numpy())
    types = []
    for Type in range(len(encoded_types)):
        if encoded_types[Type] == 0 : types.append('preCC')
        elif encoded_types[Type] == 1 : types.append('lowrange')
        else : types.append('highrange')

    if len(types) > 1: multi_type = True 

    calibs = np.unique(dataframe.calib.to_numpy(dtype=np.int64))

    if not multi_type:
        fig, (ax1,ax2) = plt.subplots(2,1, figsize=(10,8))

        for calib in calibs:
            ax1.bar(dataframe[(dataframe.calib==calib) & (dataframe.channel < 36)].channel.to_numpy(dtype=np.int64), 
                        dataframe[(dataframe.calib==calib) & (dataframe.channel < 36)].outlier_sum.to_numpy(dtype=np.int64), alpha=0.7, label=f"CALIB{calib}")
            ax2.bar(dataframe[(dataframe.calib==calib) & (dataframe.channel > 35)].channel.to_numpy(dtype=np.int64), 
                        dataframe[(dataframe.calib==calib) & (dataframe.channel > 35)].outlier_sum.to_numpy(dtype=np.int64), alpha=0.7, label=f"CALIB{calib}")
        fig.supxlabel('Channels', fontsize=15)
        fig.supylabel('Total outlier count', fontsize=15)
        ax1.set_xticks(np.arange(36, step=2))
        ax2.set_xticks(np.arange(36,72, step=2))
        ax1.grid()
        ax2.grid()
        ax1.legend(bbox_to_anchor=(1.1, 1), loc = 'upper right')
        ax2.legend(bbox_to_anchor=(1.1, 1), loc = 'upper right')
        ax1.set_title("Link 0")
        ax2.set_title("Link 1")
        fig.suptitle(f"Total number of outliers per channel - {types[0]}", fontsize=14)
        if args.plot_directory:
            plt.savefig(os.path.join(args.plot_directory,f'outlier_evaluation_sum.png'), dpi=400)
            plt.close()
        else: plt.show()
        
        fig, (ax1,ax2) = plt.subplots(2,1, figsize=(10,8))
        for calib in calibs:
            ax1.bar(dataframe[(dataframe.calib==calib) & (dataframe.channel < 36)].channel.to_numpy(dtype=np.int64), 
                    dataframe[(dataframe.calib==calib) & (dataframe.channel < 36)].outlier_fraction.to_numpy(dtype=np.float64), alpha=0.7, label=f"CALIB{calib}")
            ax2.bar(dataframe[(dataframe.calib==calib) & (dataframe.channel > 35)].channel.to_numpy(dtype=np.int64), 
                    dataframe[(dataframe.calib==calib) & (dataframe.channel > 35)].outlier_fraction.to_numpy(dtype=np.float64), alpha=0.7, label=f"CALIB{calib}")

        fig.supxlabel('Channels')
        fig.supylabel('Outlier-sample frequency')
        ax1.set_xticks(np.arange(36, step=2))
        ax2.set_xticks(np.arange(36,72, step=2))
        ax1.grid()
        ax2.grid()
        ax1.legend(bbox_to_anchor=(1.1, 1), loc = 'upper right')
        ax2.legend(bbox_to_anchor=(1.1, 1), loc = 'upper right')
        ax1.set_title("Link 0")
        ax2.set_title("Link 1")
        fig.suptitle(f"Fraction of samples with outliers per channel - {types[0]}", fontsize=14)
        if args.plot_directory:
            plt.savefig(os.path.join(args.plot_directory,f'outlier_evaluation_freq.png'), dpi=400)
            plt.close()
        else: plt.show()

        # potential outliers ------------------------------------------------

        fig, (ax1,ax2) = plt.subplots(2,1, figsize=(10,8))

        for calib in calibs:
            ax1.bar(dataframe[(dataframe.calib==calib) & (dataframe.channel < 36)].channel.to_numpy(dtype=np.int64), 
                        dataframe[(dataframe.calib==calib) & (dataframe.channel < 36)].pot_outlier_sum.to_numpy(dtype=np.int64), alpha=0.7, label=f"CALIB{calib}")
            ax2.bar(dataframe[(dataframe.calib==calib) & (dataframe.channel > 35)].channel.to_numpy(dtype=np.int64), 
                        dataframe[(dataframe.calib==calib) & (dataframe.channel > 35)].pot_outlier_sum.to_numpy(dtype=np.int64), alpha=0.7, label=f"CALIB{calib}")
        fig.supxlabel('Channels', fontsize=15)
        fig.supylabel('Total potential-outlier count', fontsize=15)
        ax1.set_xticks(np.arange(36, step=2))
        ax2.set_xticks(np.arange(36,72, step=2))
        ax1.grid()
        ax2.grid()
        ax1.legend(bbox_to_anchor=(1.1, 1), loc = 'upper right')
        ax2.legend(bbox_to_anchor=(1.1, 1), loc = 'upper right')
        ax1.set_title("Link 0")
        ax2.set_title("Link 1")
        fig.suptitle(f"Total number of potential outliers per channel - {types[0]}", fontsize=14)
        if args.plot_directory:
            plt.savefig(os.path.join(args.plot_directory,f'pot_outlier_evaluation_sum.png'), dpi=400)
            plt.close()
        else: plt.show()
        
        fig, (ax1,ax2) = plt.subplots(2,1, figsize=(10,8))
        for calib in calibs:
            ax1.bar(dataframe[(dataframe.calib==calib) & (dataframe.channel < 36)].channel.to_numpy(dtype=np.int64), 
                    dataframe[(dataframe.calib==calib) & (dataframe.channel < 36)].outlier_fraction.to_numpy(dtype=np.float64), alpha=0.7, label=f"CALIB{calib}")
            ax2.bar(dataframe[(dataframe.calib==calib) & (dataframe.channel > 35)].channel.to_numpy(dtype=np.int64), 
                    dataframe[(dataframe.calib==calib) & (dataframe.channel > 35)].outlier_fraction.to_numpy(dtype=np.float64), alpha=0.7, label=f"CALIB{calib}")

        fig.supxlabel('Channels')
        fig.supylabel('Potential-outlier-sample frequency')
        ax1.set_xticks(np.arange(36, step=2))
        ax2.set_xticks(np.arange(36,72, step=2))
        ax1.grid()
        ax2.grid()
        ax1.legend(bbox_to_anchor=(1.1, 1), loc = 'upper right')
        ax2.legend(bbox_to_anchor=(1.1, 1), loc = 'upper right')
        ax1.set_title("Link 0")
        ax2.set_title("Link 1")
        fig.suptitle(f"Fraction of samples with potential-outliers per channel - {types[0]}", fontsize=14)
        if args.plot_directory:
            plt.savefig(os.path.join(args.plot_directory,f'pot_outlier_evaluation_freq.png'), dpi=400)
            plt.close()
        else: plt.show()

    else:
        fig, (ax1,ax2) = plt.subplots(2,1,figsize=(18,7))
        for calib in calibs:
            ax1.scatter(dataframe[(dataframe.calib==calib) & (dataframe.scan_type==encoded_types[0])].channel.to_numpy(dtype=np.int64), 
                        dataframe[(dataframe.calib==calib) & (dataframe.scan_type==encoded_types[0])].outlier_sum.to_numpy(dtype=np.int64), alpha=0.7, label=f"CALIB{calib}, {types[0]}")
            ax2.scatter(dataframe[(dataframe.calib==calib) & (dataframe.scan_type==encoded_types[1])].channel.to_numpy(dtype=np.int64), 
                        dataframe[(dataframe.calib==calib) & (dataframe.scan_type==encoded_types[1])].outlier_sum.to_numpy(dtype=np.int64), alpha=0.7, label=f"CALIB{calib}, {types[1]}")
        ax1.set_xticks(np.arange(72))
        ax2.set_xticks(np.arange(72))
        ax1.set_xlabel("Channels")
        ax2.set_xlabel("Channels")
        ax1.set_ylabel("Outlier sum")
        ax2.set_ylabel("Outlier sum")
        ax1.grid()
        ax2.grid()
        ax1.legend()
        ax2.legend()
        
        ax1.set_title(f"{types[0]}")
        ax2.set_title(f"{types[1]}")
        fig.suptitle(f"Total number of outliers per channel, {types[0]} and {types[1]}")
        
        if args.plot_directory:
            plt.savefig(os.path.join(args.plot_directory,f'outlier_evaluation_sum.png'), dpi=400)
            plt.close()
        else: plt.show()

        fig2, (ax3,ax4) = plt.subplots(2,1,figsize=(12,8))
        for calib in calibs:
            ax3.bar(dataframe[(dataframe.calib==calib) & (dataframe.scan_type==encoded_types[0])].channel.to_numpy(dtype=np.int64), 
                        dataframe[(dataframe.calib==calib) & (dataframe.scan_type==encoded_types[0])].outlier_fraction.to_numpy(dtype=np.int64), alpha=0.7, label=f"CALIB{calib}, {types[0]}")
            ax4.bar(dataframe[(dataframe.calib==calib) & (dataframe.scan_type==encoded_types[1])].channel.to_numpy(dtype=np.int64), 
                        dataframe[(dataframe.calib==calib) & (dataframe.scan_type==encoded_types[1])].outlier_fraction.to_numpy(dtype=np.int64), alpha=0.7, label=f"CALIB{calib}, {types[1]}")
        ax3.set_xticks(np.arange(72))
        ax4.set_xticks(np.arange(72))
        ax3.set_xlabel("Channels")
        ax4.set_xlabel("Channels")
        ax3.set_ylabel("Outlier sum")
        ax4.set_ylabel("Outlier sum")
        ax3.grid()
        ax4.grid()
        ax3.legend()
        ax4.legend()
        
        ax3.set_title(f"{types[0]}")
        ax4.set_title(f"{types[1]}")
        fig.suptitle(f"Fraction of samples with outliers per channel, {types[0]} and {types[1]}")
        
        if args.plot_directory:
            plt.savefig(os.path.join(args.plot_directory,f'outlier_evaluation_sum.png'), dpi=400)
            plt.close()
        else: plt.show()

# -------  SAVING DATA -------

def write_to_csv(dataset : list, save_path : Path):

    header = ["Type", "Channel", "CALIB", "Potential_outliers", "PO_counts", "Outliers", "O_counts", "Missing_ADC_codes"]

    csv = []

    for sample in dataset:
        
        if sample.parameters["preCC"] : scan_type = "preCC"
        elif sample.parameters["highrange"] : scan_type = "HR"
        else : scan_type = "LR"
        if sample.potential_outliers_number != 0: potential_count = 1
        else: potential_count = 0

        if sample.outliers_number != 0 : count = 1
        else : count = 0

        csv.append([scan_type, sample.channel, sample.calib, potential_count, sample.potential_outliers_number, count, sample.outliers_number, sample.missing_adc])

    df = pd.DataFrame(csv, columns = header)
    df.to_csv(os.path.join(save_path,f'outlier-results-channel-{dataset[0].channel}-calib-{dataset[0].calib}-{scan_type}.csv'))

# ------- EXECUTION BLOCK -------
    
working_data = []
for file_path in args.dataset:
    raw_data, data_parameters = read_data(file_path) 
    processed_data = sort_data(raw_data, data_parameters)
    working_data.append(classify_data(processed_data, data_parameters))

if args.cluster_scan: 
    for data in working_data:
        cluster_outlier_search(data)

if args.threshold_scan:
    for data in working_data:
        th_value = int(input("What delta-ADC threshold do you want to choose for the scan?\n" \
                            f"Default options for HR data: {HR_thresholds}\n" \
                            "Threshold: "))
        threshold_outlier_search(data, th_value)

if args.phase_analysis:
    for data in working_data:
        outlier_phase_analysis(data)

if args.bulk_evaluation:
    outlier_dataframe = outlier_bulk_evaluation(working_data)

if args.csv:
    for data in working_data:
        write_to_csv(data, args.csv)

if args.plot:
    if args.plot == "EVALUATION":
        plot_evaluation(outlier_dataframe)
    else:
        for data in working_data:
            plot_outliers(data, args.plot)
    