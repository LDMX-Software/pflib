#include "pflib/packing/TriggerLinkFrame.h"

#include <iostream>

#include "pflib/Exception.h"
#include "pflib/packing/Mask.h"

#include "pflib/packing/DecodeAEBM.h"

namespace pflib::packing {

uint32_t TriggerLinkFrame::compressed_to_linearized(uint8_t cs) {
  return static_cast<uint32_t>(decodeAEBM<4, 3>(cs));
}

void TriggerLinkFrame::from(std::span<uint32_t> data) {
  if (data.size() != 5) {
    throw std::runtime_error(
        "Trigger Link Frame given the wrong length of data " +
        std::to_string(data.size()));
  }

  corruption[0] = ((data[0] >> 28) != 0x3);
  if (corruption[0]) {
    pflib_log(warn)
        << "bad link frame header inserted by software, probably software bug";
  }

  i_link = (data[0] & mask<8>);
  for (std::size_t i_word{0}; i_word < 4; i_word++) {
    auto head{data[i_word + 1] >> 28};
    corruption[i_word + 1] = (head != 0xa and head != 0x9);
    if (corruption[i_word + 1]) {
      pflib_log(warn) << "bad leading-four bits inserted by chip, probably "
                         "link misalignment";
    }
    data_words[i_word] = data[i_word + 1];
  }
}

uint8_t TriggerLinkFrame::compressed_sum(int i_sum, int i_bx) const {
  if (i_bx < -1 or i_bx > 2) {
    PFEXCEPTION_RAISE("OutOfRange",
                      "The provided BX index " + std::to_string(i_bx) +
                          " is outside the allow range of -1 to 2");
  }
  if (i_sum < 0 or i_sum > 3) {
    PFEXCEPTION_RAISE("OutOfRange",
                      "The provided sum index " + std::to_string(i_sum) +
                          " is outside the allow range of 0 to 3");
  }
  uint32_t w = data_words[i_soi_ + i_bx];
  return ((w >> ((3 - i_sum) * 7)) & mask<7>);
}

uint32_t TriggerLinkFrame::linearized_sum(int i_sum, int i_bx) const {
  return compressed_to_linearized(compressed_sum(i_sum, i_bx));
}

TriggerLinkFrame::TriggerLinkFrame(std::span<uint32_t> data) { from(data); }

}  // namespace pflib::packing
