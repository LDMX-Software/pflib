import pyrogue.utilities.fileio
import sys
import numpy as np
import pypflib
print(pypflib.version.git_describe())
pypflib.logging.open(True)
#pypflib.logging.set(pypflib.logging.level.info)

ep = pypflib.packing.MultiSampleECONDEventPacket(1)

with pyrogue.utilities.fileio.FileReader(files=sys.argv[1], configChan = 255) as fd:
    for header, data in fd.records():
        if data[-1] == 0x0a:
            # fucking magic byte signaling config packet
            continue

        v = pypflib.packing.WordVector()
        words = data.view('uint32')
        v.extend(words[4:].tolist())
        ep.from_word_vector(v)
        print(ep.n_samples)
        print(ep.i_soi)
        print(ep.soi().channel(0,10).adc)
        print([ep.sample(i).channel(0,10).adc for i in range(ep.n_samples)])

pypflib.logging.close()
