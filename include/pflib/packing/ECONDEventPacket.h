#pragma once

#include <array>
#include <span>
#include <bitset>

#include "pflib/Logging.h"
#include "pflib/packing/Reader.h"

namespace pflib::packing {

/**
 * Single ECON-D event packet
 *
 * The ECON-D is the CONcentrator mezzanine for the Data path
 * and it has many different configuration modes. The two most
 * important to us are
 * 1. "Pass Through" (or no Zero Suppression) mode
 * 2. Zero Suppression (ZS) mode
 * 
 * The ECOND_Formatter emulates the ECON-D re-formatting of the HGCROC
 * event packets into the ECON-D event packet. This unpacking is done
 * separately from this formatter so that it can be applied to data
 * actually extracted from an ECON-D.
 */
class ECONDEventPacket {
  mutable ::pflib::logging::logger the_log_{::pflib::logging::get("decoding")};
 
 public:
  class LinkSubPacket {
    mutable ::pflib::logging::logger the_log_{::pflib::logging::get("decoding")};
   public:
    std::array<bool, 1> corruption;
    std::size_t length;
    std::bitset<37> channel_map;
    // temp solution while developing: (i_chan, unpacked data word 16-32 bits)
    std::vector<std::tuple<int,uint32_t>> channel_data;
    /// parse into this package from the passed data span
    void from(std::span<uint32_t> data);
  };

  std::array<bool, 2> corruption;
  std::vector<LinkSubPacket> link_subpackets;

  ECONDEventPacket(std::size_t n_links);

  /// parse into this package from the passed data span
  void from(std::span<uint32_t> data);
  /**
   * read from the input reader into this packet
   *
   * enables us to
   * ```cpp
   * ECONDEventPacket ep;
   * r >> ep;
   * ```
   */
  Reader& read(Reader& r);
};

}
