#include "pflib/packing/ECONDEventPacket.h"

#include "pflib/packing/Hex.h"
#include "pflib/packing/Mask.h"

namespace pflib::packing {

void ECONDEventPacket::from(std::span<uint32_t> data) {
}

Reader& ECONDEventPacket::read(Reader& r) {
  uint32_t word{0};
  pflib_log(trace) << "header scan...";
  while (word != 0xb33f2025) {
    if (!(r >> word)) break;
    pflib_log(trace) << hex(word);
  }
  if (!r) {
    pflib_log(trace) << "no header found";
    return r;
  }
  pflib_log(trace) << "found header";

  pflib_log(trace) << "scanning for end of packet...";
  while (word != 0x12345678) {
    if (!(r >> word)) break;
    pflib_log(trace) << hex(word);
  }

  return r;
}

}
