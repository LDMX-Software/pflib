#include "pflib/DAQ.h"

namespace pflib {

std::vector<uint32_t> DAQ::read_event_sw_headers() {
  /**
   * this is just a helper function so that we can avoid repeating the
   * emulation of the extra headers wrapping an ECOND packet between
   * the HcalBackplane and EcalSMM targets.
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
   * TODO: update to match FW, I just set ievt, soi, and subpacket size to zero
   */
  buf.push_back((0x1 << 28) | ((econid() & 0x3ff) << 18));
  return buf;
}

}  // namespace pflib
