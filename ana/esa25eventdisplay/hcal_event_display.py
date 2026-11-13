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
print(timestamps)

#bunchCounts =[t & 0xFF for t in timestamps]
#pulseId =[t >> 8 for t in timestamps]



#events = sorted(list(set(samples['event'])))
#print(events)

event_i = 0

for timestamp in timestamps:
    if int(args.n_events) != -1 and event_i >= int(args.n_events):
        print("Quitting, exceeded n_events")
        break

    print("Plotting event_i = " + str(event_i))

    fig, axs = plt.subplots(4,6, figsize=(12,10))
    
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
                    #row_right = event[event["timestamp"] & 0xFF == bunchCount]
                    row_right = event[event["i_sample"] == i]
                    print(row_right)
                    row_right = row_right[row_right["channel"] == str(ch_right)]
                    print(row_right)
                    row_right = row_right[row_right["i_link"] == ilink_right]
                    print(row_right)
                    print(timestamp)
                    print(ilink_right)
                    print(ch_right)
                    adc_right.append(int(row_right['adc']))

                    #row_left = samples[samples["timestamp"] & 0xFF == bunchCount]
                    row_left = event[event["i_sample"] == i]
                    row_left = row_left[row_left["channel"] == str(ch_left)]
                    row_left = row_left[row_left["i_link"] == ilink_left]
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


    event_i += 1
