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

for channel, ch_df in df.groupby('ch'): #group by channel

    for role, role_df in ch_df.groupby('cmb_role'): # group by flashing or recording CMB

        for dacSiPM, sipm_df in ch_df.groupby('dacSiPM'): #group by SiPM

            if role == "flash":
                led_col = "dacLED" #the applied LED DAC on the CMB that was flashed
                title = " LEDs flashed on this CMB"
            else:
                led_col = "flash_dacLED" #the LED DAC value that was applied on the CMB that flashed
                title = "LEDs flashed on other CMB"


            fig, ax = plt.subplots(figsize=(8, 5))

            mean = sipm_df.groupby([led_col, 'time'], as_index=False).agg(adc=('adc', 'mean')).sort_values([led_col, 'time']) # compute mean over multiple events

            cmap = plt.get_cmap('viridis')
            led_groups = mean.groupby(led_col)
            n = len(led_groups)

            for i, (dacLED, mean_df) in enumerate(led_groups):

                color = cmap(i / max(n - 1, 1))
                ax.scatter(mean_df['time'], mean_df['adc'], marker='o', s=10, linewidth=1, color=color, label=f'LED DAC {dacLED}')

            ax.set_xlabel('Time [ns]')
            ax.set_ylabel('Mean ADC')
            ax.set_title(f'LED Pulse Scan\n HGCROC Channel {channel}, SiPM DAC {dacSiPM}\n {title}')
            ax.grid(True)
            ax.legend(bbox_to_anchor=(1.05, 1), loc='upper left', fontsize=7)
            plt.tight_layout()

            output_name = (f'channel_{channel}_sipmDAC_{dacSiPM}_{role}.png')
            plt.savefig(output_dir / output_name, dpi=300)
            plt.close()





