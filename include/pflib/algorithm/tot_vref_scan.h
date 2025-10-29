#pragma once

#include "pflib/Target.h"

/**
 * @namespace pflib::algorithm
 * housing of higher-level methods for repeatable tasks
 */
namespace pflib::algorithm {

/**
 * Find TOT threshold value
 *
 * @param[in] tgt pointer to Target to interact with
 *
 * @note Only functional for single-ROC targets
 */
std::map<std::string, std::map<std::string, uint64_t>> tot_vref_scan(
    Target* tgt, ROC roc);

}  // namespace pflib::algorithm
