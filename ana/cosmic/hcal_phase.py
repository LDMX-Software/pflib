"""Event time from HCal TOA, the phase reference for the ECal fit

Hit time = sample index x clock + TOA x clock / 1024, measured from the
start of sample 0. The event time is the median over channels with TOA.
Check the TOA direction on the bench (--toa-reverse if TOA counts to the
next clock edge) and add a time walk correction once it is measured.
The HCal to ECal offset is calibrated per run by mip_fit.py.
"""

import argparse

import numpy as np
import pandas as pd

from hgcroc_fit import ESA_CLOCK_NS
from samples import Samples


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("input", help="decoded HCal CSV")
    p.add_argument("-o", "--output", help="phase CSV, default <input>_phase.csv")
    p.add_argument("--clock", type=float, default=ESA_CLOCK_NS)
    p.add_argument(
        "--toa-reverse",
        action="store_true",
        help="TOA counts from the hit to the next clock edge",
    )
    p.add_argument(
        "--min-hits", type=int, default=1, help="channels with TOA required per event"
    )
    args = p.parse_args()

    s = Samples(args.input)
    toa = s.toa.astype(float)
    frac = (1024 - toa) / 1024 if args.toa_reverse else toa / 1024
    t = (np.arange(s.n_samples)[None, None, None, :] + frac) * args.clock
    t = np.where(s.toa > 0, t, np.nan).reshape(len(s.timestamp), -1)
    n = np.isfinite(t).sum(axis=1)
    with np.errstate(all="ignore"):
        time = np.nanmedian(t, axis=1)
        spread = np.nanstd(t, axis=1)
    ok = n >= args.min_hits

    out = args.output or args.input.replace(".csv", "_phase.csv")
    df = pd.DataFrame(
        {
            "timestamp": s.timestamp,
            "ror": np.arange(len(n)),
            "hcal_time_ns": time,
            "n_toa": n,
            "spread_ns": spread,
        }
    )
    df[ok].to_csv(out, index=False)
    print(f"{ok.sum()} / {len(n)} RORs with >= {args.min_hits} TOA hits -> {out}")
    print(
        f"median hits {np.median(n[ok]) if ok.any() else 0:.0f},"
        f" median spread within an event {np.nanmedian(spread[ok]) if ok.any() else 0:.2f} ns"
    )


if __name__ == "__main__":
    main()
