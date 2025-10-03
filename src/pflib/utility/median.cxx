#include "pflib/utility/median.h"

namespace pflib::utility {

int median(std::vector<int> samples) {
  auto halfway = samples.size() / 2;
  /// we partially sort the vector in order to find the median
  std::nth_element(samples.begin(), samples.begin() + halfway, samples.end());
  /// we don't bother to see if the median should be between two numbers
  return samples[halfway];
}

double median(std::vector<double> samples) {
  auto halfway = samples.size() / 2;
  /// we partially sort the vector in order to find the median
  std::nth_element(samples.begin(), samples.begin() + halfway + 1, samples.end());
  if (samples.size() % 2 == 0) {
    /// for even-number sets, median is halfway between the two central values
    return (samples[halfway] + samples[halfway+1])/2;
  } else {
    /// for odd-number sets, there is a central value
    return samples[halfway];
  }
}

}  // namespace pflib::utility
