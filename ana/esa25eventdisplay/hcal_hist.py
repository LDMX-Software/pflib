import pandas as pd
import numpy as np
import hist
import matplotlib.pyplot as plt
from pathlib import Path
import argparse
import os

parser = argparse.ArgumentParser()
parser.add_argument('data', type=Path, help='decoded CSV file from rogue-decoder')
parser.add_argument('outputdir', type=Path, help='plot output directory')
parser.add_argument('runnumber', help='run number')
parser.add_argument('nsamples', help='number of samples')
parser.add_argument('n_events', help='number of events to be plotted')
args = parser.parse_args()

mapping_file = open("hcal_ch_mapping.csv","r")

if not os.path.isdir(args.outputdir):
    raise Exception("Output directory does not exist")

chmap = {}

for line in mapping_file:
    line = line[:-1]
    s = line.split(",")
    if s[0] == "iLink":
        continue
    #print(line)
    chmap[(int(s[3]),int(s[4]), int(s[5]))] = (int(s[0]),int(s[1]))


samples = pd.read_csv(args.data)

timestamps = list(set(samples['timestamp']))

hists_x = {}
hists_y = {}
maxadc = {}
for k in chmap.keys():
    hists_x[k] = []
    hists_y[k] = []
    maxadc[k] = []


event_i = 0
for timestamp in timestamps:
    if int(args.n_events) != -1 and event_i >= int(args.n_events):
        print("Quitting, exceeded n_events")
        break

    print("Plotting event_i = " + str(event_i))
    
    x = range(0,8)

    event = samples[samples["timestamp"] == timestamp]

    for layer in range(1,7):
        for bar in range(0,4):
            ilink_right,ch_right = chmap[(layer,bar,0)]
            if ilink_right == 1:
                ch_right -= 36
            ilink_left,ch_left  = chmap[(layer,bar,1)] 
            if ilink_left == 1:
                ch_left -= 36

            adc_left = []
            adc_right = []

            for i in range(0, int(args.nsamples)):
                    row_right = event[event["i_sample"] == i]
                    row_right = row_right[row_right["channel"] == str(ch_right)]
                    row_right = row_right[row_right["i_link"] == ilink_right]
                    adc = int(row_right['adc'])
                    adc_right.append(adc)
                    hists_x[(layer,bar,0)].append(i)
                    hists_y[(layer,bar,0)].append(adc)

                    row_left = event[event["i_sample"] == i]
                    row_left = row_left[row_left["channel"] == str(ch_left)]
                    row_left = row_left[row_left["i_link"] == ilink_left]
                    adc = int(row_left['adc'])
                    adc_left.append(adc)
                    hists_x[(layer,bar,1)].append(i)
                    hists_y[(layer,bar,1)].append(adc)

            maxadc[(layer,bar,0)].append(max(adc_right))
            maxadc[(layer,bar,1)].append(max(adc_left))


            """
            axs[3-bar, layer-1].plot(x, adc_left, color="red", marker="*", label="Left")
            axs[3-bar, layer-1].plot(x, adc_right, color="blue", marker="*", label="Right")

            axs[3-bar, layer-1].set_xlim(0,int(args.nsamples)-1)
            axs[3-bar, layer-1].set_ylim(0,1023)
            if bar == 0:
                axs[3-bar, layer-1].set_xlabel("L"+str(layer))
            else:
                axs[3-bar,layer-1].set_xticklabels([])
            
            if layer == 1:
                axs[3-bar,layer-1].set_ylabel("Bar "+str(bar))
            else:
                axs[3-bar,layer-1].set_yticklabels([])

            if layer == 6 and bar == 3:
                axs[3-bar,layer-1].legend()
            """

    event_i += 1

fig, axs = plt.subplots(4,6, figsize=(12,10))
for layer in range(1,7):
    for bar in range(0,4):
            axs[3-bar, layer-1].hist2d(hists_x[(layer,bar,0)],hists_y[(layer,bar,0)])

            if bar == 0:
                axs[3-bar, layer-1].set_xlabel("L"+str(layer))
            if layer == 1:
                axs[3-bar,layer-1].set_ylabel("Bar "+str(bar))

            if layer == 6 and bar == 3:
                axs[3-bar,layer-1].legend()

fig.suptitle("Run " + args.runnumber + ", right SiPMs")
plt.tight_layout()
plt.savefig(str(args.outputdir)+"/hist2d_right_run_"+args.runnumber+".png",dpi=300)
plt.clf()
fig, axs = plt.subplots(4,6, figsize=(12,10))
for layer in range(1,7):
    for bar in range(0,4):
            axs[3-bar, layer-1].hist2d(hists_x[(layer,bar,1)],hists_y[(layer,bar,1)])

            if bar == 0:
                axs[3-bar, layer-1].set_xlabel("L"+str(layer))
            if layer == 1:
                axs[3-bar,layer-1].set_ylabel("Bar "+str(bar))
            if layer == 6 and bar == 3:
                axs[3-bar,layer-1].legend()

fig.suptitle("Run " + args.runnumber + ", left SiPMs")
plt.tight_layout()
plt.savefig(str(args.outputdir)+"/hist2d_left_run_"+args.runnumber+".png",dpi=300)
plt.clf()
fig, axs = plt.subplots(4,6, figsize=(12,10))
for layer in range(1,7):
    for bar in range(0,4):
            axs[3-bar, layer-1].hist(maxadc[(layer,bar,0)])

            if bar == 0:
                axs[3-bar, layer-1].set_xlabel("L"+str(layer))
            if layer == 1:
                axs[3-bar,layer-1].set_ylabel("Bar "+str(bar))

            if layer == 6 and bar == 3:
                axs[3-bar,layer-1].legend()

fig.suptitle("Run " + args.runnumber + ", maxADC, right SiPMs")
plt.tight_layout()
plt.savefig(str(args.outputdir)+"/maxadc_right_run_"+args.runnumber+".png",dpi=300)
plt.clf()
fig, axs = plt.subplots(4,6, figsize=(12,10))
for layer in range(1,7):
    for bar in range(0,4):
            axs[3-bar, layer-1].hist(maxadc[(layer,bar,1)])

            if bar == 0:
                axs[3-bar, layer-1].set_xlabel("L"+str(layer))
            if layer == 1:
                axs[3-bar,layer-1].set_ylabel("Bar "+str(bar))

            if layer == 6 and bar == 3:
                axs[3-bar,layer-1].legend()

fig.suptitle("Run " + args.runnumber + ", maxADC, left SiPMs")
plt.tight_layout()
plt.savefig(str(args.outputdir)+"/maxadc_left_run_"+args.runnumber+".png",dpi=300)

