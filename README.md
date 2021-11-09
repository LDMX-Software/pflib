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
- Add the directory to `PYTHONPATH`
- Be in the directory that `pflib_python.so` is in

After you are able to get the library compiled and installed,
you can start interacting with the polarfire in a python terminal.

```python
>>> import pflib_python as pflib
>>> wbi = pflib.RogueWishboneInterface('<host>',<port>)
>>> i2c = pflib.I2C(wbi)
>>> roc = pflib.ROC(i2c, 0)
```

This is simply nice for experimentation. Common tasks can be coded into the library as they are discovered.
