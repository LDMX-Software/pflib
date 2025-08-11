#pragma once

#include <vector>

namespace pflib::utility {

/**
 * find the efficiency of the input vector of samples
 * this is the fraction of samples that are above 0
 * @param[in] samples list of samples to find efficiency of
 * @return efficiency of the samples
 */
double efficiency(std::vector<int> samples);

}
