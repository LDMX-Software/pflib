#include "pflib/packing/LinkFrame.h"

#include <cstdint>

namespace pflib::packing {

Reader& LinkFrame::Sample::read(Reader& r) {
  uint32_t word;
  r >> word;
  Tc = ((word >> 31)&0b1) == 1;
  Tp = ((word >> 30)&0b1) == 1;
  toa = (word & mask<10>);
  if (not Tc and not Tp) {
    // normal adc behavior, no TOT
    adc_tm1 = (word >> 20) & mask<10>;
    adc = (word >> 10) & mask<10>;
    tot = -1;
  } else if (not Tc and Tp) {
    // TOT busy, adc saturated or undershoot
    adc_tm1 = (word >> 20) & mask<10>;
    adc = (word >> 10) & mask<10>;
    tot = -1;
  } else if (Tc and Tp) {
    // TOT output
    adc_tm1 = (word >> 20) & mask<10>;
    adc = -1;
    tot = (word >> 10) & mask<10>;
  } else /* Tc and not Tp */ {
    // only appear when TOT value is near threshold
    adc_tm1 = -1;
    adc = (word >> 20) & mask<10>;
    tot = (word >> 10) & mask<10>;
  }
  return r;
}

Reader& LinkFrame::read(Reader& r) {
  uint32_t header, cm, crc_sum, idle;
  r >> header;

  uint32_t allones = (header >> (12+6+3+1+1+1+4)) & mask<4>;
  if (allones != 0b1111) {
    // bad!
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
  }

  r >> cm;
  if (((cm >> 20) & mask<12>) != ~mask<12>) {
    // bad!
  }
  adc_cm0 = (cm >> 10) & mask<10>;
  adc_cm1 = cm & mask<10>;

  std::size_t i_chan{0};
  for (; i_chan < 18; i_chan++) {
    r >> channels[i_chan];
  }

  r >> calib;

  for (; i_chan < 36; i_chan++) {
    r >> channels[i_chan];
  }

  r >> crc_sum;
  r >> idle;
  return r;
}

}
