#pragma once

#include <array>
#include <span>

#include "pflib/packing/Reader.h"
#include "pflib/packing/DAQLinkFrame.h"
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
struct SingleROCEventPacket {
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
  /// default constructor that does nothing
  SingleROCEventPacket() = default;
};

}
