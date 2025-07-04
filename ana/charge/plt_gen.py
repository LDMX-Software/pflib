import matplotlib.pyplot as plt

"""
    General function that defines the plotting canvas, runs the selected plotting function
    and saves it to the output
"""

def plt_gen(
    plotting_func,
    samples,
    run_params,
    output,
    xticks = True,
    ax = None,
    xlabel = 'time [ns]',
    ylabel = 'ADC',
    yval = 'adc',
    ylim = False,
    ylim_range = [0,1],
    xlim = False,
    multicsv = False,
    **kwargs
):
    plt.figure()
    plt.xlabel(xlabel)
    plt.ylabel(ylabel)
    if ylim:
        plt.ylim(ylim_range)
    if multicsv:
        plt.title('Data from multiple csv files')
    else:
        plt.title(' '.join([f'{key} = {val}' for key, val in run_params.items()]))
    plt.grid()
    #Run the specified plot
    ax = plt.gca()
    plotting_func(samples, run_params, ax)
    plt.savefig(output, bbox_inches='tight')
    plt.clf()
