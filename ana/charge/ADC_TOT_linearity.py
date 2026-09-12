import numpy as np
import matplotlib.pyplot as plt
import pandas as pd
from pathlib import Path
import argparse
import matplotlib.cm as cm
import scipy
import matplotlib.colors as mcolors

plot_options = ["ADC-TOT", "TOT-HEATMAP", "TOT-TIME", "ALL"]

parser = argparse.ArgumentParser()
parser.add_argument('datasets', type=Path, nargs='+', help='Global calib scan file paths of desired board.')
parser.add_argument('-sd', '--save_directory', type=Path, help='Path to directory, in which the results will be saved. Default is directory before the source path.')
parser.add_argument('-fl', '--fit_line', action='store_true', help='Fit a linear function to the ADC and TOT and save the results.')
parser.add_argument('-flt', '--fit_line_threshold', type=int, default=512, help='The TOT linear fit will only include values above this threshold.')
parser.add_argument('-p', '--plot', choices = plot_options, help='Plot options.')
args = parser.parse_args()

# Folder where the CSV lives
data_dir = args.datasets[0].parent
# Create output folder
if args.save_directory:
    output_dir = args.save_directory
else:
    output_dir = data_dir/ "adc_tot_linearity_figures"
output_dir.mkdir(parents=True, exist_ok=True)

dfs = []
for f in args.datasets:
    df_tmp = pd.read_csv(f)
    dfs.append(df_tmp)

df = pd.concat(dfs, ignore_index=True)

if args.fit_line: fit_data = {'Channel' : [], 'ADC_R2' : [], 'TOT_R2' : []}

for ch, df_ch in df.groupby("channel"):   

    df_valid = df_ch.copy()
    df_valid.loc[df_valid["tot"] <= 0, "tot"] = np.nan # remove invalid TOT with value -1 for median calculation

    df_med = df_valid.groupby(["calib", "time"]).agg({"adc": "median", "tot": "median"}).reset_index() # take median over same calib and timepoint
    df_med2 = df_med.groupby("calib").agg({"adc": "median", "tot": "median"}).reset_index() #take median over the above medians
    df_max = df_med.groupby("calib").agg({"adc": "max", "tot": "max"}).reset_index() #take maximum value

    df_adc = df_max.copy()
    df_adc.loc[df_adc["tot"].notna(), "adc"] = np.nan #make ADC be NaN where TOT is active (not NaN)
    adc_mask = (df_adc["adc"] < 1023) & df_adc["adc"].notna()

    if args.fit_line:

        adc_fit_results = scipy.stats.linregress(df_adc["calib"][adc_mask], df_adc["adc"][adc_mask])
        adc_fit = df_adc["calib"][adc_mask]*adc_fit_results.slope + adc_fit_results.intercept

        tot_fit_results = scipy.stats.linregress(df_med2["calib"][df_med2["tot"] > args.fit_line_threshold], df_med2["tot"][df_med2["tot"] > args.fit_line_threshold])
        tot_fit = df_med2["calib"][df_med2["tot"] > args.fit_line_threshold]*tot_fit_results.slope + tot_fit_results.intercept

        fit_data['ADC_R2'].append(adc_fit_results.rvalue**2)
        fit_data['TOT_R2'].append(tot_fit_results.rvalue**2)
        fit_data['Channel'].append(ch)

    if (args.plot == "ADC-TOT") or (args.plot == "ALL"):

        fig, ax1 = plt.subplots()
        # ADC region 
        if args.fit_line: 
            ax1.plot(df_adc["calib"][adc_mask], adc_fit, color="red", alpha = 0.5, linestyle='--', 
                                   label=f'ADC fit: y = {adc_fit_results.slope:.3g}x + {adc_fit_results.intercept:.3g}')
            ax1.legend(bbox_to_anchor=(1.0, 0.1), loc = 'lower right')
        ax1.scatter(df_adc["calib"], df_adc["adc"], color="red", s=2)
        ax1.set_xlabel("CALIB")
        ax1.set_ylabel("Max ADC [a.u.]", color="red")
        ax1.set_axisbelow(True)
        ax1.grid(True)

        # TOT region 
        ax2 = ax1.twinx() #creates second y-axis on the right side
        if args.fit_line: 
            ax2.plot(df_med2["calib"][df_med2["tot"] > args.fit_line_threshold], tot_fit, color="blue", alpha = 0.5, linestyle='--',
                                   label=f'TOT fit: y = {tot_fit_results.slope:.3g}x + {tot_fit_results.intercept:.3g}')
            ax2.legend(loc = 'lower right')
        ax2.scatter(df_med2["calib"], df_med2["tot"], color="blue", s=1)
        ax2.set_ylabel("Median TOT [a.u.]", color="blue")
        
        plt.title(f"Channel {ch}")
        plt.savefig(output_dir / f"channel_{ch}.png", dpi=300)
        plt.close()

    if (args.plot == "TOT-HEATMAP") or (args.plot == "ALL"):    

        #plot of raw TOT values, colormapped to the respective calib
        fig, ax = plt.subplots()
        df_tot = df_ch[df_ch["tot"] > 0] #raw TOT values

        norm = mcolors.Normalize(vmin=df_tot["calib"].min(), vmax=df_tot["calib"].max())
        cmap = plt.get_cmap("plasma")

        sc = ax.scatter(df_tot["calib"], df_tot["tot"], c=df_tot["calib"], cmap=cmap, norm=norm, s=0.1)

        cbar = plt.colorbar(sc, ax=ax)
        cbar.set_label("CALIB")
        ax.set_xlabel("CALIB")
        ax.set_ylabel("TOT [a.u.]")
        plt.title(f"Channel {ch}, TOT distribution")
        ax.set_axisbelow(True)
        ax.grid(True)
        plt.savefig(output_dir / f"channel_{ch}_TOT_raw.png", dpi=300)
        plt.close()

    if (args.plot == "TOT-TIME") or (args.plot == "ALL"):

        # TOT vs time as well, but calibs as lines
        fig, ax = plt.subplots()

        # Normalize calib range to [0, 1]
        norm = mcolors.Normalize(vmin=df_med["calib"].min(), vmax=df_med["calib"].max())
        cmap = plt.get_cmap("plasma")  # good default

        for calib, df_c in df_med.groupby("calib"):
            df_c = df_c[df_c["tot"].notna()]
            if df_c.empty:
                continue
            df_c = df_c.sort_values("time")
            ax.plot(df_c["time"], df_c["tot"], color=cmap(norm(calib)), linewidth=1, alpha=0.8)

        sm = cm.ScalarMappable(norm=norm, cmap=cmap) #for colorbar
        sm.set_array([])
        cbar = plt.colorbar(sm, ax=ax)
        cbar.set_label("CALIB")

        ax.set_xlabel("time")
        ax.set_ylabel("TOT [a.u.]")
        ax.set_title(f"Channel {ch} – TOT vs time")
        ax.set_axisbelow(True)
        ax.grid(True)
        plt.savefig(output_dir / f"channel_{ch}_TOT_vs_time_lines.png", dpi=300)
        plt.close()

if args.fit_line:
    results_df = pd.DataFrame(fit_data)
    results_df.to_csv(output_dir / "fit_results.csv")
