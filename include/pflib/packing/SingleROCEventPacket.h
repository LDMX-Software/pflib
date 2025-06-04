#pragma once

#include <array>
#include <span>

#include "pflib/Logging.h"
#include "pflib/packing/DAQLinkFrame.h"
#include "pflib/packing/Reader.h"
#include "pflib/packing/TriggerLinkFrame.h"

namespace pflib::packing {

/**
 * Simple HGCROC-only event packet
 *
 * No zero suppressions or condensation is done,
 * the entire HGCROC DAQ and Link packets are reaodut
 * into a single event packet in sequence.
 *
 * This is the style of readout from the 2021 teastbeam
 * and can be done easily in pflib without much emulation.
 */
class SingleROCEventPacket {
  mutable ::pflib::logging::logger the_log_{::pflib::logging::get("decoding")};

 public:
  /// the two daq links for the connected HGCROC
  std::array<DAQLinkFrame, 2> daq_links;
  /// the four trigger links
  std::array<TriggerLinkFrame, 4> trigger_links;
  /// parse into this package from the passed data span
  void from(std::span<uint32_t> data);
  /**
   * read from the input reader into this packet
   *
   * enables us to
   * ```cpp
   * SingleROCEventPacket ep;
   * r >> ep;
   * ```
   */
  Reader& read(Reader& r);
  /// header string if using to_csv
  static const std::string to_csv_header;
  /**
   * write current packet into a CSV
   *
   * @param[in,out] f file to write CSV to
   */
  void to_csv(std::ofstream& f) const;
  /**
   * Get a specific Sample from a channel
   *
   * The channel index input here is relative to the ROC
   * [0,71]. If you have the channel index within a daq
   * link (or ROC "half"), just access the daq links directly
   * ```cpp
   * ep.daq_links[i_link].channels[i_chan]
   * ```
   * where `i_link` is the link index 0 or 1 and `i_chan` is
   * the channel index within the link [0,35].
   */
  Sample channel(int ch) const;
  /// default constructor that does nothing
  SingleROCEventPacket() = default;
};

}  // namespace pflib::packing
