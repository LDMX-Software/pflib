# pflib

The Pretty Fine Libary (pflib) encapsulates library functionality for communicating with the HGCROC on the HCAL and ECAL front ends and provides a simple user frontend for use with the ZCU102-based readouts (both direct readout and via lpGBT).

The library is separated from the rest of the development for efficiency at this time.

## Quickstart
```
# SSH to a ZCU connected to an HGCROC
git clone https://github.com/LDMX-Software/pflib.git
cd pflib
cmake -B build -S . # may need to provide paths to dependencies
cd build
make
./pftool -h # print help
```

## Directory Structure
- ana : shared Python snippets for useful analyses (mostly making plots)
- app : executables and supporting source files for executables
- config : helpful HGC ROC YAML parameter settings to load onto chips
- docs : documentation generation
- env : build context for a container image emulating the ZCU software environment
- include : header files of pflib library
- register_maps : files for mapping parameters to their registers on the chip
- src : implementation files of pflib library
- test : source files to build a testing executable with Boost.Test

## Maintainer

[@tomeichlersmith](https://github.com/tomeichlersmith)

## Contributors

<a href="https://github.com/LDMX-Software/pflib/graphs/contributors">
  <img src="https://contrib.rocks/image?repo=LDMX-Software/pflib&anon=1" />
</a>
