# Generating docs
The documentation is generated with [Doxygen](https://www.doxygen.nl/index.html)
using the fancy [doxygen-awesome](https://github.com/jothepro/doxygen-awesome-css) theme
In order to obtain the same styling as the online documentation,
you must make sure the doxygen-awesome submodule is downloaded. You can do this with
```
git submodule update --init
```

You can generate a local copy of the documentation after installing doxygen.
We assume that doxygen is run from the root directory of the fire repository.
```
doxygen docs/doxyfile
```
The output of the documentation then is put into the `docs/html` folder.
