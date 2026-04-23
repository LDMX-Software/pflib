import numpy as np
import matplotlib.pyplot as plt
import pandas as pd
from pathlib import Path
import argparse
import matplotlib.cm as cm
import matplotlib.colors as mcolors

parser = argparse.ArgumentParser()
parser.add_argument('datasets', type=Path, nargs='+', help='Global calib scan file paths of desired board.')
args = parser.parse_args()

# Folder where the CSV lives
data_dir = args.datasets[0].parent
# Create output folder
output_dir = data_dir.parent / "adc_tot_linearity_figures"
output_dir.mkdir(exist_ok=True)

#dfs = [pd.read_csv(f) for f in args.datasets]
dfs = []
for i, f in enumerate(args.datasets):
    df_tmp = pd.read_csv(f)
    
    if len(args.datasets) == 3 and i == 2:
        overlap_calibs = [1080, 1180] #filter out these calibs from dataset 3, because it overlaps with dataset 2
        df_tmp = df_tmp[~df_tmp["calib"].isin(overlap_calibs)]
    dfs.append(df_tmp)

df = pd.concat(dfs, ignore_index=True)

for ch, df_ch in df.groupby("channel"):   

    df_valid = df_ch.copy()
    df_valid.loc[df_valid["tot"] <= 0, "tot"] = np.nan # remove invalid TOT with value -1 for median calculation

    df_med = df_valid.groupby(["calib", "time"]).agg({"adc": "median", "tot": "median"}).reset_index() # take median over same calib and timepoint
    df_med2 = df_med.groupby("calib").agg({"adc": "median", "tot": "median"}).reset_index() #take median over the above medians
    df_max = df_med.groupby("calib").agg({"adc": "max", "tot": "max"}).reset_index() #take maximum value

    df_adc = df_max.copy()
    df_adc.loc[df_adc["tot"].notna(), "adc"] = np.nan #make ADC be NaN where TOT is active (not NaN)

    fig, ax1 = plt.subplots()
    # ADC region 
    ax1.scatter(df_adc["calib"], df_adc["adc"], color="red", s=2, label="ADC")
    ax1.set_xlabel("calib")
    ax1.set_ylabel("Max ADC [a.u.]", color="red")
    ax1.set_axisbelow(True)
    ax1.grid(True)

    # TOT region 
    ax2 = ax1.twinx() #creates second y-axis on the right side
    ax2.scatter(df_med2["calib"], df_med2["tot"], color="blue", s=2, label="TOT (median) [TDC]")
    ax2.set_ylabel("Median TOT [TDC]", color="blue")
    #plt.legend()
    plt.title(f"Channel {ch}")
    plt.savefig(output_dir / f"channel_{ch}.png", dpi=300)
    plt.close()

    # compare raw data to median to max of median 
    fig, ax = plt.subplots()
    df_tot = df_ch[df_ch["tot"] > 0] #raw TOT values

    #ax.scatter(df_tot["calib"], df_tot["tot"], color="dimgray", s=20, label="raw TOT") #raw TOT
    #ax.scatter(df_med[df_med["tot"] > 0]["calib"], df_med[df_med["tot"] > 0]["tot"], color="turquoise", s=15, marker="x", label="median TOT") # median per (calib, time)
    #ax.scatter(df_max[df_max["tot"] > 0]["calib"], df_max[df_max["tot"] > 0]["tot"], color="red", s=40, marker="x", label="max TOT") #max per calib

    norm = mcolors.Normalize(vmin=df_tot["calib"].min(), vmax=df_tot["calib"].max())
    cmap = plt.get_cmap("plasma")

    sc = ax.scatter(df_tot["calib"], df_tot["tot"], c=df_tot["calib"], cmap=cmap, norm=norm, s=10)

    cbar = plt.colorbar(sc, ax=ax)
    cbar.set_label("calib")
    ax.set_xlabel("calib")
    ax.set_ylabel("TOT [TDC]")
    ax.legend()
    plt.title(f"Channel {ch}, TOT distribution")
    ax.set_axisbelow(True)
    ax.grid(True)
    plt.savefig(output_dir / f"channel_{ch}_TOT_raw.png", dpi=300)
    plt.close()

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
    cbar.set_label("calib")

    ax.set_xlabel("time")
    ax.set_ylabel("TOT [TDC]")
    ax.set_title(f"Channel {ch} – TOT vs time")
    ax.set_axisbelow(True)
    ax.grid(True)
    plt.savefig(output_dir / f"channel_{ch}_TOT_vs_time_lines.png", dpi=300)
    plt.close()