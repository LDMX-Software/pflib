#pragma once

#include "pflib/packing/ECONDEventPacket.h"
#include "pflib/packing/Reader.h"

namespace pflib::packing {

/**
 * Unpack an event packet that includes an extra header inserted by
 * the software.
 *
 * This software-inserted-header is written in the `read_event` for
 * the HcalBackplaneZCU target.
 *
 * @see ECONDEventPacket for how a single sample from a single ECOND
 * is unpacked. The class contains the more complicated logic and is
 * much more stable since the hardware/firmware on the ECOND is not
 * liable to change as quickly and the firmware/software we are 
 * writing for LDMX.
 */
class SoftWrappedECONDEventPacket {
  /// handle to logging source
  mutable ::pflib::logging::logger the_log_{::pflib::logging::get("decoding")};

 public:
  /// provide number of links (eRx) on the ECOND
  SoftWrappedECONDEventPacket(std::size_t n_links);
  /**
   * Corruption bits
   *
   * Index | Description
   * ------|------------
   *  0    | sw header version mismatch
   */
  std::array<bool, 1> corruption;
  /// L1A index for this packet
  int il1a;
  /// ID specifying ECOND we are reading
  int econ_id;
  /// whether this packet is the sample-of-interest
  bool is_soi;
  /// actual data packet from ECOND
  ECONDEventPacket data;
  /// unpack the given data into this structure
  void from(std::span<uint32_t> frame);
  /// read into this structure from the input Reader
  Reader& read(Reader& r);
};

}  // namespace pflib::packing
