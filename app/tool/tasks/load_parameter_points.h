#pragma once

#include <vector>
#include <string>

/**
 * load a CSV of parameter points into member
 *
 * @param[in] filepath path to CSV containing parameter points
 * @return 2-tuple with parameter names and values
 */
std::tuple<std::vector<std::pair<std::string,std::string>>, std::vector<std::vector<int>>>
load_parameter_points(const std::string& filepath);
