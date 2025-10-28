"""set reference clock for lpGBT

- only needs to be done if the lpGBT is power-cycled.
- needs the smbus2 package from pip
"""


import argparse

parser=argparse.ArgumentParser(description="ZCU Controls")
parser.add_argument('--newrefclk',type=float,default=320.64,help='New reference clock (default 320.64 MHz)')
parser.add_argument('--curclk',type=lambda x: int(x,0),required=True,help='Current GTH_REFCLK reading in hex')

args=parser.parse_args()

from smbus2 import SMBus, i2c_msg
# Arcane incantation to update Si570 clock rate
# https://www.silabs.com/documents/public/data-sheets/si570.pdf
#   a=transceiver.getNode("RATES.GTH_REFCLK").read()
#   refClkRate = (a*1.)*0.5e-3#transceiver.getNode("RATES.GTH_REFCLK").read()*8e-6
a=args.curclk

refClkRate = (a*1.)*1e-3

ibus=9
hs_div_map = {0:4, 1:5, 2:6, 3:7, 5:9, 7:11}
rev_hs_div_map = dict((reversed(item) for item in hs_div_map.items()))

with SMBus(ibus) as bus:
    bus.write_byte(0x5d, 7, force=True)
    msg_read    = i2c_msg.read(0x5d, 6)
    
    bus.i2c_rdwr(msg_read)

value = int.from_bytes(list(msg_read), byteorder="big", signed=False)

rfreq = float(value&0x3fffffffff) / 2**28
hs_div = hs_div_map[(value >> 45)&0x7]
N1 = ((value >> 38)&0x7f) + 1

fxtal = refClkRate * hs_div * N1 / rfreq
newFreq = args.newrefclk #MHz
#   newFreq = 348.64 #MHz

   # select HS_DIV and N1 such that 4.85 GHz < newFreq * HS_DIV * N1 < 5.67 GHz
new_hs_div = 4
new_N1 = 4

new_rfreq = newFreq * new_hs_div * new_N1 / fxtal

new_rfreq_int = int(new_rfreq * 2**28)

i2c_data = [(rev_hs_div_map[new_hs_div] << 5) | (((new_N1-1) >> 2)&0x1f),
            (((new_N1-1) << 6)&0xc0) | ((new_rfreq_int >> 32))&0x3f,
            ((new_rfreq_int >> 24))&0xff,
            ((new_rfreq_int >> 16))&0xff,
            ((new_rfreq_int >> 8)) &0xff,
            ((new_rfreq_int >> 0)) &0xff]

with SMBus(ibus) as bus:
    bus.write_i2c_block_data(0x5d, 137, [0x18], force=True)
    bus.write_i2c_block_data(0x5d, 7, i2c_data, force=True)
    bus.write_i2c_block_data(0x5d, 137, [0x08], force=True)
    bus.write_i2c_block_data(0x5d, 135, [0x40], force=True)
