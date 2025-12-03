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
    PFEXCEPTION_RAISE("BadName", "No I2C Bus exists with name "+name);
  }
  return *(it->second);
}

std::vector<std::string> Target::opto_link_names() const {
  std::vector<std::string> names;
  names.reserve(opto_.size());
  for (const auto& [name, _link]: opto_) names.push_back(name);
  return names;
}

OptoLink& Target::get_opto_link(const std::string& name) const {
  auto it{opto_.find(name)};
  if (it == opto_.end()) {
    PFEXCEPTION_RAISE("BadName", "No OptoLink exists with name "+name);
  }
  return *(it->second);
}

}  // namespace pflib
