import numpy as np
import matplotlib.pyplot as plt
import pandas as pd
from pathlib import Path
import argparse

parser = argparse.ArgumentParser()
parser.add_argument('dataset', type=Path, help='LED bias scan file path')
args = parser.parse_args()

data_dir = args.dataset.parent
output_dir = data_dir / f"{args.dataset.stem}_LED_pulses"
output_dir.mkdir(exist_ok = True)

df = pd.read_csv(args.dataset, comment='#', skipinitialspace=True)

channel_groups = df.groupby('ch')

for channel, ch_df in channel_groups:

    # group by SiPM DAC
    sipm_groups = ch_df.groupby('dacSiPM')

    for dacSiPM, sipm_df in sipm_groups:

        fig, ax = plt.subplots(figsize=(8, 5))

        med = sipm_df.groupby(['dacLED', 'time'], as_index=False).agg(adc=('adc', 'median')).sort_values(['dacLED', 'time']) # compute median over multiple events

        cmap = plt.get_cmap('viridis')
        led_groups = med.groupby('dacLED')
        n = len(led_groups)

        for i, (dacLED, med_df) in enumerate(led_groups):

            color = cmap(i / max(n - 1, 1))
            ax.scatter(med_df['time'], med_df['adc'], marker='o', s=10, linewidth=1, color=color, label=f'LED DAC {dacLED}')

        ax.set_xlabel('Time [ns]')
        ax.set_ylabel('Median ADC')
        ax.set_title(f'LED Pulse Scan\n HGCROC Channel {channel}, SiPM DAC {dacSiPM}')
        ax.grid(True)
        ax.legend(bbox_to_anchor=(1.05, 1), loc='upper left', fontsize=7)
        plt.tight_layout()

        output_name = (f'channel_{channel}_sipmDAC_{dacSiPM}.png')
        plt.savefig(output_dir / output_name, dpi=300)
        plt.close()
