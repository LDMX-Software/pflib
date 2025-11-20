set -o nounset
set -o errexit

cmake -B build -S . -DCMAKE_INSTALL_PREFIX=${PREFIX}
cmake --build build --target install -- -j ${CPU_COUNT}

# filthy hack to get pypflib into the PYTHONPATH without having to use pip
pyrogue_dir=$(pip show rogue | grep Location | cut -f 2 -d ':' | sed "s|${BUILD_PREFIX}|${PREFIX}|")
echo ${pyrogue_dir}
mkdir -p ${pyrogue_dir}
ln -s ${PREFIX}/lib/pypflib.so ${pyrogue_dir}
