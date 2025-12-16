#pragma once

#include "pflib/Target.h"

/**
 * @namespace pflib::algorithm
 * housing of higher-level methods for repeatable tasks
 */
namespace pflib::algorithm {

/**
 * Find trim_toa to align TOA for each channel
 *
 * @param[in] tgt pointer to Target to interact with
 *
 * @note Only functional for single-ROC targets
 */
std::array<int, 72> trim_tot_scan(Target* tgt, ROC& roc, int& n_events,
                                  std::array<int, 72>& calibs,
                                  std::array<int, 2>& tot_vrefs,
                                  std::array<int, 72>& tot_trims);

}  // namespace pflib::algorithm
