#include "pflib/utility/efficiency.h"

#include <algorithm>
#include <limits>  // need for "numeric_limits" to work in conditional below.

namespace pflib::utility {

double efficiency(std::vector<int> samples) {
  /// find number of non-zero samples divided by total number of samples in
  /// vector
  if (samples.size() == 0) {
    return std::numeric_limits<double>::quiet_NaN();
  }
  double eff = std::count_if(samples.begin(), samples.end(),
                             [](int sample) { return sample > 0; }) /
               static_cast<double>(samples.size());
  return eff;
}
}  // namespace pflib::utility
