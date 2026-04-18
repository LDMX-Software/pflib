import numpy as np
import matplotlib.pyplot as plt
import pandas as pd
import json
import os
from scipy import stats

from pathlib import Path
import argparse

# ------- PARSER -------

parser = argparse.ArgumentParser()
parser.add_argument('datasets', type=Path, nargs="+", help='Parameter timescan of one channel, one CALIB, n samples per time-point')
parser.add_argument('-p', '--plot', action='store_true', help=f'Plot results')
parser.add_argument('-g', '--goodness_of_fit', action='store_true', help='Perform a goodness-of-fit evaluation for a linear fit')
parser.add_argument('-pd', '--plot_directory', type=Path, help='Figures directory path. If not provided, figures are not saved automatically')
parser.add_argument('-csv', '--csv', type=Path, help='Save the fit results to a csv with the given path')
args = parser.parse_args()

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
        data = pd.read_csv(path, skiprows=1, dtype=np.float64)

    channels = data["channel"].to_numpy()
    link = round(channels[0]/71)

    run_params["channel"] = int(channels[0])
    
    if run_params["preCC"] : 
        run_params["calib"] = int(data[f"REFERENCEVOLTAGE_{link}.CALIB_2V5"][0])
        datatype = "preCC"
    else: 
        run_params["calib"] = int(data[f"REFERENCEVOLTAGE_{link}.CALIB"][0])
        if run_params["highrange"] : datatype = "highrange"
        else: datatype = "lowrange"

    time_points = np.unique(data["time"].to_numpy())
    avg_adc = []
    std_dev = []

    for time_point in time_points:
        avg_adc.append(np.mean(data[data.time == time_point].adc))
    avg_peak = (max(avg_adc))

    return np.array((avg_peak, run_params["calib"], run_params["channel"], link, datatype))


def scatter_plot_evaluation(data, multi_type):
    
    calibs = np.unique(data.calib.to_numpy(dtype=np.int64))
    if not multi_type:
        fig, (ax1,ax2) = plt.subplots(2,1,figsize=(12,8))

        for calib in calibs:

            link0_peaks = data[(data.calib==str(calib)) & (data.link == str(0))].peak.to_numpy(dtype=np.float64)
            link1_peaks = data[(data.calib==str(calib)) & (data.link == str(1))].peak.to_numpy(dtype=np.float64)
            ax1.scatter(np.arange(36), link0_peaks, alpha = 0.5, label=f"calib{calib}, {data[data.calib==str(calib)].scan_type.to_numpy()[0]}")
            ax2.scatter(np.arange(36,72), link1_peaks, alpha = 0.5, label=f"calib{calib}, {data[data.calib==str(calib)].scan_type.to_numpy()[0]}")
        ax1.set_xticks(np.arange(36))
        ax1.set_title("Link 0")
        ax1.set_xlabel("Channels")
        ax1.set_ylabel("Average peak ADC [a.u.]")
        ax1.grid()
        ax1.legend()

        ax2.set_xticks(np.arange(36,72))
        ax2.set_title("Link 1")
        ax2.set_xlabel("Channels")
        ax2.set_ylabel("Average peak ADC [a.u.]")
        ax2.grid()
        ax2.legend()
        plt.show()
    
    else:
        types = np.unique(data.scan_type.to_numpy())
        
        if len(types) == 2:

            fig, (ax1,ax2) = plt.subplots(2,1,figsize=(12,8))

            for calib in calibs:
                ax1.scatter(data[(data.calib==str(calib)) & (data.scan_type==types[0])].channel.to_numpy(dtype=np.int64), data[(data.calib==str(calib)) & (data.scan_type==types[0])].peak.to_numpy(dtype=np.float64), label=f"calib{calib}, {types[0]}")
                ax2.scatter(data[(data.calib==str(calib)) & (data.scan_type==types[1])].channel.to_numpy(dtype=np.int64), data[(data.calib==str(calib)) & (data.scan_type==types[1])].peak.to_numpy(dtype=np.float64), label=f"calib{calib}, {types[1]}")
            
            ax1.grid()
            ax2.grid()
            ax1.legend()
            ax2.legend()
            plt.show()

        if len(types) == 3:

            fig, (ax1,ax2,ax3) = plt.subplots(3,1,figsize=(12,8))

            for calib in calibs:
                ax1.scatter(data[(data.calib==str(calib)) & (data.scan_type==types[0])].channel.to_numpy(dtype=np.int64), data[(data.calib==str(calib)) & (data.scan_type==types[0])].peak.to_numpy(dtype=np.float64), label=f"calib{calib}, {types[0]}")
                ax2.scatter(data[(data.calib==str(calib)) & (data.scan_type==types[1])].channel.to_numpy(dtype=np.int64), data[(data.calib==str(calib)) & (data.scan_type==types[1])].peak.to_numpy(dtype=np.float64), label=f"calib{calib}, {types[1]}")
                ax3.scatter(data[(data.calib==str(calib)) & (data.scan_type==types[2])].channel.to_numpy(dtype=np.int64), data[(data.calib==str(calib)) & (data.scan_type==types[2])].peak.to_numpy(dtype=np.float64), label=f"calib{calib}, {types[2]}")

            ax1.grid()
            ax2.grid()
            ax1.legend()
            ax2.legend()
            plt.show()

        else: print("Too many scan types! Maximum: 3")

def gradient_evaluation(data):
    
    results = []
    calibs = np.unique(data.calib.to_numpy(dtype=np.int64))
    types = np.unique(data.scan_type.to_numpy())

    for Type in types:
        for calib in calibs:
            link0_peaks = data[(data.calib==str(calib)) & (data.link == str(0)) & (data.scan_type == Type)].peak.to_numpy(dtype=np.float64)
            link1_peaks = data[(data.calib==str(calib)) & (data.link == str(1)) & (data.scan_type == Type)].peak.to_numpy(dtype=np.float64)
            
            gradient0, intercept0, r_value0, p_value0, std_err0 = stats.linregress(np.arange(36), link0_peaks)
            gradient1, intercept1, r_value1, p_value1, std_err1 = stats.linregress(np.arange(36,72), link1_peaks)
            
            goodness_of_fit = np.array(([[gradient0, p_value0], [gradient1, p_value1]]))
            results.append([goodness_of_fit, calib, Type])
    
    return results

data = []
for dataset in args.datasets:
    data.append(read_data(dataset))
data = np.array(data)

data_sorted = {"peak" : data[:,0], "calib" : data[:,1], "channel" : data[:,2], "link" : data[:,3], "scan_type" : data[:,4]}
if len(np.unique(data_sorted["scan_type"])) > 1: multi_type = True
else: multi_type = False
df = pd.DataFrame(data_sorted)

if args.plot : scatter_plot_evaluation(df, multi_type)
if args.goodness_of_fit : 
    fit = gradient_evaluation(df)
    if args.csv : print("Save to csv - WIP")
    else: print(fit)
