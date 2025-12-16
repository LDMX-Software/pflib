#pragma once

#include "pflib/Target.h"

/**
 * @namespace pflib::algorithm
 * housing of higher-level methods for repeatable tasks
 */
namespace pflib::algorithm {

/**
 * Find TOT threshold voltage that corresponds to 50% efficiency for a given
 * pulse
 *
 * @param[in] tgt pointer to Target to interact with
 *            roc
 *            calib: target calib
 *            tot_vref: previously set tot_vref
 *
 * @note Only functional for single-ROC targets
 */
std::array<int, 2> tp50_scan(Target* tgt, ROC roc, int& n_events,
        std::array<int, 72>& calibs, std::array<int, 2>& link_vref_list);

}  // namespace pflib::algorithm
