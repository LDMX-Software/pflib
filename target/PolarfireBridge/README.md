# PolarfireBridge
A pair of light rogue-based servers that receive wishbone commands from a Rogue source and send data from a DMA device to a Rogue destination.

## On DPM of Interest
```
# get this source code to the DPM somehow, e.g.
wget -O - -q https://github.com/LDMX-Software/pflib/archive/refs/tags/v2.0.tar.gz | tar -xz
cd pflib/target/PolarfireBridge
# build it
cmake -B build -S .
cd build
make
make install
systemctl enable --now ldmx-polarfire
systemctl enable --now ldmx-dma-read
```

## Development
Developing software on the DPM is somewhat tricky, one possible solution (if you are booting the DPM via NFS) 
is to hardlink this directory to some place on the DPM (e.g. a home directory) so that your developments are
propagated and tracked by git. Compiling will still need to be done on the DPM, but then editing can be done
on a more powerful remote machine.
