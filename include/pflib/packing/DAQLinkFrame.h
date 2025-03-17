#pragma once

#include <span>
#include <array>
#include <cstdint>

#include "pflib/packing/Mask.h"
#include "pflib/packing/Sample.h"

namespace pflib::packing {

/**
 * A frame readout from a DAQ Link
 *
 * The header and common mode words are unpacked into memory
 * while the DAQ and Calibration samples are given to Sample
 */
struct DAQLinkFrame {
  int bx;
  int event;
  int orbit;
  bool counter_err;
  bool first_quarter_err;
  bool second_quarter_err;
  bool first_event;
  int adc_cm0;
  int adc_cm1;

  std::array<Sample, 36> channels;
  Sample calib;

  /**
   * Parse into this link frame from a std::span over 32-bit words
   */
  void from(std::span<uint32_t> data);

  /**
   * Construct from a std::span over 32-bit words
   *
   * @note
   * A std::span can be transparently constructed from a std::vector
   * if that is what is available, but it can also be used to simply
   * view a subslice of a larger std::vector.
   */
  DAQLinkFrame(std::span<uint32_t> data);

  DAQLinkFrame() = default;
};

}
