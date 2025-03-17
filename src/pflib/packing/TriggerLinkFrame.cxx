#include "pflib/packing/TriggerLinkFrame.h"

#include "pflib/packing/Mask.h"

#include <iostream>

namespace pflib::packing {

void TriggerLinkFrame::from(std::span<uint32_t> data) {
  if (data.size() != 5) {
    throw std::runtime_error("Trigger Link Frame given the wrong length of data "+std::to_string(data.size()));
  }

  if ((data[0] >> 28) != 0b0011) {
    // bad!
    std::cout << "bad trigger link header" << std::endl;
  }

  i_link = (data[0] & mask<8>);
  for (std::size_t i_word{0}; i_word < 4; i_word++) {
    data_words[i_word] = data[i_word+1];
  }
}

TriggerLinkFrame::TriggerLinkFrame(std::span<uint32_t> data) {
  from(data);
}

}
