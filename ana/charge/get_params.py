def get_params(
    samples,
    num
):
    """
    Returns a data framework ordred in groups of the given parameter.
    num decides which parameter to get. num = 0 gives first param, num = 1 the second, and so on.
    """
    parameter_names = []
    for column in samples:
        if column.find('.') != -1:
            parameter_names.append(column)
    groups = samples.groupby(parameter_names[num])
    return groups, parameter_names[num]
