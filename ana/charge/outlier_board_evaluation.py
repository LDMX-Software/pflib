import numpy as np
import matplotlib.pyplot as plt
import pandas as pd
import os

from pathlib import Path
import argparse

parser = argparse.ArgumentParser()
parser.add_argument('results', type=Path, nargs='+', help='Results of the outlier_scan algorithm')
args = parser.parse_args()

def read_results(path):
    calib_data = []
    for file in path:
        outlier_sum = []
        df = pd.read_csv(file)
        channels = np.unique(df.Channel.to_numpy())
        for ch in channels:
            outlier_sum.append(df[df.Channel == ch].O_counts.to_numpy().sum())
        calib = np.unique(df.CALIB.to_numpy())
        calib_data.append([outlier_sum,calib])
    print(calib_data)

read_results(args.results)



