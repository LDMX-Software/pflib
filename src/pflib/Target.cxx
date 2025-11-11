#include "pflib/Target.h"

namespace pflib {

std::vector<std::string> Target::i2c_bus_names() {
  std::vector<std::string> names;
  for (auto items : i2c_) names.push_back(items.first);
  return names;
}

I2C& Target::get_i2c_bus(const std::string& name) { return *(i2c_[name]); }


}  // namespace pflib
