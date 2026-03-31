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

std::vector<int> invertRocErxMapping(const std::vector<std::pair<int,int>> roc_to_erx) {
  std::vector<int> erx_to_roc(2*roc_to_erx.size());
  int i_link_offset{100};
  for (int i_roc{0}; i_roc < roc_to_erx.size(); i_roc++) {
    for (int i_link : roc_to_erx.at(i_roc)) {
      if (has_roc(i_roc)) {
        erx_to_roc[i_link] = i_roc;
        if (i_link < i_link_offset) {
          i_link_offset = i_link;
        }
      } else {
        erx_to_roc[i_link] = -1;
      }
    }
  }
  return erx_to_roc;
}

std::pair<int, int> toROCChannel(int i_link, int channel) {
  static const erx_to_roc{invertRocErxMapping(getRocErxMapping())};

}

}  // namespace pflib
