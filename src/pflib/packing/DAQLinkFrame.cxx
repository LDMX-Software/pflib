#include "pflib/packing/DAQLinkFrame.h"

#include <bitset>
#include <iostream>
#include <sstream>

#include "pflib/Exception.h"
#include "pflib/packing/Hex.h"
#include "pflib/utility/crc.h"

namespace pflib::packing {

void DAQLinkFrame::from(std::span<uint32_t> data) {
  if (data.size() != 40) {
    std::stringstream msg{
        "DAQLinkFrame provided data words of incorrect length "};
    msg << data.size() << ".";
    if (data.size() > 40) {
      msg << "\nIdle words need to be trimmed.";
    }
    PFEXCEPTION_RAISE("Decoding", msg.str());
  }

  const uint32_t& header = data[0];
  pflib_log(trace) << hex(header) << " daq link header";
  uint32_t leading = (header >> (12 + 6 + 3 + 1 + 1 + 1 + 4)) & mask<4>;
  corruption[0] = (leading != 0b1111 and leading != 0b0101);
  if (corruption[0]) {
    pflib_log(warn) << "bad leading 4-bits of header (0b1111 or 0b0101): "
                    << std::bitset<4>(leading);
  }
  bx = (header >> (6 + 3 + 1 + 1 + 1 + 4)) & mask<12>;
  event = (header >> (3 + 1 + 1 + 1 + 4)) & mask<6>;
  orbit = (header >> (1 + 1 + 1 + 4)) & mask<3>;
  corruption[2] = ((header >> (1 + 1 + 4)) & mask<1>) == 1;
  corruption[3] = ((header >> (1 + 4)) & mask<1>) == 1;
  corruption[4] = ((header >> (4)) & mask<1>) == 1;
  uint32_t trailflag = (header & mask<4>);
  corruption[5] = (trailflag != 0b0101 and trailflag != 0b0010);
  if (corruption[5]) {
    pflib_log(warn) << "bad trailing 4 bits of header (0b0101 or 0b0010): "
                    << std::bitset<4>(trailflag);
  }

  const uint32_t& cm{data[1]};
  pflib_log(trace) << hex(cm) << " common mode word";
  corruption[6] = (((cm >> 20) & mask<12>) != 0);
  if (corruption[6]) {
    // these leading bits are ignored in the CMS hexactrl-sw decoding
    // so we are going to ignore them here putting
    pflib_log(warn) << "bad common mode leading 12 bits (should be all zero): "
                    << std::bitset<12>(cm >> 20);
  }
  adc_cm0 = (cm >> 10) & mask<10>;
  adc_cm1 = cm & mask<10>;

  std::size_t i_chan{0};
  for (; i_chan < 18; i_chan++) {
    channels[i_chan].word = data[2 + i_chan];
    pflib_log(trace) << hex(channels[i_chan].word) << " -> ch " << i_chan
                     << " Tp = " << std::boolalpha << channels[i_chan].Tp()
                     << " Tc = " << std::boolalpha << channels[i_chan].Tc()
                     << " adc = " << channels[i_chan].adc()
                     << " adc_tm1 = " << channels[i_chan].adc_tm1()
                     << " tot = " << channels[i_chan].tot()
                     << " toa = " << channels[i_chan].toa();
  }

  calib.word = data[2 + 18];
  pflib_log(trace) << hex(calib.word) << " -> calib"
                   << " Tp = " << std::boolalpha << calib.Tp()
                   << " Tc = " << std::boolalpha << calib.Tc()
                   << " adc = " << calib.adc()
                   << " adc_tm1 = " << calib.adc_tm1()
                   << " tot = " << calib.tot() << " toa = " << calib.toa();

  for (; i_chan < 36; i_chan++) {
    channels[i_chan].word = data[2 + 1 + i_chan];
    pflib_log(trace) << hex(channels[i_chan].word) << " -> ch " << i_chan
                     << " Tp = " << std::boolalpha << channels[i_chan].Tp()
                     << " Tc = " << std::boolalpha << channels[i_chan].Tc()
                     << " adc = " << channels[i_chan].adc()
                     << " adc_tm1 = " << channels[i_chan].adc_tm1()
                     << " tot = " << channels[i_chan].tot()
                     << " toa = " << channels[i_chan].toa();
  }

  // CRC of first 39 words, 40th word is the crc itself
  auto crcval = utility::crc32(std::span(data.begin(), 39));
  uint32_t target = data[39];
  corruption[1] = (crcval != target);
  // no warning on CRC sum, again like CMS hexactrl-sw
  if (corruption[1]) {
    pflib_log(warn) << "CRC sum don't match " << hex(crcval)
                    << " != " << hex(target);
  }
}

DAQLinkFrame::DAQLinkFrame(std::span<uint32_t> data) { from(data); }

}  // namespace pflib::packing
