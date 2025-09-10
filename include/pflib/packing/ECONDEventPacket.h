#pragma once

#include <array>
#include <span>
#include <bitset>

#include "pflib/Logging.h"
#include "pflib/packing/Reader.h"
#include "pflib/packing/DAQLinkFrame.h"

namespace pflib::packing {

/**
 * Single ECON-D event packet
 *
 * The ECON-D is the CONcentrator mezzanine for the Data path
 * and it has many different configuration modes. The two most
 * important to us are
 * 1. "Pass Through" mode
 * 2. Zero Suppression (ZS) mode
 * 
 * The ECOND_Formatter emulates the ECON-D re-formatting of the HGCROC
 * event packets into the ECON-D event packet. This unpacking is done
 * separately from this formatter so that it can be applied to data
 * actually extracted from an ECON-D.
 */
class ECONDEventPacket {
  mutable ::pflib::logging::logger the_log_{::pflib::logging::get("decoding")};
  std::size_t unpack_link_subpacket(std::span<uint32_t> data, DAQLinkFrame &link);
 public:
  std::array<bool, 4> corruption;
  std::vector<DAQLinkFrame> link_subpackets;

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
  /// header string if using to_csv
  static const std::string to_csv_header;
  /**
   * write current packet into a CSV
   *
   * @param[in,out] f file to write CSV to
   */
  void to_csv(std::ofstream& f) const;
};

}
