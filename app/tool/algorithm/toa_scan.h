#pragma once

#include "pflib/Target.h"

/**
 * @namespace pflib::algorithm
 * housing of higher-level methods for repeatable tasks
 */
namespace pflib::algorithm {

/**
 * Find toa_vref per link
 * Find trim_toa per channel
 *
 * @param[in] tgt pointer to Target to interact with
 *
 */
std::map<int, std::map<std::string, std::map<std::string, uint64_t>>> toa_scan(
    Target* tgt);

}  // namespace pflib::algorithm
