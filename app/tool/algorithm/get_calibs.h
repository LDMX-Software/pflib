#pragma once

#include "pflib/Target.h"

/**
 * @namespace pflib::algorithm
 * housing of higher-level methods for repeatable tasks
 */
namespace pflib::algorithm {

/**
 * Find calib value where the max adc corresponds to a target value
 *
 * @param[in] tgt pointer to Target to interact with
 *            roc for setting params
 *            target calib value
 */
template <class EventPacket>
std::array<int, 72> get_calibs(Target* tgt, ROC& roc, int& target_adc);

}  // namespace pflib::algorithm
