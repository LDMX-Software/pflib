#include "pflib/Target.h"

namespace pflib {

std::vector<std::string> Target::i2c_bus_names() {
  std::vector<std::string> names;
  names.reserve(i2c_.size());
  for (const auto& [name, _i2c] : i2c_) names.push_back(name);
  return names;
}

I2C& Target::get_i2c_bus(const std::string& name) {
  auto it{i2c_.find(name)};
  if (it == i2c_.end()) {
    PFEXCEPTION_RAISE("BadName", "No I2C Bus exists with name " + name);
  }
  return *(it->second);
}

std::vector<std::string> Target::opto_link_names() const {
  std::vector<std::string> names;
  names.reserve(opto_.size());
  for (const auto& [name, _link] : opto_) names.push_back(name);
  return names;
}

OptoLink& Target::get_opto_link(const std::string& name) const {
  auto it{opto_.find(name)};
  if (it == opto_.end()) {
    PFEXCEPTION_RAISE("BadName", "No OptoLink exists with name " + name);
  }
  return *(it->second);
}

/**
 * Invert the ROC -> eRx pair mapping including which ROCs are active
 *
 * As far as I (Tom) know, the data is packed into the ECON-D packet
 * in eRx order which, depending on the wiring of the board connecting
 * the ROCs to the ECON-D, can lead to the "first" link not being eRx 0.
 * This means we need to do two "mappings" to get back the ROC-half a
 * specific link from the decoded data applies to.
 * 1. Map from link index (i_link) to the eRx of the ECON-D
 *    (e.g. 0 -> 2 for HGCROC0 on the HcalBackplane)
 * 2. Map from eRx of the ECON-D to the ROC-half
 *
 * For example, if HGCROC0 is active on an HcalBackplane, its pair
 * of output links are mapped to eRx (3, 2) which are then packed
 * into the ECONDEventPacket in eRx order and unpacked with i_link (0, 1).
 * i_link 0 was eRx 2 which was the upper half (half 1) of HGCROC0.
 *
 * Since we are dealing with re-mapping of indices and integers, I don't
 * use std::map - I just use std::vector (but I'm still thinking of them
 * as maps where the "keys" are the vector indices).
 */
void Target::invertRocErxMapping() {
  erx_to_roc_half_.resize(2*getRocErxMapping().size());
  i_erx_to_erx_.clear();
  for (int i_roc{0}; i_roc < getRocErxMapping().size(); i_roc++) {
    auto [erx_half_0, erx_half_1] = getRocErxMapping().at(i_roc);
    erx_to_roc_half_[erx_half_0] = std::make_pair(i_roc, 0);
    erx_to_roc_half_[erx_half_1] = std::make_pair(i_roc, 1);
    if (have_roc(i_roc)) {
      i_erx_to_erx_.push_back(erx_half_0);
      i_erx_to_erx_.push_back(erx_half_1);
    }
  }
  std::sort(i_erx_to_erx_.begin(), i_erx_to_erx_.end());
  // maximum of 12 eRx for an ECON
  erx_to_i_erx_.resize(12);
  // mark inactive eRx with a negative index
  for (int& i_erx : erx_to_i_erx_) {
    i_erx = -1;
  }
  // invert map of indexed eRx
  for (std::size_t i_erx{0}; i_erx < i_erx_to_erx_.size(); i_erx++) {
    erx_to_i_erx_.at(i_erx_to_erx_[i_erx]) = i_erx;
  }
}

std::pair<int, int> Target::toErxChannel(int i_roc, int channel) {
  assert(channel >= 0 && channel < 72);
  int half = channel / 36;
  int channel_in_erx = channel % 36;
  const auto& [half_0_eRx, half_1_eRx] = getRocErxMapping().at(i_roc);
  return std::make_pair(
      erx_to_i_erx_.at((half == 0) ? half_0_eRx : half_1_eRx),
      channel_in_erx
  );
}

std::pair<int, int> Target::toROCChannel(int i_erx, int channel) {
  assert(channel >= 0 and channel < 36);
  if (i_erx_to_erx_.empty()) {
    invertRocErxMapping();
  }
  auto [iroc, half] = erx_to_roc_half_.at(i_erx_to_erx_.at(i_erx));
  return std::make_pair(iroc, channel + half*36);
}

}  // namespace pflib
