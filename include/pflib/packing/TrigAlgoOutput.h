#pragma once
#ifndef PFLIB_PACKING_TRIGALGOOUTPUT_H
#define PFLIB_PACKING_TRIGALGOOUTPUT_H

#include <bitset>
#include <optional>

#include "pflib/logging/Logging.h"
#include "pflib/packing/ECONTCaptureHeader.h"
#include "pflib/packing/Reader.h"

namespace pflib::packing {

class TrigAlgoOutput {
 public:
  struct SingleBXOutput {
    /// whether STC i had a high peak
    std::bitset<8> is_high_peak_;
    /// if the algorithm would trigger
    bool algo_trigger_;
    /// if a trigger was actually sent
    /// (the trigger may be prevented by the single-shot gate)
    bool gated_trigger_;
    /// if the elink was valid
    bool elink_valid_;
    /// if the link alignment was valid
    bool econ_tdata_dv_;
    /// print human readable sample
    friend inline std::ostream& operator<<(std::ostream& o, const SingleBXOutput& sample) {
      o << "{ is_high_peak: " << sample.is_high_peak_
        << ", elink_valid: " << sample.elink_valid_
        << ", econ_tdata_dv: " << sample.econ_tdata_dv_
        << ", algo_trigger: " << sample.algo_trigger_
        << ", gated_trigger: " << sample.gated_trigger_
        << " }";
      return o;
    }
  };
  const SingleBXOutput& sample(std::optional<int> i_sample = {}) const;
  TrigAlgoOutput() = default;
  TrigAlgoOutput(std::span<uint32_t> data);
  void from(std::span<uint32_t> data);
  Reader& read(Reader& r);
  std::size_t n_samples() const;
  const ECONTCaptureHeader& header() const;
  int length() const;
  bool is_high_peak(int i_stc, std::optional<int> i_sample = {}) const;
  /// reports the gated trigger i.e. if a readout request would be sent
  bool trigger(std::optional<int> i_sample = {}) const;
  /// reports if the algo would have triggered
  bool algo_trigger(std::optional<int> i_sample = {}) const;
  bool elink_valid(std::optional<int> i_sample = {}) const;
  bool econ_tdata_dv(std::optional<int> i_sample = {}) const;
 private:
  mutable ::pflib::logging::logger the_log_{::pflib::logging::get("decoding")};
  std::vector<SingleBXOutput> samples_;
  ECONTCaptureHeader header_;
};

}  // namespace pflib::packing

#endif
