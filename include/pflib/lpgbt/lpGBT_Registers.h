#ifndef pflib_lpGBT_Registers_h_included
#define pflib_lpGBT_Registers_h_included

#include <stdint.h>

#include <string>

namespace pflib {

namespace lpgbt {

/** Find an lpGBT register by name and its information in the table of registers
 */
bool findRegister(const std::string& registerName, uint16_t& addr,
                  uint8_t& mask, uint8_t& offset);

}  // namespace lpgbt

}  // namespace pflib

#endif  // pflib_lpGBT_Registers_h_included
