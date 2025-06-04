"""reading data output by pflib"""

import json
import pandas as pd

def read_pflib_csv(fp, **kwargs):
    """read a "pflib CSV" file into memory

    A "pflib CSV" file is just a CSV file with (optionally) an additional
    line at the top specifying additional run parameters.
    This line begins with `#` and then contains the run parameters serialized
    as a single-line JSON dictionary.

    Parameters
    ----------
    fp: str | pathlib.Path
        filepath to pflib CSV to load
    kwargs: dict[str,Any]
        additional key-word arguments given to pd.read_csv

    Returns
    -------
    pandas.DataFrame, dict
        2-tuple containing the pd.DataFrame of the CSV data
        and a dictionary of the run parameters from the header
        if no header was included, then the run_parameters dictionary is empty
    """
    with open(fp) as f:
        header = f.readline().strip('\n')
    has_pflib_header = header[0] == '#'
    run_params = (
        json.loads(header.strip('#'))
        if has_pflib_header else
        {}
    )
    if has_pflib_header:
        kwargs['skiprows'] = 1
    data = pd.read_csv(fp, **kwargs)
    return data, run_params
