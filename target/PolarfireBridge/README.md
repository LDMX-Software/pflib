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
./polarfire_wb_server -A <baseaddress> -D -F &
./dma-read &
```
With the firmware Jeremy has been developing, `<baseaddress>` is `0xA0144000`.
