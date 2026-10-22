"""Fit the HGCROC pulse template to CHARGE_TIMESCAN runs

Each input is a time scan of one channel (pftool TASKS.CHARGE_TIMESCAN).
The shape parameters rate_up, rate_dn, time_dn are fit with time_up and
time_peak fixed at the ldmx-sw values (they are degenerate with the hit
time), plus an amplitude and a hit time per scan. The median shape is written for mip_fit.py --shape
(same order as ldmx-sw pulse_shape). The hit time per ROC shows how well
PHASE_CK is equalized.

The time-scan axis assumes 25 ns per BX; --clock rescales it to the
actual bunch clock.
"""

import argparse
import sys
from pathlib import Path

import numpy as np
import pandas as pd
from scipy.optimize import curve_fit

from hgcroc_fit import ECAL_SHAPE, ESA_CLOCK_NS, HgcrocShape

sys.path.insert(0, str(Path(__file__).parent.parent))
from read import read_pflib_csv

TIME_UP, TIME_PEAK = ECAL_SHAPE[1], ECAL_SHAPE[4]


def model(t, amp, t0, ru, rd, td):
    return amp * HgcrocShape((ru, TIME_UP, rd, td, TIME_PEAK))(t - t0)


def fit_scan(t, adc):
    # pedestal from the earliest tenth of the scan, before the pulse
    early = t <= np.quantile(t, 0.1)
    ped = np.median(adc[early])
    y = adc - ped
    p0 = [y.max(), t[np.argmax(y)], ECAL_SHAPE[0], ECAL_SHAPE[2], ECAL_SHAPE[3]]
    popt, pcov = curve_fit(model, t, y, p0=p0, maxfev=20000)
    resid = y - model(t, *popt)
    return ped, popt, np.sqrt(np.diag(pcov)), resid.std()


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("inputs", nargs="+", help="CHARGE_TIMESCAN CSVs")
    p.add_argument("--clock", type=float, default=ESA_CLOCK_NS)
    p.add_argument(
        "-o",
        "--output",
        default="template.csv",
        help="shape parameters for mip_fit.py --shape",
    )
    p.add_argument("--plot", help="save the scans with the fitted template")
    args = p.parse_args()

    rows, curves = [], []
    for f in args.inputs:
        df, head = read_pflib_csv(f)
        df.columns = df.columns.str.strip()
        t = df["time"].to_numpy(float) * args.clock / 25.0
        adc = df["adc"].to_numpy(float)
        ped, popt, perr, rms = fit_scan(t, adc)
        rows.append(
            {
                "file": f,
                "iroc": head.get("iroc"),
                "channel": head.get("channel"),
                "calib": head.get("calib"),
                "pedestal": ped,
                "amp": popt[0],
                "t0": popt[1],
                "t0_err": perr[1],
                "rate_up": popt[2],
                "time_up": TIME_UP,
                "rate_dn": popt[3],
                "time_dn": popt[4],
                "resid_rms": rms,
            }
        )
        curves.append((t, adc - ped, popt))

    res = pd.DataFrame(rows)
    print(
        res.drop(columns="file").to_string(
            index=False, float_format=lambda v: f"{v:.3f}"
        )
    )
    shape = [res[k].median() for k in ("rate_up", "time_up", "rate_dn", "time_dn")] + [
        TIME_PEAK
    ]
    pd.DataFrame(
        [shape], columns=["rate_up", "time_up", "rate_dn", "time_dn", "time_peak"]
    ).to_csv(args.output, index=False)
    print(f"median shape {', '.join(f'{v:.4g}' for v in shape)} -> {args.output}")
    if res["iroc"].notna().any() and res["iroc"].nunique() > 1:
        per_roc = res.groupby("iroc")["t0"].median()
        print(
            "hit time per ROC [ns] (equalize PHASE_CK if these differ by more than about 1 ns)"
        )
        print(per_roc.round(2).to_string())

    if args.plot:
        import matplotlib.pyplot as plt

        fig, ax = plt.subplots()
        for t, y, popt in curves:
            order = np.argsort(t)
            ax.plot(t[order], y[order], ".", ms=2)
            ax.plot(t[order], model(t[order], *popt), "-", lw=1)
        ax.set_xlabel("time [ns]")
        ax.set_ylabel("ADC - pedestal")
        fig.savefig(args.plot, bbox_inches="tight")


if __name__ == "__main__":
    main()
