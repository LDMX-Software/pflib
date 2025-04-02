#include "pflib/packing/SingleROCEventPacket.h"

#include <iostream>

#include "pflib/packing/Hex.h"
#include "pflib/packing/Mask.h"

namespace pflib::packing {

void SingleROCEventPacket::from(std::span<uint32_t> data) {
  if (data.size() != data[0]) {
    throw std::runtime_error(
        "Malformed Single ROC Event Packet. The total length should be the "
        "first word.");
  }

  // three words containing the link lengths
  std::array<uint32_t, 6> link_lengths;
  for (std::size_t i_word{0}; i_word < 3; i_word++) {
    const auto& word = data[i_word + 1];
    link_lengths[2 * i_word] = (word >> 16) & mask<16>;
    link_lengths[2 * i_word + 1] = (word)&mask<16>;
    pflib_log(trace) << hex(word) << " link lengths ";
  }

  std::size_t link_start_offset{1 + 3};
  for (std::size_t i_link{0}; i_link < 6; i_link++) {
    auto link_len = link_lengths[i_link];
    pflib_log(trace) << "link " << i_link << " length " << link_len;
    if (i_link < 2) {
      daq_links[i_link].from(data.subspan(link_start_offset, link_len));
    } else {
      pflib_log(trace) << "trigger link - skip";
      /*
      trigger_links[i_link-2].from(data.subspan(link_start_offset, link_len));
      */
    }
    link_start_offset += link_len;
  }
}

Reader& SingleROCEventPacket::read(Reader& r) {
  uint32_t prev_word{0}, word{0};
  pflib_log(trace) << "header scan...";
  while (prev_word != 0x11888811 or word != 0xbeef2025) {
    prev_word = word;
    if (!(r >> word)) break;
    pflib_log(trace) << hex(word);
  }
  if (!r) {
    pflib_log(trace) << "no header found";
    return r;
  }
  pflib_log(trace) << "found header";

  uint32_t total_len;
  r >> total_len;
  pflib_log(trace) << hex(total_len) << " total len: " << total_len;
  if (!r) {
    pflib_log(warn) << "partially transmitted ROC stream";
    return r;
  }

  std::vector<uint32_t> link_data(total_len);
  link_data[0] = total_len;
  // we already read the total_len word so we read the next total_len-1 words
  if (!r.read(link_data, total_len - 1, 1)) {
    pflib_log(warn) << "partially transmitted ROC stream";
    return r;
  }

  from(link_data);

  pflib_log(trace) << "trailer scan...";
  while (prev_word != 0xd07e2025 or word != 0x12345678) {
    prev_word = word;
    if (!(r >> word)) break;
    pflib_log(trace) << hex(word);
  }
  if (!r) {
    pflib_log(trace) << "no trailer found";
    return r;
  }
  pflib_log(trace) << "found trailer";

  return r;
}

}  // namespace pflib::packing
