#pragma once

#include "pflib/packing/ECONDEventPacket.h"
#include "pflib/packing/Reader.h"

namespace pflib::packing {

/**
 * Unpack an event that has potentially more than one sample
 * collected from a single ECOND.
 *
 * @see ECONDEventPacket for how a single sample from a single ECOND
 * is unpacked.
 *
 * @note This unpacking is not well tested and may change depending
 * on how the firmware/software progresses. The current draft was
 * written using TargetFiberless::read_event as a reference.
 * A second draft SoftWrappedECONDEventPacket was written using
 * the HcalBackplaneZCUTarget::read_event as reference.
 */
class MultiSampleECONDEventPacket {
  /// handle to logging source
  mutable ::pflib::logging::logger the_log_{::pflib::logging::get("decoding")};
  /// number of links connected to the ECOND
  int n_links_;

 public:
  /**
   * Corruption bits
   *
   * Index | Description
   * ------|------------
   *  0    | full packet header flag mismatch
   *  1    | subsystem ID not equal to 7 (HCAL)
   */
  std::array<bool, 2> corruption;
  /// index of the sample of interest (SOI)
  std::size_t i_soi;
  /// bunch counter/number for event
  int bx;
  /// event counter
  int ievent;
  /// contributor ID specifying ECOND
  int contrib_id;
  /// subsystem ID specifying ECOND
  int subsys_id;
  /// run number
  int run;
  /// samples from ECOND stored in order of transmission
  std::vector<ECONDEventPacket> samples;
  /// constructor defining how many links are connected to this ECOND
  MultiSampleECONDEventPacket(int n_links);
  /// unpack the given data into this structure
  void from(std::span<uint32_t> data);
  /// read into this structure from the input Reader
  Reader& read(Reader& r);

  /// header string if using to_csv
  static const std::string to_csv_header;

  void to_csv(std::ofstream& f) const;

  /// the two daq links for the connected HGCROC
  std::array<DAQLinkFrame, 2> daq_links;
};

}  // namespace pflib::packing
