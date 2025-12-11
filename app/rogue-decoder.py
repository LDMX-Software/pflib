#!/usr/bin/env python3
import pyrogue.utilities.fileio
import sys
import numpy as np
import pypflib
import argparse
from pathlib import Path

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('input', help='input file to Hcal/Ecal data from')
    parser.add_argument('--n-links', help='number of active links connected to the ECON-D', type=int, default=2)
    parser.add_argument('--output', '-o', help='output CSV to write to')
    parser.add_argument('--log-level', '-l', help='log level to print out while decoding',
            choices=list(pypflib.logging.level.names.keys()), default='info')
    parser.add_argument('--nevent', '-n', help='maximum number of events to decode', type=int)
    parser.add_argument('--contrib', '-c', choices=['ecal','hcal'], help='contributor to focus on, without this dump all data from subsystem 5 which could be both Ecal and Hcal')
    args = parser.parse_args()

    if args.output is None:
        args.output = str(Path(args.input).with_suffix('.csv'))

    pypflib.logging.open(True)
    pypflib.logging.set(getattr(pypflib.logging.level, args.log_level))

    contrib_id = None
    if args.contrib is None:
        contrib_id = 0 # undefined
    elif args.contrib == 'hcal':
        contrib_id = 1
    elif args.contrib == 'ecal':
        contrib_id = 2
    
    # number of links/channels enabled in the ECON-D
    #   for HcalBackplane -> 2 channels for 1 ROC
    #   for the EcalSMM -> complicated, depends on which layer
    #                      because some of the ROCs are not accessible
    ep = pypflib.packing.MultiSampleECONDEventPacket(args.n_links)
    
    out = pypflib.packing.OFStream()
    out.open(args.output)
    ep.header_to_csv(out)

    count = 0
    
    # do NOT provide configChan = 255 because it does not provide a Loader to yaml.load
    # which fails in newer versions of Python :tada:
    with pyrogue.utilities.fileio.FileReader(files=sys.argv[1]) as fd:
        for header, data in fd.records():
            if data[-1] == 0x0a or header.channel != 0:
                # magic byte signaling config packet
                continue
    
            # a byte from the Rogue header signals the subsystem
            # from slaclab/ldmx-firmware/common/tdaq/python/ldmx_tdaq/_Constants.py
            subsys = data[1]
            # both Hcal and Ecal are using the EcalSubsystem in RunControl
            # meaning they are both using the ID 5 right now
            if (subsys != 5):
                continue

            # we are planning to use the contributor_id to separate Ecal and Hcal
            contrib = data[2]
            if contrib_id != 0 and contrib != contrib_id:
                continue

            # check on event limit
            if args.nevent is not None and count >= args.nevent:
                break
    
            # need to convert the data into a std::vector<uint32_t>
            # data here is a np.ndarray('int8') so we re-view
            # it in our words and skip the first four words which
            # contain the Rogue-inserted EventHeader
            v = pypflib.packing.WordVector()
            words = data.view('uint32')
            v.extend(words[4:].tolist())
    
            # this is the call that attempts to decode the word vector
            ep.from_word_vector(v)
    
            # write out decoded data to the CSV file
            ep.to_csv(out)

            count += 1
    
    
    pypflib.logging.close()

if __name__ == '__main__':
    main()
