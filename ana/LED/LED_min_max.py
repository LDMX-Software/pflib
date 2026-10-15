import numpy as np
import matplotlib.pyplot as plt
import pandas as pd
from pathlib import Path
import argparse

"""
This plotting script was done as pedestal fluctuations/small pulses were observed on every 4th channel when doing LED flashes, even when no CMB was connected.
The script came to be on Lennart's request, to check for any local dependencies of the amplitude across the board. This way he wanted to check if there is a pattern 
which channels are the most affected, and what physical issue on the boards themselves could cause these pulses.
"""

parser = argparse.ArgumentParser()
parser.add_argument('dataset', type=Path, help='LED bias scan file path')
parser.add_argument('pedestal', type=Path, help='Pedestal scan file path')
args = parser.parse_args()

data_dir = args.dataset.parent
output_dir = data_dir / f"{args.dataset.stem}_LED_min_max"
output_dir.mkdir(exist_ok = True)

df = pd.read_csv(args.dataset, comment='#', skipinitialspace=True)
df_ped = pd.read_csv(args.pedestal, comment='#', skipinitialspace=True)

df_ped = df_ped[df_ped['channel'] != 'calib'].copy() # drop calibration channel
df_ped['channel'] = df_ped['channel'].astype(int)

pedestal = df_ped.groupby(['i_link', 'channel'], as_index=False).agg(pedestal=('adc', 'mean')) #takes the mean of the 100 pedestal events, for the two links separately
pedestal['ch'] = pedestal['i_link'] * 36 + pedestal['channel'] #conversion needed, as pedestal csv saves ch 0-35 with i_link = 0 or 1, to differentiate between links, whereas the LED csv already saves ch 0-70

all_ch = sorted(df['ch'].unique())

pulse_ch = all_ch[2::4] #selecting every 4th channel, starting with CH2

df = df[df['ch'].isin(pulse_ch)] #keeps only the channels that are within pulse_ch, hence give True

dacSiPM = df['dacSiPM'].unique()
dacLED = df['dacLED'].unique()

med = df.groupby(['ch', 'time'], as_index=False).agg(adc=('adc', 'median')) #take median over events
med = med.merge(pedestal, on='ch') #merge each pedestal to the LED pulses for each channel respectively
med['adc_ped_sub'] = med['adc'] - med['pedestal'] #subtract pedestal of the LED ADC values, for all channels respectively

extrema = med.groupby('ch').agg(adc_min=('adc_ped_sub','min'), adc_max=('adc_ped_sub', 'max')).reset_index() #find min and max for each channel

fig, ax = plt.subplots(figsize=(8, 5))

ax.scatter(extrema['ch'], extrema['adc_max'], marker='.', s=40, color='tab:red', label='max')
ax.scatter(extrema['ch'], extrema['adc_min'], marker='.', s=30, color='tab:blue', label='min')

ax.set_xticks(extrema['ch'])
ax.set_xlabel('channel')
ax.set_ylabel('Median ADC minus pedestal')
ax.set_title(f'LED Pulse Min/Max of Median Pulse with subtracted pedestal\n SiPM DAC {dacSiPM[0]}, LED DAC {dacLED[0]}')
ax.set_axisbelow(True)
ax.grid(True)
ax.legend()
plt.tight_layout()
plt.savefig(output_dir / 'LED_min_max_ped_sub.png', dpi=300)
plt.close()