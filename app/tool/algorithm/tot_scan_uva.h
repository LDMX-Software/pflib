#pragma once

#include "pflib/Target.h"

/**
 * @namespace pflib::algorithm
 * housing of higher-level methods for repeatable tasks
 */
namespace pflib::algorithm {

/**
 * Find tot_vref per link and trim_tot per channel
 *
 * @param[in] tgt pointer to Target to interact with
 *
 */
std::map<int, std::map<std::string, std::map<std::string, uint64_t>>> tot_scan_uva(
    Target* tgt);

}  // namespace pflib::algorithm
