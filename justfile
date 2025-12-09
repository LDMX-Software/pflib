help_message := "shared recipes for pflib development

Use the environment variable PFLIB_CONTAINER to signal
you are not on a prepared environment and want to
mimic a DAQ server within a container
else and wish to mimic the DAQ server with a container.

  # without PFLIB_CONTAINER, we run with denv
  just -n configure
  cmake -B build -S .

  # with PFLIB_CONTAINER, we assume the environment
  # is already constructed properly
  export PFLIB_CONTAINER=1
  just -n configure
  denv cmake -B build -S .

RECIPES:
"

_default:
    @just --list --unsorted --justfile {{justfile()}} --list-heading "{{ help_message }}"

env_cmd_prefix := if env("PFLIB_CONTAINER","0") == "1" { "denv " } else { "" }

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
    {{env_cmd_prefix}}'PYTHONPATH=${PWD}/build LD_LIBRARY_PATH=${PWD}/build python3 test/decoding.py'

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
