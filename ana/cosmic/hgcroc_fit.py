"""HGCROC pulse template fits, vectorized over many hits

Same algorithm and conventions as ldmx-sw ldmx::PulseFitter with
ldmx::HgcrocShape: sample i is at time t_i, the model is
pedestal + A * f(t_i - T), A is solved analytically for each T,
T is scanned on a grid and refined with a parabola.
"""

import numpy as np

# ECal pulse shape from ldmx-sw LDMX.Ecal.digi
# (rate_up, time_up, rate_dn, time_dn, time_peak)
ECAL_SHAPE = (-0.345, 70.6547, 0.140068, 87.7649, 77.732)

# ESA bunch clock, 185.7 MHz / 5
ESA_CLOCK_NS = 5 / 0.1857


class HgcrocShape:
    """HGCROC pulse normalized so that f(0) = 1, same as the ldmx-sw emulator"""

    def __init__(self, params=ECAL_SHAPE):
        self.ru, self.tu, self.rd, self.td, self.tp = params
        self.norm = (1 + np.exp(self.ru * (self.tp - self.tu))) * (
            1 + np.exp(self.rd * (self.tp - self.td))
        )

    def __call__(self, t):
        t = np.asarray(t, dtype=float)
        return self.norm / (
            (1 + np.exp(self.ru * (t - self.tu + self.tp)))
            * (1 + np.exp(self.rd * (t - self.td + self.tp)))
        )


def load_shape(arg):
    """HgcrocShape from a fit_template.py CSV or five comma separated numbers"""
    if arg is None:
        return HgcrocShape()
    if "," in arg:
        return HgcrocShape(tuple(float(v) for v in arg.split(",")))
    import pandas as pd

    return HgcrocShape(tuple(pd.read_csv(arg).iloc[0]))


def _grid(t_lo, t_hi, step):
    # accumulate like the C++ loop so both scan the same points
    g = []
    t = t_lo
    while t <= t_hi:
        g.append(t)
        t += step
    return np.array(g)


def fit_fixed(y, t_samples, t0, shape):
    """Amplitude only fit with known hit time

    Parameters
    ----------
    y : array (N, n)
        pedestal subtracted samples
    t_samples : array (n,)
        sample times [ns]
    t0 : float or array (N,)
        hit time per hit [ns]
    shape : HgcrocShape

    Returns
    -------
    amp, chi2, sff : arrays (N,)
        sff = sum of f^2, the amplitude error is noise / sqrt(sff)
    """
    y = np.atleast_2d(np.asarray(y, dtype=float))
    t0 = np.broadcast_to(np.asarray(t0, dtype=float), (y.shape[0],))
    f = shape(t_samples[None, :] - t0[:, None])
    sff = (f * f).sum(axis=1)
    amp = (y * f).sum(axis=1) / sff
    chi2 = ((y - amp[:, None] * f) ** 2).sum(axis=1)
    return amp, chi2, sff


def fit_free(y, t_samples, shape, t_lo=None, t_hi=None, step=0.25):
    """Fit amplitude and hit time

    The default T range keeps the peak within half a sample spacing of the
    window, otherwise noise on the tails fakes large pulses.

    Returns
    -------
    amp, t0, chi2, converged : arrays (N,)
        converged is False if no sample is above pedestal
    """
    y = np.atleast_2d(np.asarray(y, dtype=float))
    dt = t_samples[1] - t_samples[0] if len(t_samples) > 1 else 25.0
    if t_lo is None:
        t_lo = t_samples[0] - 0.5 * dt
    if t_hi is None:
        t_hi = t_samples[-1] + 0.5 * dt
    grid = _grid(t_lo, t_hi, step)

    f = shape(t_samples[None, :] - grid[:, None])  # (G, n)
    sff = (f * f).sum(axis=1)  # (G,)
    syf = y @ f.T  # (N, G)
    chi2 = (y * y).sum(axis=1)[:, None] - syf**2 / sff[None, :]
    i_best = np.argmin(chi2, axis=1)
    rows = np.arange(y.shape[0])
    best_chi2 = chi2[rows, i_best]
    best_t = grid[i_best]
    best_amp = syf[rows, i_best] / sff[i_best]

    # parabola through the best point and its two neighbours
    ok = (i_best > 0) & (i_best < len(grid) - 1)
    il, ih = np.clip(i_best - 1, 0, None), np.clip(i_best + 1, None, len(grid) - 1)
    d1, d2 = best_t - grid[il], grid[ih] - best_t
    dc1, dc2 = chi2[rows, il] - best_chi2, chi2[rows, ih] - best_chi2
    ok &= (dc1 > 0) & (dc2 > 0)
    denom = 2 * (d1 * dc2 + d2 * dc1)
    with np.errstate(divide="ignore", invalid="ignore"):
        delta = np.where(
            ok & (np.abs(denom) > 1e-12), (d2 * d2 * dc1 - d1 * d1 * dc2) / denom, 0.0
        )
    ok &= np.abs(delta) < np.maximum(d1, d2)
    t_ref = best_t + np.where(ok, delta, 0.0)
    amp_ref, chi2_ref, _ = fit_fixed(y, t_samples, t_ref, shape)
    take = ok & (chi2_ref < best_chi2)

    amp = np.where(take, amp_ref, best_amp)
    t0 = np.where(take, t_ref, best_t)
    chi2_out = np.where(take, chi2_ref, best_chi2)
    converged = (y > 0).any(axis=1)
    return amp, t0, chi2_out, converged
