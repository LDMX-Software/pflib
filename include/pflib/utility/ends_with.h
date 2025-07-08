/**
 * @file ends_with.h
 */
#pragma once

#include <string>

/**
 * @namespace pflib::utility
 *
 * Dumping ground for various functions that are used in many places
 */
namespace pflib::utility {

/**
 * Check if a given string has a specific ending
 *
 * @param[in] full string to check
 * @param[in] ending string to look for
 * @return true if full's last characters match ending
 */
bool ends_with(const std::string& full, const std::string& ending);

}  // namespace pflib::utility
