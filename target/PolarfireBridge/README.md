# PolarfireBridge
A pair of light rogue-based servers that receive wishbone commands from a Rogue source and send data from a DMA device to a Rogue destination.

## On DPM of Interest
```
# get this source code to the DPM somehow
cmake -B build -S .
cd build
make
./polarfire_wb_server -A <baseaddress> -D -F &
./dma-read --port 5972 &
```
