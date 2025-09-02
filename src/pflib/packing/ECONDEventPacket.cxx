#include "pflib/packing/ECONDEventPacket.h"

#include "pflib/packing/Hex.h"
#include "pflib/packing/Mask.h"

namespace pflib::packing {

void ECONDEventPacket::from(std::span<uint32_t> data) {
  uint32_t header_marker = ((data.at(0) >> 23) & mask<9>);
  corruption[0] = (header_marker != (0xaa << 1));
  if (corruption[0]) {
    pflib_log(warn) << "Bad header marker from ECOND " << hex(header_marker);
  }

  uint32_t length = ((data.at(0) >> 14) & mask<9>);
  corruption[1] = (data.size() - 2 != length);
  if (corruption[1]) {
    pflib_log(warn) << "Incomplete event packet, stored payload length "
      << length << " does not equal packet length " << data.size() - 2;
  }

  bool passthrough = ((data.at(0) >> 13) & mask<1>) == 1;
  bool expected = ((data.at(0) >> 12) & mask<1>) == 1;
  uint32_t ht_ebo = ((data.at(0) >> 8) & mask<8>);
  bool m = ((data.at(0) >> 7) & mask<1>) == 1;
  bool truncated = ((data.at(0) >> 6) & mask<1>) == 1;
  uint32_t hamming = (data.at(0) & mask<6>);

  uint32_t bx = ((data.at(1) >> 20) & mask<12>);
  uint32_t l1a = ((data.at(1) >> 14) & mask<6>);
  uint32_t orb = ((data.at(1) >> 11) & mask<3>);
  bool subpacket_error = ((data.at(1) >> 10) & mask<1>) == 1;
  uint32_t rr = ((data.at(1) >> 8) & mask<2>);
  uint32_t crc = (data.at(1) & mask<8>);

  // sub-packet start
  uint32_t stat = ((data.at(2) >> 29) & mask<3>);
  corruption[2] = (stat != 0b111); // all ones is GOOD
  if (corruption[2]) {
    pflib_log(warn) << "bad subpacket checks";
  }
  uint32_t ham = ((data.at(2) >> 26) & mask<3>);
  bool is_empty = ((data.at(2) >> 25) & mask<1>) == 1;
  uint32_t cm0 = ((data.at(2) >> 15) & mask<12>);
  uint32_t cm1 = ((data.at(2) >>  5) & mask<12>);
  if (is_empty) {
    // F=1
    // single-word header, no channel data or channel map, entire link is empty
    bool error_caused_empty = ((data.at(2) >> 4) & mask<1>);
    // E=0 (false) means none of the 37 channels passed zero suppression
    // E=1 (true) means the unmasked Stat error bits caused the sub-packet to be suppressed
  } else {
    // construct channel map
    std::bitset<37> chan_map;

    // consume channel data
  }
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
