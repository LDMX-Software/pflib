#pragma once

#include "pflib/Target.h"

/**
 * @namespace pflib::algorithm
 * housing of higher-level methods for repeatable tasks 
 */
 namespace pflib::algorithm {

/**
 * Find TOA threshold voltage reference to align TOA values
 *
 * @param[in] tgt pointer to Target to interact with
 *
 * @note Only functional for single-ROC targets
 */
std::map<std::string, std::map<std::string, int>>
find_toa_vref(Target* tgt, ROC roc);

}