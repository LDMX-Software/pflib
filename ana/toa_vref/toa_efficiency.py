# Calculates TOA_EFFICIENCY for a given dataset,
# requires a "toa_vref_scan.csv" file to run correctly.
# Then, plots the efficiency versus toa_vref and saves the plot.

import pandas as pd
import matplotlib.pyplot as plt

fp = 'toa_vref_scan.csv' # your file path here
fp = 'toa_vref_scan_20250710_170132.csv'

data = pd.read_csv(fp,
                   skiprows = 1)

channel_lists = []
for chan in range(72):
    channel_lists.append([])

unique_toa_vrefs = data['TOA_VREF'].unique()

for toa_vref in unique_toa_vrefs:
    filtered_data = data[data['TOA_VREF'] == toa_vref]
    for chan in range(72):
        non_zero = filtered_data[filtered_data[str(chan)] != 0]
        chan_triggers = len(non_zero)
        chan_toa_efficiency = chan_triggers / len(filtered_data)
        channel_lists[chan].append(chan_toa_efficiency)

# Plotting the results
plt.figure(figsize=(10, 6))
for chan in range(72):
    plt.plot(channel_lists[chan], 
             marker = 'o', 
             label=f'Ch. {chan}', 
             linestyle='none')
plt.title('TOA Efficiency vs TOA VREF per channel')
plt.xlabel('TOA VREF')
plt.ylabel('TOA Efficiency')
plt.tight_layout()
plt.savefig('toa_efficiency_plot.png')
plt.show()
print("Plot saved as 'toa_efficiency_plot.png'")