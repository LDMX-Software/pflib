#pragma once

#include <array>
#include <cstdint>
#include <span>

#include "pflib/Logging.h"
#include "pflib/packing/Mask.h"
#include "pflib/packing/Sample.h"

namespace pflib::packing {

/**
 * A frame readout from a DAQ Link
 *
 * The header and common mode words are unpacked into memory
 * while the DAQ and Calibration samples are given to Sample
 * to be unpacked only upon request.
 */
class DAQLinkFrame {
  mutable ::pflib::logging::logger the_log_{::pflib::logging::get("decoding")};

 public:
  /// id number for bunch crossing of this sample
  int bx;
  /// event number for this readout-request
  int event;
  /// orbit id number
  int orbit;
  /// error present in one of the counters
  bool counter_err;
  /// error present in first quarter (ch0-17 and cm)
  bool first_quarter_err;
  /// error present in second quarter (calib and ch18-35)
  bool second_quarter_err;
  /// flag if this is the first event
  bool first_event;
  /// adc readout from common mode 0
  int adc_cm0;
  /// adc readout from common mode 1
  int adc_cm1;

  /// array of samples from the channels
  std::array<Sample, 36> channels;
  /// sample from calibration channel
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

  /// default constructor that does not do anything:w
  DAQLinkFrame() = default;
};

}  // namespace pflib::packing
