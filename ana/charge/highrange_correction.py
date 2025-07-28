import update_path
from pathlib import Path
import argparse

import matplotlib.pyplot as plt
import numpy as np

from read import read_pflib_csv
from get_params import get_params

parser = argparse.ArgumentParser()
parser.add_argument("lowrange_csv", type=Path, help="Path to the lowrange CSV file.")
parser.add_argument("highrange_csv", type=Path, help="Path to the highrange CSV file.")
parser.add_argument('-xl','--xlabel', default='Q [fC]', type=str, help=f'What to label the x-axis with.')
parser.add_argument('-yl', '--ylabel', default='ADC', type=str, help=f'What to label the y-axis with.')
parser.add_argument("-o", "--output", type=Path, default=Path("."), help="Output directory for plots.")
args = parser.parse_args()

def highrange_correction(
    samples_list,
    run_params_list,
    adcmax_linear = 600 #cut to disregard non-linear data
): 

    """
    This function does two linear fits to the highrange and lowrange ADC vs Q [fC]. 
    The conversion from calib to fC depends on the individual HGCROC setup, meaning the max_dac_volt variable needs to be changed to the respective applied DAC voltage,
    in Lund this is 0.871V. It also assumes that the capacitances are the same for all the channels.

    Assuming that the lowrange is "correct", the highrange data is corrected so that it aligns with lowrange. 
    This only happens in the linear region, meaning it excludes everything above an ADC of 600. 
    Then both the lowrange and highrange slopes are plotted against the channel number. These two plots are a good basis for comparison studies of the ADC linearity 
    across channels. 

    Requires a two csv input, first lowrange then highrange!
    """

    low_samples, high_samples = samples_list
    low_run_params, high_run_params = run_params_list
    
    channels = sorted(low_samples['channel'].unique())

    m_high_all_ch = []
    m_low_all_ch = []

    for ch in channels:
        low_ch  = low_samples[low_samples['channel'] == ch]
        high_ch = high_samples[high_samples['channel'] == ch]

        def extract_x_y(samples, run_params):
            groups, param_name = get_params(samples, 0)
            cap = 8e-12 if run_params['highrange'] else 500e-15 #for conversion of calib to charge Q[C]
            max_dac_volt = 0.871 #in [V], change this to the respective applied maximum DAC voltage for lowrange and highrange 

            x, y = [],[]
            for _, group_df in groups:
                q = (group_df[param_name].iloc[0]/4095)*cap*max_dac_volt*(10**(15))
                adc_max = group_df['adc'].max()

                if adc_max <= adcmax_linear:
                    x.append(q)
                    y.append(adc_max)

            return np.array(x), np.array(y)

        low_x, low_y = extract_x_y(low_ch, low_run_params)
        high_x, high_y = extract_x_y(high_ch, high_run_params)

        m_low, b_low = np.polyfit(low_x, low_y, deg=1) #slope=m, intercept=b
        m_high, b_high = np.polyfit(high_x, high_y, deg=1)
        m_low_all_ch.append(m_low)
        m_high_all_ch.append(m_high)

        high_x_corr = ((m_high*high_x) + b_high - b_low)/m_low
        m_high_corr, b_high_corr = np.polyfit(high_x_corr, high_y, deg=1)

        #points for linear fits
        xs_low  = np.linspace(0, max(low_x),  2)
        xs_high = np.linspace(0, max(high_x), 2)

        fig, ax = plt.subplots()
        #lowrange data and fit
        ax.scatter(low_x, low_y, color='lightskyblue', label='lowrange data')
        ax.plot(xs_low, m_low*xs_low + b_low, '--', color='black',  label='lowrange fit')

        #highrange raw data and fit
        ax.scatter(high_x, high_y, color='orange', label='highrange raw')
        ax.plot(xs_high, m_high*xs_high + b_high, '--', color='orange', label='highrange fit')

        #highrange corrected
        ax.scatter(high_x_corr, high_y,  color='red', marker='x', label=f'highrange corrected')

        ax.set_xlabel('Q [fC]')
        ax.set_ylabel('ADC')
        ax.set_title(f'Lowrange and Highrange for CH{ch}')
        ax.grid()
        ax.legend()   

        outdir = args.lowrange_csv.parent / f"hrcorr_ch{channels[0]}_to_ch{channels[-1]}"
        outdir.mkdir(parents=True, exist_ok=True)
        out_file = outdir / f"HR_CORR_CH{ch}.png"
        plt.savefig(out_file)
        plt.close(fig)

    #m_high against channel number plot
    fig, ax = plt.subplots()
    ax.plot(channels, m_high_all_ch)
    ax.set_xlabel('Channel number')
    ax.set_ylabel('m_high')
    ax.set_title('Slope of highrange against channel number')
    scale_factor_outfile = args.lowrange_csv.parent / "m_high.png"
    plt.savefig(scale_factor_outfile)
    plt.close(fig)

    #m_low against channel number plot
    fig, ax = plt.subplots()
    ax.plot(channels, m_low_all_ch)
    ax.set_xlabel('Channel number')
    ax.set_ylabel('m_low')
    ax.set_title('Slope of lowrange against channel number')
    scale_factor_outfile = args.lowrange_csv.parent / "m_low.png"
    plt.savefig(scale_factor_outfile)
    plt.close(fig)

low_samples, low_run_params = read_pflib_csv(args.lowrange_csv)
high_samples, high_run_params = read_pflib_csv(args.highrange_csv)

highrange_correction(samples_list=[low_samples, high_samples],run_params_list=[low_run_params, high_run_params])