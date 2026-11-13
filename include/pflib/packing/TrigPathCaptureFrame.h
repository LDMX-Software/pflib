#pragma once
#ifndef PFLIB_PACKING_TRIGPATHCAPTUREFRAME_H
#define PFLIB_PACKING_TRIGPATHCAPTUREFRAME_H

#include <optional>

#include "pflib/logging/Logging.h"
#include "pflib/packing/ECONTCaptureHeader.h"
#include "pflib/packing/Reader.h"

namespace pflib::packing {

/**
 * This represents the capture block in the ZCU trigpath firmware
 *
 * It uses the information passed through the ECONTCaptureHeader words
 * in order to deduce the number of ECONT Samples to parse.
 *
 * @tparam SingelECONTSample This class must define how to decode a single
 * sample from the ECONT(s).
 */
template<class SingleECONTSample>
class TrigPathCaptureFrame {
  mutable ::pflib::logging::logger the_log_{::pflib::logging::get("decoding")};
 public:
  TrigPathCaptureFrame() = default;
  TrigPathCaptureFrame(std::span<uint32_t> data);
  Reader& read(Reader& r);
  /**
   * parse the header and then pass parsing onto the samples
   *
   * We do some checks on the packet length mostly to check for coding
   * errors. This is where we require the SingleECONTSample class to
   * define a static constant value WORD_LEN defining how many words
   * are required per sample.
   */
  void from(std::span<uint32_t> data);
  /**
   * get a sample at a specific index
   *
   * @param[in] i_sample index for desired sample, default is SoI as defined by capture header
   */
  const SingleECONTSample& sample(std::optional<int> i_sample = {}) const;
  /// get access to header
  const ECONTCaptureHeader& header() const;
  /// get the total length of this frame
  int length() const {
    return header().length();
  }
 private:
  ECONTCaptureHeader header_;
  std::vector<SingleECONTSample> samples_;
};

}  // namespace pflib::packing

#endif
