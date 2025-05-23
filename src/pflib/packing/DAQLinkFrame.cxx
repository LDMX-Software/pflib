#include "pflib/packing/DAQLinkFrame.h"

#include <bitset>
#include <iostream>
#include <sstream>

#include <boost/endian/conversion.hpp>
#include <boost/crc.hpp>

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
  corruption[0] = (allones != 0b1111);
  if (corruption[0]) {
    pflib_log(warn) << "bad leading header bits (should be all ones): "
                    << std::bitset<4>(allones);
  }
  bx = (header >> (6 + 3 + 1 + 1 + 1 + 4)) & mask<12>;
  event = (header >> (3 + 1 + 1 + 1 + 4)) & mask<6>;
  orbit = (header >> (1 + 1 + 1 + 4)) & mask<3>;
  corruption[2] = ((header >> (1 + 1 + 4)) & mask<1>) == 1;
  corruption[3] = ((header >> (1 + 4)) & mask<1>) == 1;
  corruption[4] = ((header >> (4)) & mask<1>) == 1;
  uint32_t trailflag = (header & mask<4>);
  first_event = (trailflag == 0b0010);
  corruption[5] = (not first_event and trailflag != 0b0101);
  if (corruption[5]) {
    pflib_log(warn)
        << "bad event header flag (0b0101 or 0b0010 for first event): "
        << std::bitset<4>(trailflag);
  }

  const uint32_t& cm{data[1]};
  if (((cm >> 20) & mask<12>) != 0) {
    // these leading bits are ignored in the CMS hexactrl-sw decoding
    // so we are going to ignore them here
    pflib_log(trace) << "bad common mode leading 12 bits (should be all zero): "
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

  // CMS hexactrl-sw decoding needed to endian-reverse all the words
  // prior to calculating the CRC here so we have to copy the data into
  // a new area so we can change the words before processing them through
  // the CRC calculator
  std::vector<uint32_t> crc_inputs{data.begin(), data.end()};
  std::transform( crc_inputs.begin(), crc_inputs.end(), crc_inputs.begin(),
    [](uint32_t w) { return boost::endian::endian_reverse(w); });
  auto input_ptr = reinterpret_cast<const unsigned char*>(crc_inputs.data());
  auto crc32 = boost::crc<32, 0x4c11db7, 0x0, 0x0, false, false>(input_ptr, 39*4);
  uint32_t target = data[39];
  corruption[1] = (crc32 != target);
  /* no warning on CRC sum, again like CMS hexactrl-sw
  if (corruption[1]) {
    pflib_log(warn) << "CRC sum don't match " << hex(crc32) << " != " << hex(target);
  }
  */
}

DAQLinkFrame::DAQLinkFrame(std::span<uint32_t> data) { from(data); }

}  // namespace pflib::packing
