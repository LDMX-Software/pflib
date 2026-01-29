#include "pflib/utility/stdev.h"

#include <cmath>  // std::sqrt, std::pow, std::abs
#include <limits>
#include <numeric>  // std::accumulate

#include "pflib/utility/mean.h"

namespace pflib::utility {

template <typename T>
double stdev_impl(const std::vector<T>& samples) {
  if (samples.size() == 0) {
    return std::numeric_limits<double>::quiet_NaN();
  }
  double mean = pflib::utility::mean(samples);
  double sum = std::accumulate(samples.begin(), samples.end(), 0.0,
                               [mean](double acc, double v) {
                                 return acc + std::pow(std::abs((v - mean)), 2);
                               });
  return std::sqrt(sum / samples.size());
}

double stdev(const std::vector<double>& samples) { return stdev_impl(samples); }

double stdev(const std::vector<int>& samples) { return stdev_impl(samples); }

}  // namespace pflib::utility
