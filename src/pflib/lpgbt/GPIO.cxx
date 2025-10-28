#include "pflib/lpgbt/GPIO.h"

#include "pflib/lpGBT.h"
#include "pflib/utility/string_format.h"

namespace pflib {
namespace lpgbt {

std::vector<std::string> GPIO::getGPOs() {
  std::vector<std::string> retval;
  for (auto item : gpos_) retval.push_back(item.first);
  return retval;
}

std::vector<std::string> GPIO::getGPIs() {
  std::vector<std::string> retval;
  for (auto item : gpis_) retval.push_back(item.first);
  return retval;
}
bool GPIO::getGPI(const std::string& name) {
  auto ptr = gpis_.find(name);
  if (ptr == gpis_.end()) {
    PFEXCEPTION_RAISE("GPIOError", pflib::utility::string_format(
                                       "Unknown GPI bit '%s'", name.c_str()));
  }
  int ibit = ptr->second;
  return lpgbt_.gpio_get(ibit);
}
bool GPIO::getGPO(const std::string& name) {
  auto ptr = gpos_.find(name);
  if (ptr == gpos_.end()) {
    printf("lpGBT Available GPOs:\n");
    for (const auto& name : getGPOs()) {
      printf("  %s\n", name.c_str());
    }
    PFEXCEPTION_RAISE("GPIOError", pflib::utility::string_format(
                                       "Unknown GPO bit '%s'", name.c_str()));
  }
  int ibit = ptr->second;
  return lpgbt_.gpio_get(ibit);
}
void GPIO::setGPO(const std::string& name, bool toTrue) {
  auto ptr = gpos_.find(name);
  if (ptr == gpos_.end()) {
    printf("lpGBT Available GPOs:\n");
    for (const auto& name : getGPOs()) {
      printf("  %s\n", name.c_str());
    }
    PFEXCEPTION_RAISE("GPIOError", pflib::utility::string_format(
                                       "Unknown GPO bit '%s'", name.c_str()));
  }
  int ibit = ptr->second;
  lpgbt_.gpio_set(ibit, toTrue);
}

void GPIO::add_pin(const std::string& name, int ibit, bool output) {
  // remove this pin number or name if used anywhere
  for (auto cur = gpos_.begin(); cur != gpos_.end();) {
    if (cur->second == ibit || cur->first == name)
      cur = gpos_.erase(cur);
    else
      cur++;
  }
  for (auto cur = gpis_.begin(); cur != gpis_.end();) {
    if (cur->second == ibit || cur->first == name)
      cur = gpis_.erase(cur);
    else
      cur++;
  }
  if (output)
    gpos_[name] = ibit;
  else
    gpis_[name] = ibit;
}

}  // namespace lpgbt
}  // namespace pflib
