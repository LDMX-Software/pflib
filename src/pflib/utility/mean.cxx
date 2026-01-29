#include "pflib/utility/mean.h"

#include <numeric>  // std::accumulate

namespace pflib::utility {

template<typename T>
double mean(const std::vector<T>& samples) {
  if (samples.size() == 0) {
    return std::numeric_limits<double>::nan();
  }
  return std::accumulate(samples.begin(), samples.end(), 0.0) / samples.size();
}

double mean(const std::vector<double>& samples) {
  return mean(samples);
}

double mean(const std::vector<int>& samples) {
  return mean(samples);
}

}  // namespace pflib::utility
