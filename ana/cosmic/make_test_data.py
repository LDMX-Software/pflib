"""Write synthetic ECal and HCal decoded CSVs with known phase and latency

Lets the cosmic analysis chain be run end to end before real data exists.
Sample i of a burst reads BX = L1A - L1OFFSET + i, so a pulse at
BX = L1A - true_latency lands in sample L1OFFSET - true_latency.
"""

import argparse

import numpy as np
import pandas as pd

from hgcroc_fit import ESA_CLOCK_NS, load_shape

COLUMNS = [
    "timestamp",
    "orbit",
    "bx",
    "event",
    "i_sample",
    "i_link",
    "channel",
    "Tp",
    "Tc",
    "adc_tm1",
    "adc",
    "tot",
    "toa",
]


def write_csv(path, timestamp, adc, toa):
    """adc, toa: (n_ror, n_link, 36, n_sample), calib rows are written as zeros"""
    n_ror, n_link, n_ch, n_s = adc.shape
    rows = []
    for r in range(n_ror):
        for s in range(n_s):
            for lk in range(n_link):
                rows.append(
                    (timestamp[r], 0, 0, r * n_s + s, s, lk, "calib", 0, 0, 0, 0, 0, 0)
                )
                prev = adc[r, lk, :, s - 1] if s > 0 else adc[r, lk, :, 0]
                for c in range(n_ch):
                    rows.append(
                        (
                            timestamp[r],
                            0,
                            0,
                            r * n_s + s,
                            s,
                            lk,
                            c,
                            0,
                            0,
                            prev[c],
                            adc[r, lk, c, s],
                            0,
                            toa[r, lk, c, s],
                        )
                    )
    pd.DataFrame(rows, columns=COLUMNS).to_csv(path, index=False)


def moyal(rng, size):
    # Landau-like tail: -ln(z^2), z normal
    return -np.log(rng.normal(size=size) ** 2)


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("-o", "--output", default="test", help="output prefix")
    p.add_argument("-n", "--n-ror", type=int, default=1000)
    p.add_argument("--n-samples", type=int, default=4)
    p.add_argument("--l1offset", type=int, default=12)
    p.add_argument(
        "--true-latency", type=int, default=11, help="BX between the muon and the L1A"
    )
    p.add_argument("--noise", type=float, default=1.0, help="ECal noise [ADC]")
    p.add_argument(
        "--common-mode",
        type=float,
        default=0.5,
        help="common mode noise per link [ADC]",
    )
    p.add_argument("--mip-adc", type=float, default=11.0)
    p.add_argument(
        "--hcal-offset",
        type=float,
        default=5.0,
        help="HCal hit time minus ECal pulse t0 [ns]",
    )
    p.add_argument(
        "--hcal-resolution",
        type=float,
        default=1.0,
        help="HCal TOA time resolution [ns]",
    )
    p.add_argument("--clock", type=float, default=ESA_CLOCK_NS)
    p.add_argument("--seed", type=int, default=1)
    p.add_argument(
        "--true-shape", help="pulse shape to simulate, five comma separated numbers"
    )
    p.add_argument(
        "--roc-phase",
        type=float,
        default=2.0,
        help="PHASE_CK offset of ROC 1 in the time scans [ns]",
    )
    args = p.parse_args()

    rng = np.random.default_rng(args.seed)
    shape = load_shape(args.true_shape)
    n_ror, n_s, clock = args.n_ror, args.n_samples, args.clock
    t_samples = np.arange(n_s) * clock
    phase = rng.uniform(0, clock, n_ror)
    t0 = (args.l1offset - args.true_latency) * clock + phase
    timestamp = 1000 + 7 * np.arange(n_ror)

    # ECal: 12 eRx x 36 channels, a few MIP cells per event
    ped = rng.uniform(40, 60, (12, 36))
    noise = args.noise * rng.uniform(0.8, 1.2, (12, 36))
    amp = np.zeros((n_ror, 12, 36))
    for r in range(n_ror):
        n_hit = rng.poisson(3)
        cells = rng.choice(12 * 36, size=n_hit, replace=False)
        a = args.mip_adc * (1 + 0.12 * moyal(rng, n_hit))
        amp[r].flat[cells] = np.clip(a, 0, None)
    pulse = amp[..., None] * shape(t_samples[None, :] - t0[:, None])[:, None, None, :]
    cm = args.common_mode * rng.normal(size=(n_ror, 12, 1, n_s))
    raw = (
        ped[None, :, :, None]
        + cm
        + noise[None, :, :, None] * rng.normal(size=pulse.shape)
        + pulse
    )
    ecal_adc = np.clip(np.floor(raw + 0.5), 0, 1023).astype(int)
    write_csv(f"{args.output}_ecal.csv", timestamp, ecal_adc, np.zeros_like(ecal_adc))

    # HCal: 2 eRx, a few bars with large pulses and TOA
    h_ped = rng.uniform(80, 120, (2, 36))
    h_amp = np.zeros((n_ror, 2, 36))
    h_toa = np.zeros((n_ror, 2, 36, n_s), dtype=int)
    for r in range(n_ror):
        bars = rng.choice(72, size=rng.integers(2, 5), replace=False)
        h_amp[r].flat[bars] = rng.uniform(100, 300, len(bars))
        for b in bars:
            t_hit = t0[r] + args.hcal_offset + rng.normal(0, args.hcal_resolution)
            s = int(t_hit // clock)
            if 0 <= s < n_s:
                h_toa[r].reshape(72, n_s)[b, s] = int(
                    (t_hit - s * clock) / clock * 1024
                )
    h_pulse = (
        h_amp[..., None] * shape(t_samples[None, :] - t0[:, None])[:, None, None, :]
    )
    h_raw = h_ped[None, :, :, None] + 2.0 * rng.normal(size=h_pulse.shape) + h_pulse
    hcal_adc = np.clip(np.floor(h_raw + 0.5), 0, 1023).astype(int)
    write_csv(f"{args.output}_hcal.csv", timestamp, hcal_adc, h_toa)

    pd.DataFrame({"timestamp": timestamp, "phase_ns": phase, "ecal_t0_ns": t0}).to_csv(
        f"{args.output}_truth.csv", index=False
    )
    r, lk, c = np.nonzero(amp)
    pd.DataFrame({"ror": r, "link": lk, "channel": c, "amp_adc": amp[r, lk, c]}).to_csv(
        f"{args.output}_truth_hits.csv", index=False
    )
    # CHARGE_TIMESCAN like scans of one channel on two ROCs, time axis in 25 ns BX
    for roc, dphase in ((0, 0.0), (1, args.roc_phase)):
        tt = np.arange(-50, 150, 25 / 16)
        a = (
            200 * shape(tt * clock / 25 - (20 + dphase))
            + 50
            + rng.normal(0, 1, len(tt))
        )
        with open(f"{args.output}_timescan_roc{roc}.csv", "w") as f:
            f.write(f'# {{"iroc": {roc}, "channel": 10, "calib": 200}}\n')
            pd.DataFrame(
                {
                    "time": tt,
                    "Tp": 0,
                    "Tc": 0,
                    "adc_tm1": 0,
                    "adc": np.floor(a + 0.5).astype(int),
                    "tot": 0,
                    "toa": 0,
                }
            ).to_csv(f, index=False)
    print(
        f"wrote {args.output}_ecal.csv, {args.output}_hcal.csv, {args.output}_truth[_hits].csv,"
        f" {args.output}_timescan_roc[01].csv"
    )
    print(
        f"pulse in sample {args.l1offset - args.true_latency}, true offset {-args.hcal_offset:.2f} ns"
    )


if __name__ == "__main__":
    main()
