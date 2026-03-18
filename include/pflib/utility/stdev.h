#pragma once

#include <vector>

namespace pflib::utility {

/**
 * find the stdev of the input vector of samples
 *
 * @param[in] samples list of samples to find stdev of
 * @return stdev of the samples
 *
 * Method based on the method used by numpy.std. See numpy.std documentation.
 */
double stdev(const std::vector<int>& samples);

/**
 * find the stdev of the input vector of samples
 *
 * @param[in] samples list of samples to find stdev of
 * @return stdev of the samples
 *
 * Method based on the method used by numpy.std. See numpy.std documentation.
 */
double stdev(const std::vector<double>& samples);

}  // namespace pflib::utility
