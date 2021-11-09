# pflib

The Polarfire Libary (pflib) encapsulates the higher-level functionality for communicating with the Polarfire FPGA on the HCAL and ECAL front ends.  The library is separated from the rest of the development as it is also used in a standalone implementation which does not depend on the COB/Rogue.

## Python Bindings
This library has its 'lib' prefix removed so that _if_ the directory the built library ends up in is findable by python,
then the user can
```
import pflib_python as pflib
```
inside a python shell or script.

There are many ways for the library to be "findable by python", here are a few:
- Add the directory to `LD_LIBRARY_PATH`
- Add the directory to `PYTHONPATH`
- Be in the directory that `pflib_python.so` is in
