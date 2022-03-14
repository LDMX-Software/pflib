# pflib

The Polarfire Libary (pflib) encapsulates the higher-level functionality for communicating with the Polarfire FPGA on the HCAL and ECAL front ends.  The library is separated from the rest of the development as it is also used in a standalone implementation which does not depend on the COB/Rogue.

## Quickstart
We assume you have the YAML parser yaml-cpp and at least one of the communication methods below installed.
```
git clone https://github.com/LDMX-Software/pflib.git
cd pflib
git tag # see tags, pick latest
git checkout v1.8 # or whatever is latest
cmake -B build -S . # may need to provide paths to dependencies
cd build
make
./pftool <ip-of-polarfire>
```

## YAML->Register "Compilation"
The more detailed documentation for this compilation can be found in the pflib/Compile.h file.
The \hgcrocmanual is also a decent reference since that is where most of the parameter names are pulled from.

Translating YAML files containing named settings and their values into actual register values that can be written to the chip requires a (you guessed it) YAML parser. I have chosen to use [yaml-cpp](https://github.com/jbeder/yaml-cpp) which is very light and easy to install.

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
This software can communicate with the polarfire using two different methods. At least one is required to be available in order for the library to be built.

### Rogue
 - [Documentation](https://slaclab.github.io/rogue/index.html)
 - [Installng Rogue with Anaconda](https://slaclab.github.io/rogue/installing/anaconda.html)
 - [Installing Rogue on Archlinux](https://slaclab.github.io/rogue/installing/build.html#archlinux)

### uHal
 - [Installation Docs](https://ipbus.web.cern.ch/doc/user/html/software/installation.html)
 - [General uHAL Docs](https://ipbus.web.cern.ch/doc/user/html/software/index.html)

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
Rogue | 5.11.1
uHAL | 2.8.1

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
