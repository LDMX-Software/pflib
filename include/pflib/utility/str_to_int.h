/**
 * @file str_to_int.h
 * definition of str_to_int function
 */
#pragma once

#include <string>

namespace pflib::utility {

/**
 * Check if the input string matches an integer
 *
 * @param[in] str string that could be an integer
 * @return true if string could be an unsigned
 * integer represented in binary, hex, octal, or decimal.
 */
bool is_integer(const std::string& str);

/**
 * Get an integer from the input string
 *
 * The normal stoi (and similar) tools don't support binary inputs
 * which are helpful in our case where sometimes the value is set
 * in binary but each bit has a non-base-2 scale or where the
 * value is a bit-map and its helpful to see exactly which bits
 * are set.
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
unsigned long long int str_to_ullint(std::string str);

int str_to_int(std::string str);

}  // namespace pflib::utility
