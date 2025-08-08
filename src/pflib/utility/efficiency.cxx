#include "pflib/utility/efficiency.h"

namespace pflib::utility {

double efficiency(std::vector<int> samples) {
  /// find number of non-zero samples divided by total number of samples in vector
  double eff = std::count_if(samples.begin(), samples.end(), [](int sample) { return sample > 0; }) / static_cast<double>(samples.size());
  // if (samples.empty()) {
  //   pflib_log(warn) << "No data in sample; efficiency set to 0";
  //   eff = 0.0;
  // }
  return eff;
  }
}