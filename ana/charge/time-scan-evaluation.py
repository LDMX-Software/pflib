import numpy as np
import matplotlib.pyplot as plt
import pandas as pd
import json
import os

from pathlib import Path
import argparse

# ------- PARSER -------

plot_types = ['HEATMAP', 'SCATTER']

parser = argparse.ArgumentParser()
parser.add_argument('datasets', type=Path, nargs="+", help='Parameter timescan of one channel, one CALIB, n samples per time-point')
parser.add_argument('-p', '--plot', choices=plot_types, type=str, help=f'Plot results. Available types: {", ".join(plot_types)}')
parser.add_argument('-pd', '--plot_directory', type=Path, help='Figures directory path. If not provided, figures are not saved automatically')
parser.add_argument('-csv', '--csv', type=Path, help='Save the scan results to a csv with the given path')
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

    for time_point in time_points:
        avg_adc.append(np.mean(data[data.time == time_point].adc))
    avg_peak = round(max(avg_adc))

    return np.array((avg_peak, run_params["calib"], run_params["channel"], datatype))


def scatter_plot_evaluation(data, multi_type):
    
    calibs = np.unique(data.calib.to_numpy(dtype=np.int64))
    print(calibs)

    if not multi_type:
        for calib in calibs:
            plt.scatter(data[data.calib==str(calib)].channel.to_numpy(dtype=np.int64), data[data.calib==str(calib)].peak.to_numpy(dtype=np.int64), alpha = 0.5, label=f"calib{calib}, {data[data.calib==str(calib)].scan_type.to_numpy()[0]}")
        plt.axvline(35, label="Link border", linestyle='--', c='r')
        plt.xticks(np.arange(72))
        plt.xlabel("Channels")
        plt.ylabel("Average peak ADC [a.u.]")
        plt.grid()
        plt.legend()
        plt.show()
    
    else:
        types = np.unique(data.scan_type.to_numpy())
        
        if len(types) == 2:

            fig, (ax1,ax2) = plt.subplots(2,1,figsize=(12,8))

            for calib in calibs:
                ax1.scatter(data[(data.calib==str(calib)) & (data.scan_type==types[0])].channel.to_numpy(dtype=np.int64), data[(data.calib==str(calib)) & (data.scan_type==types[0])].peak.to_numpy(dtype=np.int64), label=f"calib{calib}, {types[0]}")
                ax2.scatter(data[(data.calib==str(calib)) & (data.scan_type==types[1])].channel.to_numpy(dtype=np.int64), data[(data.calib==str(calib)) & (data.scan_type==types[1])].peak.to_numpy(dtype=np.int64), label=f"calib{calib}, {types[1]}")
            
            ax1.grid()
            ax2.grid()
            ax1.legend()
            ax2.legend()
            plt.show()

        if len(types) == 3:

            fig, (ax1,ax2,ax3) = plt.subplots(3,1,figsize=(12,8))

            for calib in calibs:
                ax1.scatter(data[(data.calib==str(calib)) & (data.scan_type==types[0])].channel.to_numpy(dtype=np.int64), data[(data.calib==str(calib)) & (data.scan_type==types[0])].peak.to_numpy(dtype=np.int64), label=f"calib{calib}, {types[0]}")
                ax2.scatter(data[(data.calib==str(calib)) & (data.scan_type==types[1])].channel.to_numpy(dtype=np.int64), data[(data.calib==str(calib)) & (data.scan_type==types[1])].peak.to_numpy(dtype=np.int64), label=f"calib{calib}, {types[1]}")
                ax3.scatter(data[(data.calib==str(calib)) & (data.scan_type==types[2])].channel.to_numpy(dtype=np.int64), data[(data.calib==str(calib)) & (data.scan_type==types[2])].peak.to_numpy(dtype=np.int64), label=f"calib{calib}, {types[2]}")

            ax1.grid()
            ax2.grid()
            ax1.legend()
            ax2.legend()
            plt.show()

        else: print("Too many scan types! Maximum: 3")

data = []
for dataset in args.datasets:
    data.append(read_data(dataset))
data = np.array(data)

data_sorted = {"peak" : data[:,0], "calib" : data[:,1], "channel" : data[:,2], "scan_type" : data[:,3]}
if len(np.unique(data_sorted["scan_type"])) > 1: multi_type = True
else: multi_type = False
df = pd.DataFrame(data_sorted)


scatter_plot_evaluation(df, multi_type)



