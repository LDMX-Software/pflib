#include "pflib/DAQ.h"

#include "pflib/packing/DAQSampleHeader.h"

namespace pflib {

DAQ::DAQ(int links) : n_links{links}, econid_{0xFFF}, samples_{1}, soi_{0} {}

void DAQ::setup(int econid, int samples_per_ror, int soi) {
  econid_ = econid;
  samples_ = samples_per_ror;
  soi_ = (soi < 0 || soi > samples_ - 1) ? (0) : soi;
}

int DAQ::econid() const { return econid_; }
int DAQ::samples_per_ror() const { return samples_; }
int DAQ::soi() const { return soi_; }
void DAQ::enable(bool enable) { enabled_ = enable; }
bool DAQ::enabled() { return enabled_; }
int DAQ::nlinks() const { return n_links; }
bool DAQ::AXIS_enabled() { return false; }
void DAQ::AXIS_enable(bool enable) {}

std::map<std::string, uint32_t> DAQ::get_debug(uint32_t ask) {
  return std::map<std::string, uint32_t>();
}

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
    pflib::packing::DAQSampleHeader header{
        .version = 1,
        .econd_id = static_cast<uint32_t>(econid()),
        .i_l1a = static_cast<uint32_t>(ievt),
        .is_soi = (ievt == soi()),
        .econd_len = static_cast<uint32_t>(subpacket.size())};
    buf.push_back(header.to());
    buf.insert(buf.end(), subpacket.begin(), subpacket.end());
    advanceLinkReadPtr();
  }
  // special trailer word
  /**
   * The Bittware DAQ firmware inserts a trailer word where the ECON ID
   * and the event index ievt (aka l1a index) are set to all ones (1023 and 31)
   * respectively.
   */
  buf.push_back(pflib::packing::DAQSampleHeader::ending_trailer());
  return buf;
}

}  // namespace pflib
