#pragma once
#ifndef PFLIB_PACKING_ECALECONTCAPTUREFRAME_H
#define PFLIB_PACKING_ECALECONTCAPTUREFRAME_H

#include <optional>
#include <bitset>

#include "pflib/logging/Logging.h"
#include "pflib/packing/ECONTCaptureHeader.h"
#include "pflib/packing/Reader.h"

namespace pflib::packing {

/**
 * Each Ecal ECON-T sample has the following struture, assuming
 * - using "Best Choice" (BC) algorithm
 * - and six eTx enabled (for a full HD hexamodule)
 *
 * This means the 18 highest TCs are transmitted
 * along with the module sum.
 * The TCs are given in their original 4E+3M encoding
 * while the module sum is encoded with 5E+3M.
 *
 * eTx 0
 * - [31:28] = header
 * - [27:20] = Sum TC
 * - [19: 0] = Ch Map [47:28]
 * eTx 1
 * - [31:3] = Ch Map [27:0]
 * - [2:0] = Q TC1 [6:4]
 * eTx 2
 * - [31:28] = Q TC1 [3:0]
 * - [27:21] = Q TC2
 * - ... continue down in 6 bit increments
 */
class EcalECONTSample {
  mutable ::pflib::logging::logger the_log_{
      ::pflib::logging::get("decoding")};
  static constexpr std::size_t N_TC = 18;
  std::array<int, N_TC> max_tc_;
  std::bitset<48> tc_ch_map_;
  int sum_;
  int bx_;
 public:
  static constexpr int WORD_LEN = 6;
  void from(std::span<uint32_t> data);
  /// bx counter for this sample of STC sums
  int bx() const;
  /// decode the 5E+3M encoding into a linearized sum value
  int sum() const;
  /// the raw value trasmitted by the ECON-T
  int encoded_sum() const;
  /// get TC encoded sum for the input TC index
  int encoded_tc(int i_tc) const;
  /// get TC sum for the input TC index
  int tc(int i_tc) const;
  /// get the channel map showing which TC were included in readout
  std::bitset<48> ch_map() const;
};

}  // namespace pflib::packing

#endif
