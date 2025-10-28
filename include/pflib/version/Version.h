/**
 * pflib Version file copying parameters in from the CMake build configuration
 */

#pragma once

#include <string>

/**
 * namespace holding functions for printing/getting version
 */
namespace pflib::version {

/**
 * get the plain tag version of pflib
 */
std::string tag();

/**
 * get the full git describe of pflib
 */
std::string git_describe();

/**
 * get the full debug version of pflib
 *
 * Format: `tag` (`git_describe`)
 */
std::string debug();

}  // namespace pflib::version
