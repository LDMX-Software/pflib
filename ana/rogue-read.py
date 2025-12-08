import pyrogue.utilities.fileio
import sys
import numpy as np
import pypflib
print(pypflib.version.git_describe())
pypflib.logging.open(True)
#pypflib.logging.set(pypflib.logging.level.trace)

# number of links/channels enabled in the ECON-D
#   for HcalBackplane -> 2 channels for 1 ROC
#   for the EcalSMM -> complicated, depends on which layer
#                      because some of the ROCs are not accessible
ep = pypflib.packing.MultiSampleECONDEventPacket(2)

# do NOT provide configChan = 255 because it does not provide a Loader to yaml.load
# which fails in newer versions of Python :tada:
with pyrogue.utilities.fileio.FileReader(files=sys.argv[1]) as fd:
    for header, data in fd.records():
        if data[-1] == 0x0a or header.channel != 0:
            # fucking magic byte signaling config packet
            continue

        # need to convert the data into a std::vector<uint32_t>
        # data here is a np.ndarray('int8') so we re-view
        # it in our words and skip the first four words which
        # contain the Rogue-inserted EventHeader
        v = pypflib.packing.WordVector()
        words = data.view('uint32')
        v.extend(words[4:].tolist())

        # this is the call that attempts to decode the word vector
        ep.from_word_vector(v)

        # access the decoded data
        print(ep.n_samples)
        print(ep.i_soi)
        print(ep.soi().channel(0,10).adc)
        print([ep.sample(i).channel(0,10).adc for i in range(ep.n_samples)])

pypflib.logging.close()
