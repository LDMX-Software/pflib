#include "pflib/utility/median.h"

namespace pflib::utility {

int median(std::vector<int> samples) {
  auto halfway = samples.size()/2;
  /// we partially sort the vector in order to find the median
  std::nth_element(samples.begin(), samples.begin()+halfway, samples.end());
  /// we don't bother to see if the median should be between two numbers
  return samples[halfway];
}

}
