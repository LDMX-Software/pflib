<<<<<<< Updated upstream
import numpy as np
import matplotlib.pyplot as plt
import pandas as pd
import numpy as np
import mplhep as hep
from pathlib import Path
import argparse

parser = argparse.ArgumentParser()
parser.add_argument('dataset', type=Path, help='LED bias scan file path')
args = parser.parse_args()

data_dir = args.dataset.parent
output_dir = data_dir / f"{args.dataset.stem}_SPS_graphs"
output_dir.mkdir(exist_ok = True)

df = pd.read_csv(args.dataset, comment='#', skipinitialspace=True)

channel_groups = df.groupby('ch')

for channel, ch_df in channel_groups:

    # group by trim_inv
    trim_inv_groups = ch_df.groupby('trim_inv')

    for trim_inv, trim_df in trim_inv_groups:

        phase_ck_groups = trim_df.groupby('phase_ck')

        fig, ax = plt.subplots(figsize=(8, 5))

        data_hist = []
        labels = []

        for phase_ck, phase_df in phase_ck_groups:

          data_hist.append(phase_df["adc"])
          SiPM_DAC = phase_df["SiPM_DAC"].iloc[10]
          LED_DAC = phase_df["LED_DAC"].iloc[10]
          labels.append(f"phase_ck = {phase_ck}")

        max_adc = int(np.max(data_hist))
        min_adc = int(np.min(data_hist))
        bins = np.arange(min_adc - 0.5, max_adc + 1.5, 1)

        counts, edges = np.histogram(data_hist, bins)
        errors = np.sqrt(counts)
        centers = (edges[:-1] + edges[1:]) / 2
        hep.histplot((counts, edges), ax=ax, histtype='step', density=False, label=labels)
        ax.errorbar(centers, counts, yerr=errors, fmt='.', capsize=2, markersize=3)
        ax.set_xlabel('ADC value')
        ax.set_ylabel('Count Per Bin')
        ax.set_title(f'Single Photon Spectrum\n HGCROC Channel =  {channel}, TRIM_INV = {trim_inv}, SiPM_DAC = {SiPM_DAC}, LED_DAC = {LED_DAC}')
        ax.grid(False)
        ax.legend(bbox_to_anchor=(1.05, 1), loc='upper left', fontsize=7)
        plt.tight_layout()
           #ax.xaxis.set_major_locator(plt.MultipleLocator(1))

        output_name = (f'channel_{channel}_trim_inv_{trim_inv}_SiPM_DAC_{SiPM_DAC}_LED_DAC_{LED_DAC}.png')
        plt.savefig(output_dir / output_name, dpi=300)
        plt.close()
=======
import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

def analyze_and_plot_sps(csv_file_path="SPS-scan.csv"):
    print(f"Loading data from: {csv_file_path}")

    df = pd.read_csv(csv_file_path, comment='#')
    df.columns = df.columns.str.strip()

    adc_col = [col for col in df.columns if col.lower() == 'adc'][0]

    # Reconstruct event IDs if not explicitly present
    event_cols = [c for c in df.columns if c.lower() in ['event', 'evt', 'event_id']]
    if event_cols:
        evt_col = event_cols[0]
    else:
        evt_col = 'event_id'
        df[evt_col] = (df['sample'] == 0).groupby([df['ch'], df['trim_inv'], df['phase_ck']]).cumsum()

    # --- STEP 1: DNL CORRECTION PER TRIM_INV SETTING ---
    # Determine base TRIM_INV and calculate shift delta: 0, 1, or 2
    base_trim = df['trim_inv'].min()
    df['delta_trim'] = df['trim_inv'] - base_trim

    # Subtract 1 ADC tic per trim_inv step from each raw BX sample
    df['adc_dnl_corrected'] = df[adc_col] - df['delta_trim']

    print(f"Step 1: Applied DNL baseline shift corrections (base TRIM_INV = {base_trim})...")

    # --- STEP 2: SUM ACROSS 3 BXs PER EVENT ---
    summed_3bx = (
        df.groupby([evt_col, 'ch', 'trim_inv', 'phase_ck'], as_index=False)['adc_dnl_corrected']
        .sum()
        .rename(columns={'adc_dnl_corrected': 'adc_3bx_dnl_corrected'})
    )

    # --- STEP 3: AVERAGE ACROSS THE THREE TRIM_INV SETTINGS ---
    print("Step 2: Combining and averaging across TRIM_INV settings...")
    avg_trim = (
        summed_3bx.groupby([evt_col, 'ch', 'phase_ck'], as_index=False)['adc_3bx_dnl_corrected']
        .mean()
        .rename(columns={'adc_3bx_dnl_corrected': 'averaged_adc_sum'})
    )

    # --- PLOTTING ---
    fig, axes = plt.subplots(1, 2, figsize=(15, 6))

    # Plot 1: Pulse Timing Scan (Phase curve)
    for ch, ch_data in avg_trim.groupby('ch'):
        phase_stats = ch_data.groupby('phase_ck')['averaged_adc_sum'].agg(['mean', 'std', 'count'])
        phase_stats['sem'] = phase_stats['std'] / np.sqrt(phase_stats['count'])

        axes[0].errorbar(
            phase_stats.index,
            phase_stats['mean'],
            yerr=phase_stats['sem'],
            fmt='-o',
            capsize=4,
            label=f'Channel {ch}'
        )

    axes[0].set_title("DNL-Corrected Phase Scan\n(3-BX Sum vs. PHASE_CK)", fontsize=12)
    axes[0].set_xlabel("PHASE_CK", fontsize=11)
    axes[0].set_ylabel("DNL-Corrected 3-BX Summed ADC", fontsize=11)
    axes[0].grid(True, linestyle="--", alpha=0.6)
    axes[0].legend()

    # Plot 2: Histogram of DNL-Smoothed Signal Amplitudes
    for ch, ch_data in avg_trim.groupby('ch'):
        axes[1].hist(
            ch_data['averaged_adc_sum'],
            bins=100,
            alpha=0.6,
            label=f'Channel {ch}',
            edgecolor='black',
            linewidth=0.5
        )

    axes[1].set_title("SPS Histogram (DNL Corrected)", fontsize=12)
    axes[1].set_xlabel("Amplitude (DNL-Corrected 3-BX Summed ADC)", fontsize=11)
    axes[1].set_ylabel("Counts", fontsize=11)
    axes[1].grid(True, linestyle="--", alpha=0.6)
    axes[1].legend()

    plt.tight_layout()
    plt.savefig("sps_readout_dnl_corrected.png", dpi=300)
    plt.show()

if __name__ == "__main__":
    analyze_and_plot_sps("SPS-scan.csv")
>>>>>>> Stashed changes
