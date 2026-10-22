#pragma once

#include "pflib/packing/ECONDEventPacket.h"
#include "pflib/packing/Reader.h"

namespace pflib::packing {

/// the class used to do the mapping
class SingleECONDRocErxMapping;

/**
 * Unpack an event that has potentially more than one sample
 * collected from a **single** ECOND.
 *
 * @see ECONDEventPacket for how a single sample from a single ECOND
 * is unpacked.
 *
 * @note This unpacking is not well tested and may change depending
 * on how the firmware/software progresses. The current draft was
 * written using TargetFiberless::read_event as a reference.
 */
class MultiSampleECONDEventPacket {
  /// handle to logging source
  mutable ::pflib::logging::logger the_log_{::pflib::logging::get("decoding")};
  /// number of links connected to the ECOND
  int n_links_;

 public:
  /*
   * Corruption bits
   *
   * Index | Description
   * ------|------------
   *  0    | full packet header flag mismatch
  std::array<bool, 1> corruption;
   */

  /// index of the sample of interest (SOI)
  std::size_t i_soi;
  /// ID for this econ
  int econd_id;
  /// contributor ID
  int contrib_id;
  /// subsystem ID
  int subsys_id;
  /// timestamp from RoR
  uint64_t timestamp;
  /// get the sample of interest
  const ECONDEventPacket& soi() const;
  /// samples from ECOND stored in order of transmission
  std::vector<ECONDEventPacket> samples;
  /// constructor defining how many links are connected to this ECOND
  MultiSampleECONDEventPacket(int n_links);
  /// unpack the given data into this structure
  void from(std::span<uint32_t> data, bool expect_ldmx_ror_header = false);
  /// read into this structure from the input Reader
  Reader& read(Reader& r);

  /// header string if using MultiSampleECONDEventPacket::to_csv
  static const std::string to_csv_header;

  /// header string if using MultiSampleECONDEventPacket::to_csv with an input mapping
  static const std::string to_csv_header_with_mapping;

  /**
   * write out all of the samples into the input CSV file
   *
   * @param[in] f output file stream to write rows into
   * @param[in] mapping if provided, use the input mapping to convert the (i_erx, i_ch) indices
   * into (i_roc, ch indices)
   *
   * @note This changes the channel-labeling columns and so the
   * MultiSampleECONDEventPacket::to_csv_header_with_mapping
   * should be used instead of MultiSampleECONDEventPacket::to_csv_header
   * when writing the column header row in the output CSV file
   */
  void to_csv(std::ofstream& f, const SingleECONDRocErxMapping* mapping = nullptr) const;
};

}  // namespace pflib::packing
