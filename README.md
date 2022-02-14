# pflib

The Polarfire Libary (pflib) encapsulates the higher-level functionality for communicating with the Polarfire FPGA on the HCAL and ECAL front ends.  The library is separated from the rest of the development as it is also used in a standalone implementation which does not depend on the COB/Rogue.

## YAML->Register "Compilation"
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

### Rogue 5.9.3
 - [Documentation](https://slaclab.github.io/rogue/index.html)
 - [Installng Rogue with Anaconda](https://slaclab.github.io/rogue/installing/anaconda.html)
 - [Installing Rogue on Archlinux](https://slaclab.github.io/rogue/installing/build.html#archlinux)

### uHal
 - [Installation Docs](https://ipbus.web.cern.ch/doc/user/html/software/installation.html)
 - [General uHAL Docs](https://ipbus.web.cern.ch/doc/user/html/software/index.html)

## Other Dependencies
Besides these two "larger" dependencies, we simply use some C++14 as well as the GNU readline library for the `pftool`.
This effectively restricts us to relatively new Linux systems; however, we haven't tested this library on are large set of potentional options.
