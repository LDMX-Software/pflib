#include "pflib/utility/mean.h"

#include <numeric>   // std::accumulate

namespace pflib::utility {

double mean(std::vector<int> samples) {
  // The mean is the sum over the size of the samples
  if (samples.size() == 0) {
    throw "Trying to get the mean of an empty vector";
  }
  return std::accumulate(samples.begin(), samples.end(), 0.0) / samples.size();
}

double mean(std::vector<double> samples) {
  // The mean is the sum over the size of the samples
  if (samples.size() == 0) {
    throw "Trying to get the mean of an empty vector";
  }
  return std::accumulate(samples.begin(), samples.end(), 0.0) / samples.size();
}

}  // namespace pflib::utility
