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

events = sorted(list(set(samples['event'])))
print(events)

event_i = 0

while(True):
    if int(args.n_events) != -1 and event_i >= int(args.n_events)*int(args.nsamples):
        print("Quitting, exceeded n_events")
        break
    if event_i >= max(events):
        print("Quitting, no more events in file")
        break

    print("Plotting event_i = " + str(event_i))

    fig, axs = plt.subplots(4,6, figsize=(12,10))
    
    x = range(0,8)

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

            for i in range(event_i, event_i+int(args.nsamples)):
                    row_right = samples[samples["event"] == i]
                    min_bx = min(row_right["bx"])
                    row_right = row_right[row_right["bx"] == min_bx]
                    row_right = row_right[row_right["channel"] == str(ch_right)]
                    row_right = row_right[row_right["i_link"] == ilink_right]
                    min_orbit = min(row_right["orbit"])
                    row_right = row_right[row_right["orbit"] == min_orbit]
                    adc_right.append(int(row_right['adc']))

                    row_left = samples[samples["event"] == i]
                    min_bx = min(row_left["bx"])
                    row_left = row_left[row_left["bx"] == min_bx]
                    row_left = row_left[row_left["channel"] == str(ch_left)]
                    row_left = row_left[row_left["i_link"] == ilink_left]
                    min_orbit = min(row_left["orbit"])
                    row_left = row_left[row_left["orbit"] == min_orbit]
                    adc_left.append(int(row_left['adc']))


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



    fig.suptitle("Run " + args.runnumber + " event " + str(event_i))
    plt.tight_layout()
    #plt.show()
    plt.savefig(str(args.outputdir)+"/run_"+args.runnumber+"_event_"+str(event_i)+".png",dpi=300)


    event_i += int(args.nsamples)

