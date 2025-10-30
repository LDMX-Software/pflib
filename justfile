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
    denv init ghcr.io/ldmx-software/pflib-env:latest

# configure pflib build
configure: _cmake

# compile pflib
build: _build

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
decode *args:
    denv ./build/pfdecoder {{ args }}

# open the test menu
test-menu:
    denv ./build/test-menu

# build for the zcu using docker+qemu to emulate architecture
zcu-build:
    docker run \
      --platform linux/arm64 \
      --rm \
      --volume ${PWD}:${PWD} \
      --volume /etc/group:/etc/group \
      --volume /etc/passwd:/etc/passwd \
      --workdir ${PWD} \
      --user $(id -u "${USER}"):$(id -g "${USER}") \
      ghcr.io/ldmx-software/pflib-env:sha-8244ecf \
      sh -c 'cmake -B build -S . && cmake --build build -- -j'

