#pragma once

#include <cstdint>

namespace pflib::packing {

/**
 * A single DAQ 32-bit sample
 *
 * The 32-bit word is stored in memory and then decoded upon
 * request.
 */
struct Sample {
  uint32_t word;
  bool Tc() const;
  bool Tp() const;
  int toa() const;
  int adc_tm1() const;
  int adc() const;
  int tot() const;
};

}
