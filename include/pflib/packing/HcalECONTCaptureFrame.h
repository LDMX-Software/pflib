#pragma once
#ifndef PFLIB_PACKING_HCALBACKPLANEECONTPAIRSAMPLE_H
#define PFLIB_PACKING_HCALBACKPLANEECONTPAIRSAMPLE_H

#include <array>
#include <span>

#include "pflib/logging/Logging.h"

namespace pflib::packing {

/**
 * A single sample from a pair of ECON-T on an Hcal Backplane
 *
 * Each ECON-T sample as the following structure, assuming
 * - using STC4 algorithm
 * - with the 5M+4E encoding
 * - and three eTx enabled
 *
 * This means there are up to 8 STCs.
 * Each sample is spread across the three eTx.
 *
 * eTx 0
 * - [31:28] = header
 * - [27:26] = Max1 (index of highest TC in STC1)
 * - [25:24] = Max2 (index of highest TC in STC2)
 * - ... continue down
 * - [12:11] = Max8 (index of highest TC in STC8)
 * - [10:2] = STC1
 * - [1:0] = 2 MSBs of STC2
 * eTx 1
 * - [31:25] = 7 LSBs of STC2
 * - [25:16] = STC3
 * - [15:7] = STC4
 * - [6:0] = 7 MSB of STC5
 * eTx 2
 * - [31:30] = 2 LSB of STC5
 * - [29:21] = STC6
 * - [20:12] = STC7
 * - [11:3] = STC8
 * - [2:0] = zero padding
 *
 * @note The i_stc indices here are the normal STC indices
 * output by the ECON-T, but the "higher" ECON-T (ECON-T2)
 * is shifted by 8 so that they range from 0 to 15.
 */
class HcalBackplaneECONTPairSample {
  mutable ::pflib::logging::logger the_log_{::pflib::logging::get("decoding")};
  static constexpr std::size_t N_STC = 16;
  std::array<int, N_STC> max_tc_;
  std::array<int, N_STC> stc_sums_;
  std::array<int, 2> bx_;
 public:
  /// 2 ECON-T producing 3 words each
  static constexpr int WORD_LEN = 6;
  /// parse the input data
  void from(std::span<uint32_t> data);
  /// bx counter for this sample of STC sums
  int bx(int i_econt) const;
  /// decode the 5E+4M encoding into a linearized sum value
  int stc_sum(int i_stc) const;
  /// the raw value trasmitted by the ECON-T
  int encoded_stc_sum(int i_stc) const;
  /// index of the TC within the STC that was the maximum
  int max_tc(int i_stc) const;
};

}  // namespace pflib::packing

#endif
