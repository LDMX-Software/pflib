#ifndef pflib_lpGBT_Utility_h_included
#define pflib_lpGBT_Utility_h_included

#include <stdint.h>

#include <string>

#include "pflib/lpGBT.h"

namespace pflib {
namespace lpgbt {

/** Load a CSV file containing register names or addresses and values.
 * Converts into a RegisterValueVector, which can use only full
 * registers, not partial registers.
 * Expects two columns, register and value.
 * Ignores hash signs and anything following on the same line.
 */
lpGBT::RegisterValueVector loadRegisterCSV(const std::string& filename);

/** Apply a CSV containing register names or addresses and values.
 *  Requires an lpGBT object to operate on, can use partial registers.
 * Expects two columns, register and value.
 * Ignores hash signs and anything following on the same line.
 */
void applylpGBTCSV(const std::string& filename, lpGBT& lpgbt);

}  // namespace lpgbt
}  // namespace pflib

#endif  // pflib_lpGBT_Utility_h_included
