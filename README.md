# pflib

The Polarfire Libary (pflib) encapsulates the higher-level functionality for communicating with the Polarfire FPGA on the HCAL and ECAL front ends.  The library is separated from the rest of the development as it is also used in a standalone implementation which does not depend on the COB/Rogue.

## YAML->Register "Compilation"
Translating YAML files containing named settings and their values into actual register values that can be written to the chip requires a (you guessed it) YAML parser. I have chosen to use [yaml-cpp](https://github.com/jbeder/yaml-cpp) which is very light and easy to install.

