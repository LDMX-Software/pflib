"""Find where the cosmic pulse sits in the readout window, suggest L1OFFSET

For each ROR and sample, sum the pedestal subtracted ADC of channels above
nsig x noise. Cosmics show up as an excess in one sample.
Sample i of a burst reads BX = L1A - L1OFFSET + i, so a pulse found in
sample k moves to sample k + d when L1OFFSET is raised by d.
Several runs at different L1OFFSET can be given to cover a wide range.
"""

import argparse

import numpy as np
import pandas as pd

from pedestal_noise import pedestal_table
from samples import Samples


def excess(s, peds, nsig):
    adc = s.adc.astype(float)
    ped = peds["pedestal"].to_numpy().reshape(adc.shape[1:3])
    noise = peds["noise"].to_numpy().reshape(adc.shape[1:3])
    y = adc - ped[None, :, :, None]
    y = y - np.median(y, axis=2, keepdims=True)  # common mode
    above = y > nsig * noise[None, :, :, None]
    return np.where(above, y, 0).sum(axis=(1, 2))  # (ror, sample)


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument(
        "inputs", nargs="+", help="decoded ECal CSVs, one per L1OFFSET setting"
    )
    p.add_argument(
        "--l1offset",
        type=int,
        nargs="+",
        help="L1OFFSET of each input (default from the CSV header)",
    )
    p.add_argument(
        "--pedestals",
        help="pedestal table from pedestal_noise.py (default: from the data)",
    )
    p.add_argument(
        "--nsig", type=float, default=3.0, help="channel threshold in noise units"
    )
    p.add_argument(
        "--min-excess",
        type=float,
        default=20.0,
        help="event threshold on the summed excess [ADC]",
    )
    p.add_argument(
        "--target",
        type=int,
        help="sample where the pulse should peak, default the middle one",
    )
    p.add_argument("--plot", help="save a plot of the excess vs BX relative to the L1A")
    args = p.parse_args()

    results = []
    for i, f in enumerate(args.inputs):
        s = Samples(f)
        l1 = args.l1offset[i] if args.l1offset else s.run_params.get("L1OFFSET")
        peds = pd.read_csv(args.pedestals) if args.pedestals else pedestal_table(s)
        ex = excess(s, peds, args.nsig)
        events = ex.max(axis=1) > args.min_excess
        prof = ex[events].mean(axis=0) if events.any() else np.zeros(s.n_samples)
        k_hist = np.bincount(ex[events].argmax(axis=1), minlength=s.n_samples)
        k = int(np.argmax(k_hist)) if events.any() else None
        results.append((f, l1, prof, k_hist, k))

        print(
            f"{f}: L1OFFSET {l1}, {events.sum()} / {len(ex)} RORs with excess > {args.min_excess} ADC"
        )
        print("  sample      : " + " ".join(f"{j:6d}" for j in range(s.n_samples)))
        print("  peak count  : " + " ".join(f"{c:6d}" for c in k_hist))
        print("  mean excess : " + " ".join(f"{v:6.1f}" for v in prof))
        if k is None:
            print(
                "  no cosmic excess, widen the window or step L1OFFSET by the number of samples"
            )
            continue
        if k in (0, s.n_samples - 1):
            print("  peak at the window edge, the pulse may be partly outside")
        target = s.n_samples // 2 if args.target is None else args.target
        if l1 is not None:
            print(
                f"  pulse BX relative to L1A: {k - l1}, suggested L1OFFSET {l1 + target - k}"
                f" (puts the peak in sample {target})"
            )

    if args.plot:
        import matplotlib.pyplot as plt

        fig, ax = plt.subplots()
        for f, l1, prof, _, _ in results:
            x = np.arange(len(prof)) - (l1 or 0)
            ax.plot(x, prof, "o-", label=f"L1OFFSET {l1}")
        ax.set_xlabel(
            "sample BX relative to L1A" if results[0][1] is not None else "sample"
        )
        ax.set_ylabel("mean summed excess [ADC]")
        ax.legend()
        fig.savefig(args.plot, bbox_inches="tight")


if __name__ == "__main__":
    main()
