"""Pedestal, noise and common mode per channel from a decoded CSV

Works on a pedestal run or a cosmic run: rare pulses are removed with a
5 MAD cut before the noise is computed.
The output table is the --pedestals input of latency_scan.py and mip_fit.py.
"""

import argparse

import numpy as np
import pandas as pd

from samples import Samples


def pedestal_table(s, cm_subtract=True):
    adc = s.adc.astype(float)  # (ror, link, ch, sample)
    n_link, n_ch = adc.shape[1:3]
    flat = adc.transpose(1, 2, 0, 3).reshape(n_link, n_ch, -1)
    ped = np.median(flat, axis=-1)
    mad = np.median(np.abs(flat - ped[..., None]), axis=-1)
    keep = np.abs(flat - ped[..., None]) < 5 * np.maximum(1.4826 * mad, 1.0)[..., None]
    # mean after the cut, the median of integer ADC is quantized
    ped = np.nanmean(np.where(keep, flat, np.nan), axis=-1)
    noise = np.sqrt(
        np.nanmean(np.where(keep, (flat - ped[..., None]) ** 2, np.nan), axis=-1)
    )

    y = adc - ped[None, :, :, None]
    # common mode: per event, link and sample, median over the 36 channels
    cm = np.median(y, axis=2, keepdims=True)
    y_cm = (y - cm).transpose(1, 2, 0, 3).reshape(n_link, n_ch, -1)
    noise_cm = np.sqrt(np.nanmean(np.where(keep, y_cm**2, np.nan), axis=-1))

    rows = [
        (s.links[i], c, ped[i, c], noise[i, c], noise_cm[i, c])
        for i in range(n_link)
        for c in range(n_ch)
    ]
    return pd.DataFrame(
        rows, columns=["link", "channel", "pedestal", "noise", "noise_cm"]
    )


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("input", help="decoded ECON-D CSV")
    p.add_argument(
        "-o", "--output", help="pedestal table CSV, default <input>_pedestals.csv"
    )
    args = p.parse_args()

    s = Samples(args.input)
    t = pedestal_table(s)
    out = args.output or args.input.replace(".csv", "_pedestals.csv")
    t.to_csv(out, index=False)

    print(
        f"{s.adc.shape[0]} RORs, {s.n_samples} samples, {len(s.links)} links -> {out}"
    )
    print(" link  pedestal  noise  noise_cm  dead  noisy")
    med = t["noise"].median()
    for lk, g in t.groupby("link"):
        dead = (g["noise"] < 0.05).sum()
        noisy = (g["noise"] > 3 * med).sum()
        print(
            f"{lk:5d}  {g['pedestal'].median():8.1f}  {g['noise'].median():5.2f}"
            f"  {g['noise_cm'].median():8.2f}  {dead:4d}  {noisy:5d}"
        )
    cm = np.sqrt(max(med**2 - t["noise_cm"].median() ** 2, 0))
    print(
        f"median noise {med:.2f} ADC, after common mode subtraction {t['noise_cm'].median():.2f} ADC"
        f" (common mode about {cm:.2f} ADC)"
    )


if __name__ == "__main__":
    main()
