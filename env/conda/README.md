# conda recipe for pflib

Following notes for [rogue's conda-recipe](https://github.com/slaclab/rogue/tree/main/conda-recipe).

1. Setup conda
```
# once per machine, install conda and conda-build
wget https://github.com/conda-forge/miniforge/releases/latest/download/Miniforge3-Linux-x86_64.sh
bash Miniforge3-Linux-x86_64.sh
source ~/miniforge3/etc/profile.d/conda.sh
conda config --set always_yes yes
conda config --set channel_priority strict
conda install -n base conda-libmamba-solver
conda config --set solver libmamba
conda install conda-build
conda update -q conda conda-build
conda update --all
```

2. Build the conda package
```
conda-build \
  env/conda \
  --output-folder conda-bld \
  --channel conda-forge \
  --channel tidair-tag
conda index conda-bld/
```
- :warning: The build script uses the `conda-build` directory to hold the pflib build. The build will fail if that directory exists.
- I've summarized the `source` stuff above and these commands into the `conda-package` just recipe

3. Use conda package
```
conda install -n my-env -c file:///full/path/to/conda-bld pflib
```
Add the `conda-bld` directory (where-ever it is) as a "channel" and install the `pflib` package into a conda environment named `my-env`.


