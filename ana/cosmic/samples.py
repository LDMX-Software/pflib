"""Load decoded ECON-D CSVs (rogue-decoder.py, pftool DAQ) into arrays"""

import sys
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).parent.parent))

from read import read_pflib_csv


class Samples:
    """Decoded samples as arrays indexed [ror, link, channel, sample]

    Attributes
    ----------
    adc, toa, tot : int arrays (n_ror, n_link, 36, n_sample)
    timestamp : int array (n_ror,)
        ROR timestamp, 0 if the data has no LDMX ROR header
    link_name : str
        "i_link" or "i_roc" depending on the decoder mapping
    run_params : dict
        JSON header of the pflib CSV, if any
    """

    def __init__(self, fp):
        df, self.run_params = read_pflib_csv(fp)
        df.columns = df.columns.str.strip()
        self.link_name = "i_roc" if "i_roc" in df.columns else "i_link"
        # each ROR starts with the calib row of the first link in sample 0
        first_link = df[self.link_name].min()
        start = (
            (df["i_sample"] == 0)
            & (df[self.link_name] == first_link)
            & df["channel"].astype(str).isin(["calib", "calib_0"])
        )
        ror = (start.cumsum() - 1).to_numpy()
        df = df.assign(ror=ror)
        df = df[~df["channel"].astype(str).str.startswith("calib")]
        ch = df["channel"].astype(int).to_numpy()
        n_ch = 72 if self.link_name == "i_roc" else 36

        links = np.sort(df[self.link_name].unique())
        il = np.searchsorted(links, df[self.link_name].to_numpy())
        ir = df["ror"].to_numpy()
        isam = df["i_sample"].to_numpy()
        shape = (ir.max() + 1, len(links), n_ch, isam.max() + 1)
        self.links = links
        for col in ("adc", "toa", "tot"):
            a = np.zeros(shape, dtype=np.int32)
            a[ir, il, ch, isam] = df[col].to_numpy()
            setattr(self, col, a)
        self.timestamp = df.groupby("ror")["timestamp"].first().to_numpy()

    @property
    def n_samples(self):
        return self.adc.shape[-1]

    def channel_ids(self):
        """(link, channel) pairs in the same order as the flattened arrays"""
        n_link, n_ch = self.adc.shape[1:3]
        return [(self.links[i], c) for i in range(n_link) for c in range(n_ch)]
