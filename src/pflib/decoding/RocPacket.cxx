#include "pflib/decoding/RocPacket.h"

namespace pflib {
namespace decoding {

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
