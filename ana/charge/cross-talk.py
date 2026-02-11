
import numpy as np
import pandas as pd

import update_path
from plt_gen import plt_gen
from get_params import get_params

import os

from pathlib import Path
import argparse

import matplotlib.pyplot as plt
import matplotlib.ticker as ticker

from read import read_pflib_csv

from scipy import stats
from scipy.special import erf
from scipy.optimize import curve_fit
from scipy.signal import fftconvolve

plot_parameters = ['voltage', 'time']
amplitude_defs = ['max_adc', 'max_adc_fit', 'FWHM', 'rel_pedestal', 'rel_dip', 'all']

parser = argparse.ArgumentParser()
parser.add_argument('time_scan', type=Path, nargs='+', help='time scan data, only one event per time point')
parser.add_argument('-pp', '--plot_parameter', choices=plot_parameters, default=plot_parameters[0], type=str, help=f'What parameter to plot on the x-axis. Options: {", ".join(plot_parameters)}')
parser.add_argument('-ppu', '--plot_pulses', type=bool, help='Choose if you want to plot the pulses of all channels on the link')
parser.add_argument('-ppe', '--plot_pedestals', type=bool, help='Choose if you want to plot the pedestals of all quiet channels on the link')
parser.add_argument('-a', '--amplitude', choices=amplitude_defs, default=amplitude_defs[0], type=str, help=f'What amplitude definition to use. Options: {", ".join(amplitude_defs)}')
args = parser.parse_args()

plt.rcParams['figure.figsize'] = [15, 12]

def linear(x, m, b):
    return m*x + b
def gauss(x, H, A, x0, sigma):
    return H + A * np.exp(-(x-x0)**2 / (2*sigma**2))
def cumulative(x):
    return 0.5*(1+erf(x/np.sqrt(2)))
def skew_norm(x, H, A, x0, sigma, alpha):
    return 2 * gauss(x, H, A, x0, sigma) * cumulative(alpha*x)
def landau(x, mpv, eta):
    return stats.landau.pdf((x - mpv) / eta) / eta

samples_collection = []
run_params_collection = []
xl = []
V0 = 0

if (args.plot_parameter == plot_parameters[0]):
    for csv in args.time_scan:
        if ('ext' not in str(csv)):
            V0 = (int(str(csv).split('-')[-1].split('m')[0]))
            xl.append(0)
        else:
            xl.append(int(str(csv).split('-')[-1].split('m')[0]))
        samples, run_params = read_pflib_csv(csv)
        samples_collection.append(samples)
        run_params_collection.append(run_params)

    link = 0
    ref_ch = 16 # REF CHANNEL IS HARDCODED!
    
    # Plot pulses
    if (args.plot_pulses == True):
        for x, samples, run_params in zip(xl, samples_collection, run_params_collection):
            fig_p, ax_p = plt.subplots(1, 1)
            samples = samples[samples['nr channels'] == 1] # Remove data we don't need
            cmap = plt.get_cmap('tab20c')
            NUM_COLORS=36
            ax_p.set_prop_cycle(color=[cmap(1.*i/NUM_COLORS) for i in range(NUM_COLORS)])
            group = samples.groupby('channel')
            for i,(ch_id, ch_df) in enumerate(group):
                if (i >= 36):
                    break
                sorted_df = (ch_df
                    .groupby(['channel', 'time'], as_index=False)
                    .agg(adc=('adc', 'median'), std=('adc', 'std')))
                ax_p.errorbar(sorted_df['time'], sorted_df['adc'], 
                            yerr=sorted_df['std'], fmt='o', markersize=4, label=f'ch {i}')
            ax_p.legend(fontsize=12, ncols=3)
            ax_p.set_title(f'$V_{{ref}} = {V0} mV, V_{{oc}} = {x} mV$', fontsize=24, y=1.003)
            ax_p.set_xlabel("Time [ns]", fontsize=18)
            ax_p.set_ylabel("Median ADC [a.u.]", fontsize=18)
            ax_p.grid(which='major', axis='y')
            ax_p.tick_params(axis='both', labelsize=12)
            fig_p.savefig(f"Time_adc_{x}mv.png", dpi=400, bbox_inches='tight')
            plt.close()

    Aprimes = []
    stdevs = []
    if (args.amplitude == 'all'):
        Aprimes = [[], [], [], []]
        stdevs = [[], [], [], []]
        A0s = []
    A0 = 0
    err = 0
    for x, samples, run_params in zip(xl, samples_collection, run_params_collection):
        samples = samples[samples['nr channels'] == 1] # Remove data we don't need

        # Make new dataframe with median and std
        sorted_df = (samples
                    .groupby(['channel', 'time'], as_index=False)
                    .agg(adc=('adc', 'median'), std=('adc', 'std')))

        ch_df = sorted_df[sorted_df['channel'] == ref_ch]

        x_mean = ch_df[ch_df['adc'] == ch_df['adc'].max()]['time'].iloc[0]

        # Limit data range
        #ch_lim = ch_df[(ch_df['adc'] > ch_df['adc'].median()) & (ch_df['time'] >= (x_mean - 30)) & (ch_df['time'] <= (x_mean + 30))]
        ch_lim = ch_df[(ch_df['time'] >= (x_mean-50)) & (ch_df['time'] <= (x_mean+50))]
        #ch_lim['adc'] = ch_lim['adc'].transform(lambda x: x - ch_df['adc'].median())

        p0 = ([ch_lim['adc'].median(),
             ch_lim['adc'].max() - ch_lim['adc'].median(),
             ch_lim[(ch_lim['adc'] == ch_lim['adc'].max())]['time'].iloc[0],
             25,
        ])

        parameters, pcov = curve_fit(gauss,
                                  ch_lim['time'],
                                  ch_lim['adc'],
                                  p0=p0
        )
        H, A, x0, sigma = parameters
        uncert = np.diag(pcov)
        xl_fit = np.linspace(min(ch_lim['time']), max(ch_lim['time']), 1000)
        fit_y = gauss(xl_fit, H, A, x0, sigma)
        #fit_y = skew_norm(xl_fit, H, A, x0, sigma, -5e-3)
        FWHM = 2*np.sqrt(2*np.log(2))*sigma
        FWHM_err = 2*np.sqrt(2*np.log(2))*uncert[3]

        #params = stats.crystalball.fit(ch_lim['adc'].to_numpy())
        #fit_y = stats.crystalball.pdf(xl_fit, *params)

        #plt.plot(xl_fit, fit_y, label='Fit')
        #plt.errorbar(ch_lim['time'], ch_lim['adc'], 
        #             yerr=ch_lim['std'], fmt='o', label='Data'
        #)
        #plt.legend()
        #plt.show()
        #plt.clf()
        
        # Need to find the pulsed channels. Look for noisy channels
        ch_maxs = [ch['adc'].max() for _, ch in sorted_df.groupby('channel')]

        # Get the average dip. Iterate over quiet channels
        #dips = [df['adc'].min() for _, df in sorted_df.groupby('channel')]
        #dips_errs = [df[df['adc'] == df['adc'].min()]['std'].iloc[0] for _, df in sorted_df.groupby('channel')]
        dips = []
        dips_errs = []

        time_max = ch_df[ch_df['adc'] == ch_df['adc'].max()]['time'].iloc[0] 

        if (args.plot_pedestals == True):
            chs = sorted_df.groupby('channel')
            fig, ax = plt.subplots(1,1)
            for ch_id, ch in chs:
                if (ch_id >= 36):
                    break
                if (ch['adc'].max() < np.median(ch_maxs) + 50):
                    dips.append(ch[ch['time'] == time_max]['adc'].iloc[0])
                    dips_errs.append(ch[ch['time'] == time_max]['std'].iloc[0])
                    #ax.errorbar(ch['time'], ch['adc'], yerr=ch['std'], label=f"ch {ch_id})")
                    ax.plot(ch['time'], ch['adc'], label=f"ch {ch_id}")
            plt.legend(fontsize=12, ncols=3)
            ax.set_xlabel('Time [ns]', fontsize=18)
            ax.set_ylabel('Median ADC [a.u.]', fontsize=18)
            ax.grid(which='major', axis='y')
            ax.tick_params(axis='both', labelsize=12)
            ax.set_title(f'$V_{{ref}} = {V0} mV, V_{{oc}} = {x} mV$', fontsize=24, y=1.003)
            fig.savefig(f'Pedestal_quiet_{x}.png', dpi=400, bbox_inches='tight')
            plt.close()

        mean_dip = np.median(dips)
        mean_dip_err = np.median(dips_errs)

        max_adc_std = ch_df[ch_df['adc'] == ch_df['adc'].max()]['std'].iloc[0]
        max_fit = fit_y.max()
        max_fit_err = uncert[1]
        rel_pedestal = ch_df['adc'].max() - ch_df['adc'].median()
        rel_pedestal_err = (ch_df[ch_df['adc'] == ch_df['adc'].max()]['std'].iloc[0] 
                            + ch_df['std'].median())
        rel_dip = ch_df['adc'].max() - mean_dip
        rel_dip_err = (ch_df[ch_df['adc'] == ch_df['adc'].max()]['std'].iloc[0]
                       + mean_dip_err)
        if (args.amplitude == 'max_adc'):
            Ai = ch_df['adc'].max()
            err = max_adc_std
        elif (args.amplitude == 'max_adc_fit'):
            Ai = fit_y.max()
            err = max_fit_err
        elif (args.amplitude == 'rel_pedestal'):
            Ai = rel_pedestal
            err = rel_pedestal_err
        elif (args.amplitude == 'rel_dip'):
            Ai = rel_dip
            err = rel_dip_err
        elif (args.amplitude == 'FWHM'):
            Ai = FWHM
            err = FWHM_err
        elif (args.amplitude == 'all'):
            Aprimes[0].append(max_fit)
            stdevs[0].append(max_fit_err)
            Aprimes[1].append(rel_pedestal)
            stdevs[1].append(rel_pedestal_err)
            Aprimes[2].append(rel_dip)
            stdevs[2].append(rel_dip_err)
            Aprimes[3].append(FWHM)
            stdevs[3].append(FWHM_err)
            if (x == 0):
                A0s.append(max_fit)
                A0s.append(rel_pedestal)
                A0s.append(rel_dip)
                A0s.append(FWHM)
            continue
        Aprimes.append(Ai)
        stdevs.append(err)
        if (x == 0):
            A0 = Ai
    
    xl_fit = np.linspace(min(xl), max(xl), 1000)

    if (args.amplitude == 'all'):
        fig, ax = plt.subplots(2, 2, 
                               sharex=True, gridspec_kw={"hspace": 0.1})
        ax[1][0].set_xlabel('Voltage [mV]', fontsize=18)
        ax[1][1].set_xlabel('Voltage [mV]', fontsize=18)
        ax[0][0].set_ylabel('Absolute peak ADC of fit [a.u.]', fontsize=18)
        ax[0][1].set_ylabel('Pedestal-subtracted peak ADC [a.u.]', fontsize=18)
        ax[1][0].set_ylabel('Peak-to-dip ADC [a.u.]', fontsize=18)
        ax[1][1].set_ylabel('FWHM [ns]', fontsize=18)

        fits = []
        xls = []
        for i in range(2):
            for j in range(2):
                ax[i][j].errorbar(xl, Aprimes[2*i+j], yerr=stdevs[2*i+j], 
                                  fmt='o', label='Data', zorder=0)
                ax[i][j].scatter(0, A0s[2*i+j], color='orange', 
                                 label=f'$A_0 = {A0s[2*i+j]:.2f}$', zorder=5)
                popt, _ = curve_fit(linear, xl, Aprimes[2*i+j])
                ax[i][j].plot(xl_fit, linear(xl_fit, *popt), lw=1, 
                              label=f'Fit: {popt[0]:.5f}x + {popt[1]:.2f}', 
                              zorder=10)
                #ax[i][j].legend(fontsize=12)
                ax[i][j].grid(which='major', axis='y')
                ax[i][j].tick_params(axis='both', labelsize=12)
                handles, labels = ax[i][j].get_legend_handles_labels()
                # sort both labels and handles by labels
                labels, handles = zip(*sorted(zip(labels, handles), key=lambda t: t[0]))
                ax[i][j].legend(handles, labels, fontsize=16)

        fig.savefig('attenuation_of_signal_all_amplitudes.png', dpi=400, bbox_inches="tight")

    else:
        # Make a fit of data points
        popt, _ = curve_fit(linear, xl, Aprimes)
        slope, intercept = popt[0], popt[1]

        fig, ax = plt.subplots(1, 1)
        ax.set_xlabel('Voltage [mV]', fontsize=18)
        if (args.amplitude == 'FWHM'):
            ax.set_ylabel('FWHM of ref pulse [ns]', fontsize=18)
        elif (args.amplitude == 'rel_pedestal'):
            ax.set_ylabel('Pedestal-subtracted peak ADC [a.u.]', fontsize=18)
        elif (args.amplitude == 'rel_dip'):
            ax.set_ylabel('Peak-to-dip ADC [a.u.]', fontsize=18)
        elif (args.amplitude == 'max_adc_fit'):
            ax.set_ylabel('Absolute peak ADC of fit [a.u.]', fontsize=18)
        else:
            ax.set_ylabel('Absolute peak ADC [a.u.]', fontsize=18)
        ax.grid(which='major', axis='y')

        ax.errorbar(xl, Aprimes, fmt='o', yerr=stdevs, label='Data', zorder=0)
        ax.plot(xl_fit, linear(xl_fit, slope, intercept), lw=1, label=f'Fit: {slope:.5f}x + {intercept:.2f}', zorder=10)
        ax.scatter(0, A0, color='orange', label=f'$A_0 = {A0}$', zorder=5)

        handles, labels = ax.get_legend_handles_labels()
        labels, handles = zip(*sorted(zip(labels, handles), key=lambda t: t[0]))
        ax.legend(handles, labels, fontsize=14)

        ax.grid(which='major', axis='y')
        ax.tick_params(axis='both', labelsize=12)

        plt.savefig('attenuation_of_signal_in_voltage.png', dpi=400, bbox_inches='tight')

elif (args.plot_parameter == plot_parameters[1]):
    for csv in args.time_scan:
        xl.append(int(str(csv).split('_')[-1].split('n')[0]))
        samples, run_params = read_pflib_csv(csv)
        samples_collection.append(samples)
        run_params_collection.append(run_params)

    link = 0
    ref_ch = 16

    Aprimes = []
    A0 = 0
    for x, samples, run_params in zip(xl, samples_collection, run_params_collection):
        samples['adc'] = (
            samples
            .groupby(['channel', 'time'])['adc']
            .transform('median')
        )
        Ai = samples[samples['channel'] == ref_ch]['adc'].max()
        Aprimes.append(Ai)
        if (x == 0):
            A0 = Ai 

    fig, ax = plt.subplots(1, 1)
    ax.set_xlabel(r'Phase difference [ns]')
    ax.set_ylabel('Max ADC of reference pulse')
    ax.grid(which='major', axis='y') 

    ax.scatter(xl, Aprimes, s=15, label='data')
    ax.legend()

    plt.savefig('attenuation_of_signal_in_time.png', dpi=400, bbox_inches='tight')
