#pragma once
#ifndef PFLIB_PACKING_SINGLEECONTCAPTUREFRAME_H
#define PFLIB_PACKING_SINGLEECONTCAPTUREFRAME_H

#include <optional>

#include "pflib/packing/ECONTCaptureHeader.h"
#include "pflib/logging/Logging.h"
#include "pflib/packing/Reader.h"

namespace pflib::packing {

/**
 * A capture frame has one or more SingleECONTSamples
 * along with two headers written by the firmware that
 * encode how many samples there are and which are important
 */
class SingleECONTCaptureFrame {
  mutable ::pflib::logging::logger the_log_{::pflib::logging::get("decoding")};
 public:
  /**
   * Each ECON-T sample has the following struture,
   * assuming
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
   */
  class SingleECONTSample {
    mutable ::pflib::logging::logger the_log_{::pflib::logging::get("decoding")};
    static constexpr std::size_t N_STC = 8;
    std::array<int, N_STC> max_tc_;
    std::array<int, N_STC> stc_sums_;
    int bx_;
   public:
    void from(std::span<uint32_t> data);
    int bx() const;
    int stc_sum(int i_stc) const;
    int max_tc(int i_stc) const;
  };

  SingleECONTCaptureFrame() = default;
  SingleECONTCaptureFrame(std::span<uint32_t> data);
  Reader& read(Reader& r);
  void from(std::span<uint32_t> data);
  const SingleECONTSample& sample(std::optional<int> i_sample = {}) const;
  int bx(std::optional<int> i_sample = {}) const;
  int stc_sum(int i_stc, std::optional<int> i_sample = {}) const;
  int max_tc(int i_stc, std::optional<int> i_sample = {}) const;
  const ECONTCaptureHeader& header() const;
  int length() const;
  int version() const;
  int econ_id() const;
  int pre_samples() const;
  std::size_t n_samples() const;

 private:
  ECONTCaptureHeader header_;
  std::vector<SingleECONTSample> samples_;
};

}

#endif
