import numpy as np
import matplotlib.pyplot as plt
import pandas as pd
import json
import os
from pathlib import Path
import argparse

# ------- PARSER -------

parser = argparse.ArgumentParser()
parser.add_argument('datasets', type=Path, nargs="+", help='Parameter timescan of one channel, one CALIB, n samples per time-point')
parser.add_argument('-p', '--plot', action='store_true', help=f'Plot results')
parser.add_argument('-rms', '--rms_calculation', action='store_true', help='Calculate the RMS of the mean-ADC-peak distribution across each link')
parser.add_argument('-pd', '--plot_directory', type=Path, help='Figures directory path. If not provided, figures are not saved automatically')
parser.add_argument('-csv', '--csv', type=Path, help='Save the fit results to a csv with the given path')
args = parser.parse_args()

# ------- LOADING & SORTING DATA -------

def root_mean_square(adc):
        mean_adc = np.mean(adc)
        sum = 0
        for sample in adc:
            sum += (sample-mean_adc)**2
        return np.sqrt(sum/len(adc))

def read_data(path: Path):
    
    '''To allow for sorting later in the script, the scan modes (or datatypes), that is preCC, lowrange and highrange, are initially assigned integers 0, 1, 2, respectively.'''

    with open(path) as file:
        header = file.readline().strip('\n')
    has_pflib_header = header[0] == '#'
    run_params = (
        json.loads(header.strip('#'))
        if has_pflib_header else
        {}
    )
    if has_pflib_header:
        data = pd.read_csv(path, skiprows=1, dtype=np.float64)

    channels = data["channel"].to_numpy()
    link = round(channels[0]/71)

    run_params["channel"] = int(channels[0])
    
    if run_params["preCC"] : 
        run_params["calib"] = int(data[f"REFERENCEVOLTAGE_{link}.CALIB_2V5"][0])
        datatype = 0
    else: 
        run_params["calib"] = int(data[f"REFERENCEVOLTAGE_{link}.CALIB"][0])
        if run_params["highrange"] : datatype = 2
        else: datatype = 1

    time_points = np.unique(data["time"].to_numpy())
    avg_adc = []
    rms = []

    for time_point in time_points:
        avg_adc.append(np.mean(data[data.time == time_point].adc))
        rms.append(root_mean_square(data[data.time == time_point].adc.to_numpy()))
    avg_peak = (max(avg_adc))
    avg_peak_index = avg_adc.index(avg_peak)
    rms_peak = rms[avg_peak_index]

    return np.array((avg_peak, run_params["calib"], run_params["channel"], link, datatype, rms_peak))

def scatter_plot_evaluation(data, data_type):
    
    calibs = np.unique(data.calib.to_numpy(dtype=np.int64))

    for calib in calibs:
        index = np.where(calibs==calib)
        if (calib >= 512) and (max(data[data.calib==calib].rms.to_numpy(dtype=np.float64)) == 0.0) : calibs = np.delete(calibs, index) 
    
    fig, (ax1,ax2) = plt.subplots(2,1,figsize=(12,10))

    for calib in calibs:

        link0_peaks = data[(data.calib==calib) & (data.link == 0)].peak.to_numpy(dtype=np.float64)
        link1_peaks = data[(data.calib==calib) & (data.link == 1)].peak.to_numpy(dtype=np.float64)
        link0_channels = data[(data.calib==calib) & (data.link == 0)].channel.to_numpy(dtype=np.int64)
        link1_channels = data[(data.calib==calib) & (data.link == 1)].channel.to_numpy(dtype=np.int64)
        ax1.scatter(link0_channels, link0_peaks, alpha = 0.6, label=f"CALIB{calib}")
        ax2.scatter(link1_channels, link1_peaks, alpha = 0.6, label=f"CALIB{calib}")
    ax1.set_xticks(link0_channels)
    ax1.set_title("Link 0")
    ax1.grid()
    ax1.legend(bbox_to_anchor=(1.1, 1), loc = 'upper right')

    ax2.set_xticks(link1_channels)
    ax2.set_title("Link 1")
    ax2.grid()
    ax2.legend(bbox_to_anchor=(1.1, 1), loc = 'upper right')

    fig.supylabel("Average peak ADC [a.u.]", fontsize=14)
    fig.supxlabel("Channels", fontsize=14)
    fig.suptitle(f"Mean-ADC-peak over channels - {data_type}", fontsize=14)

    if args.plot_directory:
        plt.savefig(os.path.join(args.plot_directory,f'time-scan-evaluation-{data_type}.png'), dpi=400)
        plt.close()
    else:
        plt.show()
        plt.close()

    fig, (ax1,ax2) = plt.subplots(2,1,figsize=(12,10))

    for calib in calibs:

        link0_rms = data[(data.calib==calib) & (data.link == 0)].rms.to_numpy(dtype=np.float64)
        link1_rms = data[(data.calib==calib) & (data.link == 1)].rms.to_numpy(dtype=np.float64)
        ax1.plot(link0_channels, link0_rms, alpha = 0.6, label=f"CALIB{calib}")
        ax2.plot(link1_channels, link1_rms, alpha = 0.6, label=f"CALIB{calib}")
    ax1.set_xticks(link0_channels)
    ax1.set_title("Link 0")
    ax1.grid()
    ax1.legend(bbox_to_anchor=(1.1, 1), loc = 'upper right')

    ax2.set_xticks(link1_channels)
    ax2.set_title("Link 1")
    ax2.grid()
    ax2.legend(bbox_to_anchor=(1.1, 1), loc = 'upper right')

    fig.supylabel("RMS of average peak ADC [a.u.]", fontsize=14)
    fig.supxlabel("Channels", fontsize=14)
    fig.suptitle(f"Mean-ADC-peak-RMS over channels - {data_type}", fontsize=14)

    if args.plot_directory:
        plt.savefig(os.path.join(args.plot_directory,f'time-scan-evaluation-rms-{data_type}.png'), dpi=400)
        plt.close()
    else:
        plt.show()
        plt.close()

def rms_calculation(data):

    results = []
    calibs = np.unique(data.calib.to_numpy(dtype=np.int64))

    results = {"CALIB" : [], "link_0_RMS" : [], "link_1_RMS" : []}

    for calib in calibs:
        link0_peaks = data[(data.calib==calib) & (data.link == 0)].peak.to_numpy(dtype=np.float64)
        link1_peaks = data[(data.calib==calib) & (data.link == 1)].peak.to_numpy(dtype=np.float64)

        link0_rms = root_mean_square(link0_peaks)
        link1_rms = root_mean_square(link1_peaks)

        results["CALIB"].append(calib)
        results["link_0_RMS"].append(link0_rms)
        results["link_1_RMS"].append(link1_rms)

    if args.csv : 
        df = pd.DataFrame(results)
        df.to_csv(args.csv)
    else:
        print(results)

data = []
for dataset in args.datasets:
    data.append(read_data(dataset))
data = np.array(data)
data = data[data[:,2].argsort()]

data_sorted = {"peak" : data[:,0], "calib" : data[:,1], "channel" : data[:,2], "link" : data[:,3], "rms" : data[:,5]}

if int(data[0,4]) == 0 : scan_type = "preCC"
elif int(data[0,4]) == 1 : scan_type = "lowrange"
else : scan_type = "highrange"

df = pd.DataFrame(data_sorted)

if args.plot : scatter_plot_evaluation(df, scan_type)
if args.rms_calculation : rms_calculation(df)
