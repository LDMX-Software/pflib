import numpy as np
import matplotlib.pyplot as plt
import pandas as pd
import json
import scipy.stats as stats
import os

from pathlib import Path
import argparse

# ------- PARSER -------

'''
This block contains all the commands that will be available in-terminal. 
I'm not exactly sure how many functions there will be, so it's WIP.
'''

'''
Should have:

- nr of samples
- only one channel allowed (at least per file?)
- only one calib allowed (also per file?)
- custom threshold if threshold-based search -> also show the default thresholds
'''

HR_thresholds = {"CALIB_0":10, "CALIB_32":24, "CALIB_64":42, "CALIB_256":100, 
                     "CALIB_512":200, "CALIB_1024":200, "CALIB_2048":200, "CALIB_4096":200}
plot_types = ['CLUSTER', 'SINGULAR']

parser = argparse.ArgumentParser()
parser.add_argument('dataset', type=Path, nargs='+', help='Parameter timescan of one channel, one CALIB, n samples per time-point')
parser.add_argument('-s', '--samples', type=int, help='Number of samples per phase')
parser.add_argument('-cs', '--cluster_scan', action='store_true', help='Perform an outliers cluster-scan (default)')
parser.add_argument('-wh', '--wrong_header', action='store_true', help='Lund board-testing specific - some HR data have the wrong header')
parser.add_argument('-ts', '--threshold_scan', action='store_true', help='Perform a threshold outliers-scan with default thresholds (only HR works)')
parser.add_argument('-ph', '--phase_analysis', action='store_true', help='Perform and plot phase analysis of the outliers (include -pd for plot directory path)')
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

        self.outliers_time = []
        self.outliers_adc = []
        self.outliers_number = 0

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

    run_params["samples"] = args.samples

    return data, run_params

def sort_data(raw_data, raw_data_parameters):
    
    sorted_data = []
    raw_adc = raw_data["adc"].to_numpy()
    raw_time = raw_data["time"].to_numpy()
    raw_channels = raw_data["channel"].to_numpy()
    link = round(raw_channels[0]/72)
    
    if raw_data_parameters["preCC"]: raw_calibs = raw_data[f"REFERENCEVOLTAGE_{link}.CALIB_2V5"]
    else: 
        if args.wrong_header: raw_calibs = raw_data[f"REFERENCEVOLTAGE_{link}.CALIB_2V5"]
        else: raw_calibs = raw_data[f"REFERENCEVOLTAGE_{link}.CALIB"]
        

    scan_length = len(np.unique(raw_time))
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

    if args.samples < 10: 
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
                    elif gap == 2:
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
                    elif gap >= 3:
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
    if args.plot_directory:
        plt.savefig(os.path.join(args.plot_directory,f'outlier_phase_analysis.png'), dpi=400)
        plt.close()
    else:
        plt.show()

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
            if args.plot_directory:
                plt.savefig(os.path.join(args.plot_directory,f'sample_{i}_outliers.png'), dpi=300)
                plt.close()
            else:
                plt.show()
                plt.close()

# -------  SAVING DATA -------

def write_to_csv(dataset : list, save_path : Path):

    header = ["Type", "Channel", "CALIB", "Potential outliers", "Counts", "Outliers", "Counts"]

    csv = []

    for sample in dataset:
        
        if sample.parameters["preCC"] : scan_type = "preCC"
        elif sample.parameters["highrange"] : scan_type = "HR"
        else : scan_type = "LR"
        if sample.potential_outliers_number != 0: potential_count = 1
        else: potential_count = 0

        if sample.outliers_number != 0 : count = 1
        else : count = 0

        csv.append([scan_type, sample.channel, sample.calib, potential_count, sample.potential_outliers_number, count, sample.outliers_number])

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
    th_value = int(input("What delta-ADC threshold do you want to choose for the scan?\n" \
                        f"Default options for HR data: {HR_thresholds}\n" \
                        "Threshold: "))
    threshold_outlier_search(working_data, th_value)

if args.phase_analysis:
    outlier_phase_analysis(working_data)

if args.csv:
    for data in working_data:
        write_to_csv(data, args.csv)

if args.plot:
    for data in working_data:
        plot_outliers(data, args.plot)