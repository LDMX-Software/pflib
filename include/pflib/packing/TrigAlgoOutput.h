#pragma once
#ifndef PFLIB_PACKING_TRIGALGOOUTPUT_H
#define PFLIB_PACKING_TRIGALGOOUTPUT_H

#include <optional>
#include <bitset>

#include "pflib/packing/ECONTCaptureHeader.h"
#include "pflib/logging/Logging.h"
#include "pflib/packing/Reader.h"

namespace pflib::packing {

class TrigAlgoOutput {
  mutable ::pflib::logging::logger the_log_{::pflib::logging::get("decoding")};
  struct SingleBXOutput {
    std::bitset<8> is_high_peak_;
    bool trigger_;
  };
  const SingleBXOutput& sample(std::optional<int> i_sample = {}) const;
  std::vector<SingleBXOutput> samples_;
  ECONTCaptureHeader header_;
 public:
  TrigAlgoOutput() = default;
  TrigAlgoOutput(std::span<uint32_t> data);
  void from(std::span<uint32_t> data);
  Reader& read(Reader& r);
  std::size_t n_samples() const;
  const ECONTCaptureHeader& header() const;
  int length() const;
  bool is_high_peak(int i_stc, std::optional<int> i_sample = {}) const;
  bool trigger(std::optional<int> i_sample = {}) const;
};

}

#endif
