#include "pflib/utility/mean.h"

#include <algorithm>


namespace pflib::utility {

template <typename T>
double median(std::vector<T> samples) {
  // The mean is the sum over the size of the samples
  static_assert(std::is_same<T, int> || std::is_same<T, double>,
                "Type of samples should be int or double"); 
  int n = samples.size();
  if (n == 0) {
    throw "Trying to get the mean of an empty vector";
  }
  double sum = std::accumulate(samples.begin(), samples.end(), 0);
  double mean = sum / n;
  return mean;
}

}  // namespace pflib::utility
