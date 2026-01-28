#include "pflib/utility/median.h"

#include <algorithm>

namespace pflib::utility {

template<typename T>
double median(std::vector<T> samples) {
  static_assert(std::is_same<T, int> || std::is_same<T, double>,
                "Type of samples should be int or double");
  double sum = std::accumulate(samples.begin(), samples.end(), 0);
  int n = samples.size(); 
  double mean = pflib::utility::mean(samples);
  double d2 = std::pow(std::abs(samples - mean), 2);
  double sum = std::accumulate(d2.begin(), d2.end(), 0);
  double stdev = std::sqrt(sum);
  return stdev;
}

}  // namespace pflib::utility
