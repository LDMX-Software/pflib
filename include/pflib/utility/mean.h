#pragma once

#include <vector>

namespace pflib::utility {

/**
 * find the mean of the input vector of samples
 *
 * @param[in] samples list of samples to find mean of
 * @return mean of the samples
 */
template <typename T>
double median(std::vector<T> samples);

}  // namespace pflib::utility
