"""print summary statistics of a pedestal run"""

import pandas as pd
import argparse

parser = argparse.ArgumentParser()
parser.add_argument('pedestals', help='decoded pedestal CSV file to summarize')
args = parser.parse_args()

samples = pd.read_csv(args.pedestals).groupby(['link','channel'])

summary = pd.DataFrame({
    'mean': samples.adc.mean(),
    'std': samples.adc.std()
})

# could sort index so the channels appear in non dictionary order
# but can't figure out the correct key function right now

print(summary.to_string())
