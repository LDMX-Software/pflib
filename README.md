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

### on DAQ server at SLAC
On this machine, `conda` is being used to standardize the environment.
```
source /u1/ldmx/miniconda3/etc/profile.d/conda.sh
conda activate ldmx-env-base
```
This activation needs to be done **before** you configure (`cmake`)
or build (`make`) pflib.
Notice that this environment is **not** `ldmx-env`. We cannot use
the environment that the Rogue run control uses because that environment
already has a build of `pflib` in it that will conflict with your
developments.

## Usage with Rogue Run Control
Python bindings for specific functions have been implemented here,
so you can import them as long as you install pflib and update
your environment accordingly.
```
cmake -DCMAKE_INSTALL_PREFIX=path/to/install -B build -S .
cd build
make install
LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:path/to/install/lib
PYTHONPATH=${PYTHONPATH}:path/to/install/lib
```
- `PYTHONPATH` tells Python where it can find `pypflib`
- `LD_LIBRARY_PATH` allows `pypflib` to link to the other pflib libraries

If running Rogue Run Control within a conda environment,
there is a [conda recipe](env/conda) for pflib that can be used
to build a local conda package that can be installed into the conda
environment

## Directory Structure
- ana : shared Python snippets for useful analyses (mostly making plots)
- app : executables and supporting source files for executables
- config : helpful/example YAML files
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
