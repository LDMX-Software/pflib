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

    
    fig, (ax1,ax2) = plt.subplots(2,1,figsize=(12,10))

    for calib in calibs:

        link0_peaks = data[(data.calib==calib) & (data.link == 0)].peak.to_numpy(dtype=np.float64)
        link1_peaks = data[(data.calib==calib) & (data.link == 1)].peak.to_numpy(dtype=np.float64)
        ax1.scatter(np.arange(36), link0_peaks, alpha = 0.5, label=f"CALIB{calib}")
        ax2.scatter(np.arange(36,72), link1_peaks, alpha = 0.5, label=f"CALIB{calib}")
    ax1.set_xticks(np.arange(36))
    ax1.set_title("Link 0")
    ax1.set_xlabel("Channels")
    ax1.set_ylabel("Average peak ADC [a.u.]")
    ax1.grid()
    ax1.legend(bbox_to_anchor=(1.1, 1), loc = 'upper right')

    ax2.set_xticks(np.arange(36,72))
    ax2.set_title("Link 1")
    ax2.set_xlabel("Channels")
    ax2.set_ylabel("Average peak ADC [a.u.]")
    ax2.grid()
    ax2.legend(bbox_to_anchor=(1.1, 1), loc = 'upper right')
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
        ax1.plot(np.arange(36), link0_rms, alpha = 0.5, label=f"CALIB{calib}")
        ax2.plot(np.arange(36,72), link1_rms, alpha = 0.5, label=f"CALIB{calib}")
    ax1.set_xticks(np.arange(36))
    ax1.set_title("Link 0")
    ax1.set_xlabel("Channels")
    ax1.set_ylabel("RMS of average peak ADC [a.u.]")
    ax1.grid()
    ax1.legend(bbox_to_anchor=(1.1, 1), loc = 'upper right')

    ax2.set_xticks(np.arange(36,72))
    ax2.set_title("Link 1")
    ax2.set_xlabel("Channels")
    ax2.set_ylabel("RMS of average peak ADC [a.u.]")
    ax2.grid()
    ax2.legend(bbox_to_anchor=(1.1, 1), loc = 'upper right')

    fig.suptitle(f"Mean-ADC-peak-RMS over channels - {data_type}", fontsize=14)

    if args.plot_directory:
        plt.savefig(os.path.join(args.plot_directory,f'time-scan-evaluation-rms-{data_type}.png'), dpi=400)
        plt.close()
    else:
        plt.show()
        plt.close()

def gradient_evaluation(data):
    
    gradient = False # I think the gradient/R2 evaluation doesn't really make much sense, it's probably better to take the RMS of the mean-peak distribution instead

    if gradient:

        results = []
        calibs = np.unique(data.calib.to_numpy(dtype=np.int64))

        results = {"CALIB" : [], "link_0_gradient" : [], "link_0_R2" : [], "link_1_gradient" : [], "link_1_R2" : []}

        for calib in calibs:
            link0_peaks = data[(data.calib==calib) & (data.link == 0)].peak.to_numpy(dtype=np.float64)
            link1_peaks = data[(data.calib==calib) & (data.link == 1)].peak.to_numpy(dtype=np.float64)
            
            gradient0, intercept0, r_value0, p_value0, std_err0 = stats.linregress(np.arange(36), link0_peaks)
            gradient1, intercept1, r_value1, p_value1, std_err1 = stats.linregress(np.arange(36,72), link1_peaks)
            
            results["CALIB"].append(calib)
            results["link_0_gradient"].append(gradient0)
            results["link_1_gradient"].append(gradient1)
            results["link_0_R2"].append(r_value0**2)
            results["link_1_R2"].append(r_value1**2)
        
        if args.csv : 
            df = pd.DataFrame(results)
            df.to_csv(args.csv)
        else:
            print(results)

    else:
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

if data[0,4] == 0 : scan_type = "preCC"
if data[0,4] == 1 : scan_type = "lowrange"
else : scan_type = "highrange"

df = pd.DataFrame(data_sorted)

if args.plot : scatter_plot_evaluation(df, scan_type)
if args.goodness_of_fit : 
    gradient_evaluation(df)
