#include "pflib/packing/LinkFrame.h"

#include "pflib/packing/Hex.h"

#include <bitset>
#include <iostream>

namespace pflib::packing {

bool Sample::Tc() {
  return ((this->word >> 31)&0b1) == 1;
}

bool Sample::Tp() {
  return ((this->word >> 30)&0b1) == 1;
}

int Sample::toa() {
  return (this->word & mask<10>);
}

int Sample::adc_tm1() {
  // weird situation without pre-sample
  if (Tc() and not Tp()) return -1;
  // otherwise, just the 10 bits after the two flags
  return ((this->word >> 20) & mask<10>);
}

int Sample::adc() {
  if (not Tc()) {
    return ((this->word >> 10) & mask<10>);
  } else if (not Tp()) {
    return ((this->word >> 20) & mask<10>);
  } else {
    return -1;
  }
}

int Sample::tot() {
  if (Tc()) {
    return ((this->word >> 10) & mask<10>);
  } else {
    return -1;
  }
}

void LinkFrame::from(std::span<uint32_t> data) {
  if (data.size() != 40) {
    std::stringstream msg{"LinkFrame provided data words of incorrect length "};
    msg << data.size() << ".";
    if (data.size() > 40) {
      msg << "\nIdle words need to be trimmed."
    }
    throw std::runtime_error(msg.str());
  }

  const uint32_t& header = data[0];
  std::cout << "daq link header " << hex(header) << std::endl;
  uint32_t allones = (header >> (12+6+3+1+1+1+4)) & mask<4>;
  if (allones != 0b1111) {
    // bad!
    std::cout << "  bad leading header bits " << std::bitset<4>(allones) << std::endl;
  }
  bx = (header >> (6+3+1+1+1+4)) & mask<12>;
  event = (header >> (3+1+1+1+4)) & mask<6>;
  orbit = (header >> (1+1+1+4)) & mask<3>;
  counter_err = ((header >> (1+1+4)) & mask<1>) == 1;
  first_quarter_err = ((header >> (1+4)) & mask<1>) == 1;
  second_quarter_err = ((header >> (4)) & mask<1>) == 1;
  uint32_t trailflag = (header & mask<4>);
  first_event = (trailflag == 0b0101);
  if (not first_event and trailflag != 0b0010) {
    // bad!
    std::cout << "  bad event header flag " << std::bitset<4>(trailflag) << std::endl;
  }

  const uint32_t& cm{data[1]};
  if (((cm >> 20) & mask<12>) != 0) {
    // bad!
    std::cout << "  bad common mode leading 12 bits " << std::bitset<12>(cm >> 20) << std::endl;
  }
  adc_cm0 = (cm >> 10) & mask<10>;
  adc_cm1 = cm & mask<10>;

  std::size_t i_chan{0};
  for (; i_chan < 18; i_chan++) {
    channels[i_chan].word = data[2 + i_chan];
  }

  calib.word = data[2 + 18];

  for (; i_chan < 36; i_chan++) {
    channels[i_chan].word = data[2 + 1 + i_chan];
  }

  [[maybe_unused]] uint32_t crc_sum = data[39];
}

LinkFrame::LinkFrame(std::span<uint32_t> data) {
  from(data);
}

}
