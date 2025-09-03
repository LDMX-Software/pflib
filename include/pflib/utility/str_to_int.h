/**
 * @file str_to_int.h
 * definition of str_to_int function
 */
#pragma once

#include <string>

namespace pflib::utility {

/**
 * Get an integer from the input string
 *
 * The normal stoi (and similar) tools don't support binary inputs
 * which are helpful in our case where sometimes the value is set
 * in binary but each bit has a non-base-2 scale.
 *
 * Supported prefixes:
 * - `0b` --> binary
 * - `0x` --> hexidecimal
 * - `0`  --> octal
 * - none of the above --> decimal
 *
 * @param[in] str string form of integer
 * @return integer decoded from string
 */
int str_to_int(std::string str);

}  // namespace pflib::utility
