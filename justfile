help_message := "shared recipes for pflib development

We use 'denv' if there is a denv workspace in the pflib
directory.

  # without pflib/.denv
  just -n configure
  cmake -B build -S .

  # with pflib/.denv
  just -n configure
  denv cmake -B build -S .

Use 'just init <host>' to start using denv if desired.

RECIPES:
"

_default:
    @just --list --unsorted --justfile {{justfile()}} --list-heading "{{ help_message }}"

denv_exists := path_exists(justfile_directory() / ".denv")
env_cmd_prefix := if denv_exists == "true" { "denv " } else { "" }

_cmake *CONFIG:
    {{env_cmd_prefix}}cmake -B build -S . {{ CONFIG }}

_build *ARGS:
    {{env_cmd_prefix}}cmake --build build -- {{ ARGS }}

_test *ARGS:
    cd build && {{env_cmd_prefix}}./test-pflib {{ ARGS }}

# init a local denv for development ("zcu" or "bittware-host")
init host:
    denv init ghcr.io/ldmx-software/pflib-env:{{host}}-latest

# configure pflib build
configure: _cmake

# compile pflib
build: _build

default_install_dir := justfile_directory() / "install"

# install pflib to passed location
install dir=default_install_dir: (_cmake "-DCMAKE_INSTALL_PREFIX="+dir)
    {{env_cmd_prefix}}cmake --build build --target install

# alias for configure and then build
compile: configure build

# run Boost.Test executable
test: _test

# print all messages during testing
test-log-all: (_test "-l all")

# run econd decoding test
test-econd: (_test "-t \"decoding/econd*\"")

# format the C++ with Google style guide
format-cpp:
    git ls-tree -r --name-only HEAD | egrep '\.(h|cxx)$' | xargs denv clang-format -i --style=Google

# remove build directory
clean:
    rm -r build

# dump 32-bit words per line
[no-cd]
hexdump *args:
    hexdump -v -e '1/4 "%08x" "\n"' {{ args }}
   
# run the decoder
pfdecoder *args:
    {{env_cmd_prefix}}./build/pfdecoder {{ args }}

# run the econd-decoder
econd-decoder *args:
    {{env_cmd_prefix}}./build/econd-decoder {{ args }}

# open the test menu
test-menu:
    cd build && {{env_cmd_prefix}}make test-menu
    {{env_cmd_prefix}}./build/test-menu

# test decoding in python bindings
test-py-decoding:
    {{env_cmd_prefix}} PYTHONPATH=${PWD}/build LD_LIBRARY_PATH=${PWD}/build python3 test/decoding.py

# py-rogue decode
rogue-decode *args:
    {{env_cmd_prefix}}'PYTHONPATH=${PYTHONPATH}:${PWD}/build python3 ana/rogue-read.py {{args}}'

# build the conda package on the DAQ server
conda-package:
    #!/bin/bash
    set -o nounset
    set -o errexit
    source /u1/ldmx/miniforge3/etc/profile.d/conda.sh
    conda activate base
    conda-build env/conda --output-folder /u1/ldmx/pflib-conda-channel --channel conda-forge --channel tidair-tag
    conda index /u1/ldmx/pflib-conda-channel
