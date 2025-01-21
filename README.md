# pflib

The Pretty Fine Libary (pflib) encapsulates library functionality for communicating with the HGCROC on the HCAL and ECAL front ends and provides a simple user frontend for use with the ZCU102-based readouts (both direct readout and via lpGBT).

The library is separated from the rest of the development for efficiency at this time.

## Quickstart
We assume you have the YAML parser yaml-cpp and at least one of the communication methods below installed.
```
git clone https://github.com/LDMX-Software/pflib.git
cd pflib
git tag # see tags, pick latest
git checkout v1.11 # or whatever is latest
cmake -B build -S . # may need to provide paths to dependencies
cd build
make
./pftool -h # print help
```

## YAML->Register "Compilation"
The more detailed documentation for this compilation can be found in the pflib/Compile.h file.
The \hgcrocmanual is also a decent reference since that is where most of the parameter names are pulled from.
The documentation of \dataformats is helpful for learning how to decode the raw files.

Translating YAML files containing named settings and their values into actual register values that can be written to the chip requires a (you guessed it) YAML parser. 
I have chosen to use [yaml-cpp](https://github.com/jbeder/yaml-cpp) which is very light and easy to install.

**Note**: Make sure to build yaml-cpp with the `YAML_BUILD_SHARED_LIBS=ON` cmake option so that we can link it properly to our code.

An outline of the terminal commands necessary to install yaml-cpp is given below.

```
mkdir yaml-cpp
wget -q -O - \
  https://github.com/jbeder/yaml-cpp/archive/refs/tags/yaml-cpp-0.7.0.tar.gz |\
  tar xz --strip-components=1 --directory yaml-cpp
cd yaml-cpp
cmake -B build -S . -DYAML_BUILD_SHARED_LIBS=ON
cd build
make install
```

## Communication Method

This software can communicate with the HGCROC and other front-end items using several different methods.

### Direct I2C
 This mode of operation is used for the HCAL HGCROC board tester.  It has no external dependencies as it uses /dev/i2c character
 devices for communication.
 
## Other Dependencies
 Besides these two "larger" dependencies, we simply use some C++14 as well as the GNU readline library for the `pftool`.
 This effectively restricts us to relatively new Linux systems; we haven't tested this library on a large set of potentional options.

### UMN Setup
Layer | Description
---|---
OS | CentOS Stream 8
gcc | 8.5.0
cmake | 3.20.2
yaml-cpp | 0.7.0
readline | 7.0

### Lund Setup
Layer | Description
---|---
OS | openSUSE Leap 15.3
gcc | 7.5.0
cmake | 3.17.0
yaml-cpp | 0.7.0
readline | 7.0
Rogue | NA
uHAL | 2.8.1

## Directory Structure
- config : helpful HGC ROC YAML parameter settings to load onto chips
- docs : documentation generation and helpful PDF manuals
- include : header files
- src : implementation files
- tool : executables and supporting source files
- uhal : XML files used for configuring uHAL connection

