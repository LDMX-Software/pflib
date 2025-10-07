#include "pflib/GPIO.h"

#include <stdio.h>

#include "pflib/Exception.h"

namespace pflib {

bool GPIO::hasGPO(const std::string& name) {
  std::vector<std::string> names = getGPOs();
  for (auto i : names) {
    if (i == name) return true;
  }
  return false;
}

bool GPIO::hasGPI(const std::string& name) {
  std::vector<std::string> names = getGPIs();
  for (auto i : names) {
    if (i == name) return true;
  }
  return false;
}

}  // namespace pflib
