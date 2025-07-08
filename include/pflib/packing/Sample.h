#pragma once

#include <cstdint>
#include <fstream>

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
  /// header if using to_csv
  static const std::string to_csv_header;
  /**
   * Write out the Sample as a row in the CSV.
   * ```
   * Tp,Tc,adc_tm1,adc,tot,toa
   * ```
   * No newline is included in case other columns wish
   * to be added. If you don't want to include all seven
   * of these columns, you can write your own method.
   */
  void to_csv(std::ofstream& f) const;
};

}  // namespace pflib::packing
