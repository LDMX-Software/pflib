_default:
    @just --list --unsorted

_cmake *CONFIG:
    denv cmake -B build -S . {{ CONFIG }}

_build *ARGS:
    denv cmake --build build -- {{ ARGS }}

_test *ARGS:
    cd build && denv ./test-pflib {{ ARGS }}

# init a local denv for development
init:
    #!/bin/sh
    set -o errexit
    set -o nounset
    if command -v podman &> /dev/null; then
      podman build env/ -f env/Containerfile -t pflib-env:latest
    elif command -v docker &> /dev/null; then
      docker build env/ -f env/Containerfile -t pflib-env:latest
    else
      echo "Unable to build pflib-env locally without podman or docker."
      exit 1
    fi
    denv init pflib-env:latest

# configure pflib build
configure: _cmake

# compile pflib
build: _build

# run Boost.Test executable
test: _test

# print all messages during testing
test-log-all: (_test "-l all")

# remove build directory
clean:
    rm -r build

# dump 32-bit words per line
hexdump *args:
    hexdump -v -e '1/4 "%08x" "\n"' {{ args }}
   
# run the decoder
decode *args:
    denv ./build/pfdecoder {{ args }}
