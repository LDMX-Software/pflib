#include "pflib/packing/ECONTCaptureHeader.h"

namespace pflib::packing {

void ECONTCaptureHeader::from(std::span<uint32_t> data) {
  if (data.size() < 2) {
    pflib_log(error)
        << "ECON-T capture packet header requires 2 words but recieved "
        << data.size();
    return;
  }
  /**
   * two 32b headers are added by the firmware
   *
   * first word
   * - [32:28] = version
   * - [27:18] = econ_id
   * - [7:0] = size
   *
   * second word
   * - [27:24] = number of links
   * - [22:18] = number of presamples
   * - [17:12] = number of samples
   */
  version_ = ((data[0] >> 28) & 0xf);
  econ_id_ = ((data[0] >> 18) & 0x3ff);
  length_ = (data[0] & 0xff);
  n_links_ = ((data[1] >> 24) & 0xf);
  pre_samples_ = ((data[1] >> 18) & 0x1f);
  n_samples_ = ((data[1] >> 12) & 0x3f);
  if (length_ != 2 + n_links_ * n_samples_) {
    pflib_log(warn) << "First header reports a length of " << length_
                    << " which does not equal the expected value for STC4";
  }
}

int ECONTCaptureHeader::version() const { return version_; }

int ECONTCaptureHeader::econ_id() const { return econ_id_; }

int ECONTCaptureHeader::length() const { return length_; }

int ECONTCaptureHeader::n_links() const { return n_links_; }

int ECONTCaptureHeader::pre_samples() const { return pre_samples_; }

int ECONTCaptureHeader::n_samples() const { return n_samples_; }
}  // namespace pflib::packing
