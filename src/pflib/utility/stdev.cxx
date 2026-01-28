#include "pflib/utility/stdev.h"

#include <cmath>    // std::sqrt, std::pow, std::abs
#include <numeric>  // std::accumulate

#include "pflib/utility/mean.h"

namespace pflib::utility {

double stdev(std::vector<int> samples) {
  int n = samples.size();
  if (n == 0) {
    throw "Trying to take the stdev of an empty vector";
  }
  double mean = pflib::utility::mean(samples);
  std::vector<double> d2;
  for (int i = 0; i < n; i++) {
    d2.push_back(std::pow(std::abs(samples[i] - mean), 2));
  }
  return std::sqrt(std::accumulate(d2.begin(), d2.end(), 0.0) / n);
}

double stdev(std::vector<double> samples) {
  int n = samples.size();
  if (n == 0) {
    throw "Trying to take the stdev of an empty vector";
  }
  double mean = pflib::utility::mean(samples);
  std::vector<double> d2;
  for (int i = 0; i < n; i++) {
    d2.push_back(std::pow(std::abs(samples[i] - mean), 2));
  }
  return std::sqrt(std::accumulate(d2.begin(), d2.end(), 0.0) / n);
}

}  // namespace pflib::utility
