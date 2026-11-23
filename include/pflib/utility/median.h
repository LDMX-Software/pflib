#pragma once

#include <vector>

namespace pflib::utility {

/**
 * find the median of the input vector of samples
 *
 * @param[in] samples list of samples to find median of
 * @return median of the samples
 * @throws pflib::Exception if samples is empty
 */
int median(std::vector<int> samples);

/**
 * find the median of the input vector of samples
 *
 * @param[in] samples list of samples to find median of
 * @return median of the samples
 * @throws pflib::Exception if samples is empty
 */
double median(std::vector<double> samples);

}  // namespace pflib::utility
