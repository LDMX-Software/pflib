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
