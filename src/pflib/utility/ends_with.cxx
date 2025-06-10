#include "pflib/utility/ends_with.h"

namespace pflib::utility {

bool ends_with(const std::string& full, const std::string& ending) {
  if (full.length() < ending.length()) return false;
  return (0 == full.compare(full.length() - ending.length(), ending.length(),
                            ending));
}

}  // namespace pflib::utility
