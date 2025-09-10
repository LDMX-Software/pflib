#pragma once

#include <vector>

namespace pflib::utility {

/**
 * find the median of the input vector of samples
 *
 * @param[in] samples list of samples to find median of
 * @return median of the samples
 */
int median(std::vector<int> samples);

}  // namespace pflib::utility
