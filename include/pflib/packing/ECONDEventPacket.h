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
 * and it has many different configuration modes.
 * The two most important to us for decoding are
 * 1. "Pass Through" mode - all 37 sample words are copied from the link,
 *    the link headers from the ROC are overwritten by ECOND link headers
 * 2. "Regular" mode - the ECOND event format is written which can include
 *    zero-suppression of multiple varieties including full eliminating
 *    channels that are empty
 * 
 * The ECOND_Formatter emulates the ECON-D re-formatting of the HGCROC
 * event packets into the ECON-D event packet. This unpacking is done
 * separately from this formatter so that it can be applied to data
 * actually extracted from an ECON-D.
 */
class ECONDEventPacket {
  /// handle to logging source
  mutable ::pflib::logging::logger the_log_{::pflib::logging::get("decoding")};

  /**
   * Unpack a eRx ("link") subpacket from the ECOND event into a DAQLinkFrame
   *
   * @note We overload the zero'th corruption bit for DAQLinkFrame to mean
   * that any of the three Stat bits are bad (not 1).
   *
   * @param[in] data subpacket data to unpack
   * @param[out] link DAQLinkFrame to update with unpacked data
   * @paran[in] passthrough whether the ECOND is in pass through mode or not
   * @return std::size_t length of data in 32b words that was unpacked
   */
  std::size_t unpack_link_subpacket(std::span<uint32_t> data, DAQLinkFrame &link, bool passthrough);

 public:
  /**
   * storage of corruption bits
   *
   * Index | Description
   * ------|------------
   *  0    | 9b header marker is not the ECOND spec or common CMS config
   *  1    | stored length does not equal packet length
   *  2    | 8b header CRC mismatch
   *  3    | 32b sub-packet CRC mismatch
   */
  std::array<bool, 4> corruption;

  /**
   * storage of unpacked link data
   *
   * This is very space in-efficient since we store zero words for 
   * any channel or measurement that was zero suppressed.
   * This won't be a problem for the smaller-scale testbeam prototypes,
   * but could be a memory issue when we get to the full LDMX
   * detector with its almost 100k channels.
   */
  std::vector<DAQLinkFrame> links;

  /**
   * Construct an event packet with the number of eRx "links"
   */
  ECONDEventPacket(std::size_t n_links);

  /**
   * parse into the packet from the passed data span
   *
   * This function is what implements the format describe
   * in the ECOND specification.
   *
   * @see read for wrapping this function with more unpacking
   * to handle the extra headers and trailers that are added
   * by software emulation or hardware.
   */
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
