#include "pflib/packing/MultiSampleECONDEventPacket.h"

#include "pflib/packing/Hex.h"
#include "pflib/packing/Mask.h"

namespace pflib::packing {

MultiSampleECONDEventPacket::MultiSampleECONDEventPacket(int n_links)
  : n_links_{n_links} {}

void MultiSampleECONDEventPacket::from(std::span<uint32_t> data) {
  run = data[0];
  ievent = (data[1] >> 8);
  bx = (data[1] & mask<8>);
  uint32_t zero = data[2]; // should be total length?
  uint32_t head_flag = (data[3] >> 24);
  if (head_flag != 0xa6u) {
    // bad
  }
  contrib_id = ((data[3] >> 16) & mask<8>);
  if (((data[3] >> 8) & mask<8>) != 0x07) {
    // bad
  }

  std::size_t offset{4};
  while (offset < data.size()) {
    uint32_t another_header = data[offset];
    uint32_t link_len = (another_header & mask<16>);
    uint32_t head_flag = (another_header >> 28);
    uint32_t i_sample = ((another_header >> 12) & mask<4>);
    // other flags about SOI or last sample

    samples.emplace_back(n_links_);
    samples.back().from(data.subspan(offset+1, link_len));
    offset += link_len;
  }
}

Reader& MultiSampleECONDEventPacket::read(Reader& r) {
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

  std::vector<uint32_t> data;
  pflib_log(trace) << "scanning for end of packet...";
  while (word != 0x12345678) {
    if (!(r >> word)) break;
    data.push_back(word);
    pflib_log(trace) << hex(word);
  }
  if (!r and word != 0x12345678) {
    pflib_log(trace) << "no trailer found, probably incomplete packet";
  }

  this->from(data);

  return r;
}

}
