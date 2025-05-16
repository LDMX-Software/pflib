#include "pflib/packing/DAQLinkFrame.h"

#include <bitset>
#include <iostream>
#include <sstream>

#include "pflib/packing/Hex.h"

namespace pflib::packing {

void DAQLinkFrame::from(std::span<uint32_t> data) {
  if (data.size() != 40) {
    std::stringstream msg{
        "DAQLinkFrame provided data words of incorrect length "};
    msg << data.size() << ".";
    if (data.size() > 40) {
      msg << "\nIdle words need to be trimmed.";
    }
    throw std::runtime_error(msg.str());
  }

  const uint32_t& header = data[0];
  pflib_log(trace) << "daq link header " << hex(header);
  uint32_t allones = (header >> (12 + 6 + 3 + 1 + 1 + 1 + 4)) & mask<4>;
  if (allones != 0b1111) {
    pflib_log(warn) << "bad leading header bits (should be all ones): "
                    << std::bitset<4>(allones);
  }
  bx = (header >> (6 + 3 + 1 + 1 + 1 + 4)) & mask<12>;
  event = (header >> (3 + 1 + 1 + 1 + 4)) & mask<6>;
  orbit = (header >> (1 + 1 + 1 + 4)) & mask<3>;
  counter_err = ((header >> (1 + 1 + 4)) & mask<1>) == 1;
  first_quarter_err = ((header >> (1 + 4)) & mask<1>) == 1;
  second_quarter_err = ((header >> (4)) & mask<1>) == 1;
  uint32_t trailflag = (header & mask<4>);
  first_event = (trailflag == 0b0101);
  if (not first_event and trailflag != 0b0010) {
    pflib_log(warn)
        << "bad event header flag (0b0101 or 0b0010 for first event): "
        << std::bitset<4>(trailflag);
  }

  const uint32_t& cm{data[1]};
  if (((cm >> 20) & mask<12>) != 0) {
    pflib_log(warn) << "bad common mode leading 12 bits (should be all zero): "
                    << std::bitset<12>(cm >> 20);
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
  pflib_log(trace) << "CRC Sum from stream: " << crc_sum;
}

DAQLinkFrame::DAQLinkFrame(std::span<uint32_t> data) { from(data); }

}  // namespace pflib::packing
