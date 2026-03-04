#include "pflib/DAQ.h"

namespace pflib {

std::vector<uint32_t> DAQ::read_event_sw_headers() {
  /**
   * this is just a helper function so that we can avoid repeating the
   * emulation of the extra headers produced by the Bittware DAQ FW
   * wrapping an ECOND packet between the HcalBackplane and EcalSMM targets.
   *
   * Besides these emulated headers, it just uses getLinkData to get
   * data and advanceLinkReadPtr after gathering one sample of data.
   */
  std::vector<uint32_t> buf;
  for (int ievt = 0; ievt < samples_per_ror(); ievt++) {
    /// @note only one elink right now
    std::vector<uint32_t> subpacket = getLinkData(0);
    buf.push_back((0x1 << 28) | ((econid() & 0x3ff) << 18) | (ievt << 13) |
                  ((ievt == soi()) ? (1 << 12) : (0)) | (subpacket.size()));
    buf.insert(buf.end(), subpacket.begin(), subpacket.end());
    advanceLinkReadPtr();
  }
  // special trailer word
  /**
   * The Bittware DAQ firmware inserts a trailer word where the ECON ID
   * and the event index ievt (aka l1a index) are set to all ones (1023 and 31)
   * respectively.
   */
  buf.push_back((0x1 << 28) | (0x3ff << 18) | (31 << 13));
  return buf;
}

}  // namespace pflib
