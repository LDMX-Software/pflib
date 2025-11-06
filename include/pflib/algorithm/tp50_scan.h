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
int tp50_scan(Target* tgt, ROC roc, std::array<int,72> calib, int tot_vref);

}  // namespace pflib::algorithm
