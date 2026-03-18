#include "pflib/utility/mean.h"

#include <limits>
#include <numeric>  // std::accumulate

namespace pflib::utility {

template <typename T>
double mean_impl(const std::vector<T>& samples) {
  if (samples.size() == 0) {
    return std::numeric_limits<double>::quiet_NaN();
  }
  return std::accumulate(samples.begin(), samples.end(), 0.0) / samples.size();
}

double mean(const std::vector<double>& samples) { return mean_impl(samples); }

double mean(const std::vector<int>& samples) { return mean_impl(samples); }

}  // namespace pflib::utility
