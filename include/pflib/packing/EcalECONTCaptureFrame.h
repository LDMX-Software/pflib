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
 * A capture frame has one or more SingleECONTSamples
 * along with two headers written by the firmware that
 * encode how many samples there are and which are important
 */
class EcalECONTCaptureFrame {
  mutable ::pflib::logging::logger the_log_{::pflib::logging::get("decoding")};

 public:
  /**
   * Each ECON-T sample has the following struture,
   * assuming
   * - using "Best Choice" (BC) algorithm
   * - and six eTx enabled
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
  class SingleECONTSample {
    mutable ::pflib::logging::logger the_log_{
        ::pflib::logging::get("decoding")};
    static constexpr std::size_t N_TC = 18;
    std::array<int, N_TC> max_tc_;
    std::bitset<48> tc_ch_map_;
    int sum_;
    int bx_;

   public:
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

  EcalECONTCaptureFrame() = default;
  EcalECONTCaptureFrame(std::span<uint32_t> data);
  Reader& read(Reader& r);
  void from(std::span<uint32_t> data);
  const SingleECONTSample& sample(std::optional<int> i_econt = {},
                                  std::optional<int> i_sample = {}) const;
  int bx(std::optional<int> i_econt = {},
         std::optional<int> i_sample = {}) const;
  int sum(std::optional<int> i_econt = {},
          std::optional<int> i_sample = {}) const;
  int encoded_sum(std::optional<int> i_econt = {},
                  std::optional<int> i_sample = {}) const;
  std::bitset<48> ch_map(std::optional<int> i_econt = {},
             std::optional<int> i_sample = {}) const;
  int tc(int i_tc, std::optional<int> i_econt = {},
         std::optional<int> i_sample = {}) const;
  const ECONTCaptureHeader& header() const;
  int n_econt() const;
  int length() const;
  int version() const;
  int econ_id() const;
  int pre_samples() const;
  std::size_t n_samples() const;

 private:
  ECONTCaptureHeader header_;
  std::vector<std::vector<SingleECONTSample>> samples_;
};

}  // namespace pflib::packing

#endif
