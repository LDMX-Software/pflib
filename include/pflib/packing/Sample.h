#pragma once

#include <cstdint>

namespace pflib::packing {

/**
 * A single DAQ 32-bit sample
 *
 * The 32-bit word is stored in memory and then decoded upon
 * request in order to save memory space. Only some of the
 * values are present within the word depending on the flags
 * Tp and Tc at the beginning of the word, so if a value returns
 * -1, that value is not present in the word.
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
