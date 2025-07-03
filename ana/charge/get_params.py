"""
    Returns a data framework ordred in groups of the given parameter
"""


def get_params(
    samples
):
    parameter_names = []
    for column in samples:
        if column.find('.') != -1:
            parameter_names.append(column)
    groups = samples.groupby(parameter_names[0])
    return groups, parameter_names[0]
