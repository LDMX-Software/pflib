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
std::pair<
  std::vector<int>,
  std::vector<std::pair<int,int>>
> invertRocErxMapping(Target* tgt) {
  std::vector<std::pair<int,int>> erx_to_roc_half(2*tgt->getRocErxMapping().size());
  std::vector<int> active_erx;
  int i_link_offset{100};
  for (int i_roc{0}; i_roc < tgt->getRocErxMapping().size(); i_roc++) {
    auto [erx_half_0, erx_half_1] = tgt->getRocErxMapping().at(i_roc);
    erx_to_roc_half[erx_half_0] = std::make_pair(i_roc, 0);
    erx_to_roc_half[erx_half_1] = std::make_pair(i_roc, 1);
    if (tgt->have_roc(i_roc)) {
      active_erx.push_back(erx_half_0);
      active_erx.push_back(erx_half_1);
    }
  }
  std::sort(active_erx.begin(), active_erx.end());
  return std::make_pair(active_erx, erx_to_roc_half);
}

std::pair<int, int> Target::toECONChannel(int i_roc, int channel) {
  int half = channel % 36;
  int link_channel = channel / 36;
  const auto& [half_0_eRx, half_1_eRx] = getRocErxMapping().at(i_roc);
  return std::make_pair(
      (half == 0) ? half_0_eRx : half_1_eRx,
      link_channel
  );
}

std::pair<int, int> Target::toROCChannel(int i_link, int channel) {
  static const auto [active_erx, erx_to_roc_half] = invertRocErxMapping(this);
  auto [iroc, half] = erx_to_roc_half.at(active_erx.at(i_link));
  return std::make_pair(iroc, channel + half*36);
}

}  // namespace pflib
