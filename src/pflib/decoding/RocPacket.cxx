#include "pflib/decoding/RocPacket.h"
#include <stdio.h>

namespace pflib {
namespace decoding {

RocPacket::RocPacket(const uint32_t* header_ptr, int len) : data_{header_ptr}, length_{len} {
  
}

void RocPacket::dump() const {
  for (int i=0; i<length_; i++) {
    printf("%2d %08x ",i,data_[i]);
    if (i<36 && has_chan(i)) printf(" %d",get_adc(i));
    printf("\n");
  }
}

int RocPacket::offset_to_chan(int ichan) const {
  if (length_<2 || ichan<0 || ichan>=36) return -1;
  int offset=0;

  uint64_t readout_map=(uint64_t(data_[0])<<32)|data_[1];

  int inominal=ichan+1; // first word in ROC V2 is header
  if (ichan>=18) inominal+=2; // common mode and calib in the middle

  for (int i=0; i<inominal; i++) {
    if (readout_map&0x1) offset++;
    readout_map=readout_map>>1;
  }
  if (readout_map&0x1) return offset+2; // +2 for the two Polarfire header words
  else return -1;

}

}
}
