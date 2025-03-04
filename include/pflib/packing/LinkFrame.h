#pragma once

#include <array>

#include "pflib/packing/Mask.h"
#include "pflib/packing/Reader.h"

namespace pflib::packing {

struct LinkFrame {
  int bx;
  int event;
  int orbit;
  bool counter_err;
  bool first_quarter_err;
  bool second_quarter_err;
  bool first_event;
  int adc_cm0;
  int adc_cm1;

  struct Sample {
    bool Tc;
    bool Tp;
    int toa;
    int adc_tm1;
    int adc;
    int tot;

    Reader& read(Reader& r);
  };
  std::array<Sample, 36> channels;
  Sample calib;

  Reader& read(Reader& r);
};

}
