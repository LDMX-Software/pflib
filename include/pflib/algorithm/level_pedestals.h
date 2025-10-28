#pragma once

#include "pflib/Target.h"

/**
 * @namespace pflib::algorithm
 * housing of higher-level methods for repeatable tasks
 */
namespace pflib::algorithm {

/**
 * Level pedestals so that they are all within noise of their link median
 *
 * @param[in] tgt pointer to Target to interact with
 *
 * @note Only functional for single-ROC targets
 */
std::map<std::string, std::map<std::string, uint64_t>> level_pedestals(
    Target* tgt, ROC roc);

}  // namespace pflib::algorithm
