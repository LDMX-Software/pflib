"""ECal MIP amplitudes with the HGCROC template fit

Without --phase: free fit (amplitude and time) of channels with a seed.
With --phase (from hcal_phase.py): the HCal to ECal offset is calibrated
as the median of free-fit t0 minus HCal time on large hits (or taken from
--offset), then every channel is fit with t0 = HCal time + offset, which
is much less sensitive to noise than the free fit.
"""

import argparse

import numpy as np
import pandas as pd

from hgcroc_fit import ESA_CLOCK_NS, fit_fixed, fit_free, load_shape
from pedestal_noise import pedestal_table
from samples import Samples


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("input", help="decoded ECal CSV")
    p.add_argument(
        "--pedestals",
        help="pedestal table from pedestal_noise.py (default: from the data)",
    )
    p.add_argument("--phase", help="HCal phase CSV from hcal_phase.py")
    p.add_argument(
        "--offset", type=float, help="ECal t0 minus HCal time [ns], default: calibrate"
    )
    p.add_argument("--clock", type=float, default=ESA_CLOCK_NS)
    p.add_argument(
        "--shape", help="template from fit_template.py (default: ldmx-sw ECal shape)"
    )
    p.add_argument(
        "--seed-sigma",
        type=float,
        default=4.0,
        help="free fit seed, max sample over noise",
    )
    p.add_argument(
        "--big-sigma",
        type=float,
        default=8.0,
        help="free-fit amplitude for the offset calibration",
    )
    p.add_argument(
        "--save-sigma",
        type=float,
        default=3.0,
        help="fixed-fit significance A sqrt(sum f^2) / noise to save a hit",
    )
    p.add_argument(
        "--mip-adc", type=float, help="MIP in ADC, to print amplitudes in MIPs"
    )
    p.add_argument("-o", "--output", help="hits CSV, default <input>_hits.csv")
    p.add_argument("--plot", help="save the amplitude spectrum")
    args = p.parse_args()

    s = Samples(args.input)
    shape = load_shape(args.shape)
    peds = pd.read_csv(args.pedestals) if args.pedestals else pedestal_table(s)
    n_ror, n_link, n_ch, n_s = s.adc.shape
    ped = peds["pedestal"].to_numpy().reshape(n_link, n_ch)
    noise = peds["noise_cm"].to_numpy().reshape(n_link, n_ch)
    y = s.adc.astype(float) - ped[None, :, :, None]
    y -= np.median(y, axis=2, keepdims=True)  # common mode
    t_samples = np.arange(n_s) * args.clock
    sigma = np.broadcast_to(noise[None], (n_ror, n_link, n_ch))
    ror_idx = np.broadcast_to(np.arange(n_ror)[:, None, None], sigma.shape)
    link_idx = np.broadcast_to(np.arange(n_link)[None, :, None], sigma.shape)
    ch_idx = np.broadcast_to(np.arange(n_ch)[None, None, :], sigma.shape)

    seed = y.max(axis=-1) > args.seed_sigma * sigma
    a_free, t_free, chi2_free, conv = fit_free(y[seed], t_samples, shape)
    print(
        f"{n_ror} RORs, {seed.sum()} seeded channels, median free-fit amplitude"
        f" {np.median(a_free[conv]) if conv.any() else 0:.1f} ADC"
    )

    out = {
        "ror": ror_idx[seed],
        "link": s.links[link_idx[seed]],
        "channel": ch_idx[seed],
        "amp_free": a_free,
        "t0_free": t_free,
        "chi2_free": chi2_free,
    }
    amp_key = "amp_free"

    if args.phase:
        ph = pd.read_csv(args.phase)
        use_ts = (s.timestamp != 0).any()
        key = s.timestamp if use_ts else np.arange(n_ror)
        lut = dict(zip(ph["timestamp"] if use_ts else ph["ror"], ph["hcal_time_ns"]))
        hcal = np.array([lut.get(k, np.nan) for k in key])
        print(f"{np.isfinite(hcal).sum()} / {n_ror} RORs matched to an HCal time")

        offset = args.offset
        if offset is None:
            big = (
                conv
                & (a_free > args.big_sigma * sigma[seed])
                & np.isfinite(hcal[ror_idx[seed]])
            )
            d = t_free[big] - hcal[ror_idx[seed]][big]
            if len(d) < 10:
                raise SystemExit(
                    f"only {len(d)} large hits to calibrate the offset, give --offset"
                )
            offset = float(np.median(d))
            print(
                f"offset {offset:.2f} ns from {len(d)} hits, rms {d.std():.2f} ns,"
                f" uncertainty {1.2533 * d.std() / np.sqrt(len(d)):.2f} ns"
            )

        good = np.isfinite(hcal)
        t0 = np.broadcast_to((hcal + offset)[:, None, None], sigma.shape)
        sel = np.broadcast_to(good[:, None, None], sigma.shape)
        a_fix, chi2_fix, sff = fit_fixed(
            y[sel].reshape(-1, n_s), t_samples, t0[sel], shape
        )
        # cut on significance, the amplitude error depends on the phase
        signif = a_fix * np.sqrt(sff) / sigma[sel]
        keep = signif > args.save_sigma
        print(
            f"amplitude error / noise: median {np.median(1 / np.sqrt(sff)):.2f},"
            f" worst phase {np.max(1 / np.sqrt(sff)):.2f}"
        )
        out = {
            "ror": ror_idx[sel][keep],
            "link": s.links[link_idx[sel][keep]],
            "channel": ch_idx[sel][keep],
            "amp_fixed": a_fix[keep],
            "signif": signif[keep],
            "t0_fixed": t0[sel][keep],
            "chi2_fixed": chi2_fix[keep],
        }
        amp_key = "amp_fixed"

    hits = pd.DataFrame(out)
    hits.insert(0, "timestamp", s.timestamp[hits["ror"].to_numpy()])
    path = args.output or args.input.replace(".csv", "_hits.csv")
    hits.to_csv(path, index=False)

    amp = hits[amp_key].to_numpy()
    unit, scale = ("MIP", args.mip_adc) if args.mip_adc else ("ADC", 1.0)
    h, e = np.histogram(amp / scale, bins=60, range=(0, 3 if args.mip_adc else 40))
    # look for the MIP peak above the noise tail
    above = e[:-1] > 5 * np.median(noise) / scale
    i_pk = np.argmax(np.where(above, h, -1))
    peak = 0.5 * (e[i_pk] + e[i_pk + 1])
    print(
        f"{len(hits)} hits -> {path}, spectrum peak about {peak:.2f} {unit} ({amp_key})"
    )

    if args.plot:
        import matplotlib.pyplot as plt

        fig, ax = plt.subplots()
        ax.stairs(h, e)
        ax.set_xlabel(f"{amp_key} [{unit}]")
        ax.set_ylabel("hits")
        fig.savefig(args.plot, bbox_inches="tight")


if __name__ == "__main__":
    main()
