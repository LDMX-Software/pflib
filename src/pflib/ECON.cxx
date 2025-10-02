#include "pflib/ECON.h"

#include <yaml-cpp/yaml.h>

#include <fstream>
#include <iostream>

#include "pflib/packing/Hex.h"
#include "pflib/utility/load_integer_csv.h"

namespace pflib {

ECON::ECON(I2C& i2c, uint8_t econ_base_addr, const std::string& type_version)
    : i2c_{i2c},
      econ_base_{econ_base_addr},
      compiler_{Compiler::get(type_version)} {
  pflib_log(debug) << "base addr " << packing::hex(econ_base_);
}
  
void ECON::setRunMode(bool active) {
  // TODO: implement writing run mode to the hardware
  uint8_t reg_value = getValue(0x03c5, 3);
  const uint32_t MASK_RUN_BIT  = 0x000F;
  const int      SHIFT_RUN_BIT = 0;
  uint32_t param = (reg_value & MASK_RUN_BIT) >> SHIFT_RUN_BIT;
  //if (active) cval |= MASK_RUN_MODE;
  //setValue(TOP_PAGE, 0, cval);
}

bool ECON::isRunMode() {
  // TODO: implement reading run mode from the hardware
  uint8_t reg_value = getValue(0x03c5, 3);
  const uint32_t MASK_RUN_BIT  = 0x000F;
  const int      SHIFT_RUN_BIT = 0;
  uint32_t param = (reg_value & MASK_RUN_BIT) >> SHIFT_RUN_BIT;
  return false;  // placeholder
}
  
uint8_t ECON::getValue(int addr, int nbytes) {
  if(nbytes < 1) {
    pflib_log(error) << "Invalid nbytes = " << nbytes;
  }
  
  pflib_log(debug) << "ECON::getValue(" << addr << ", " << nbytes << ")";

  std::vector<uint8_t> waddr;
  waddr.push_back(static_cast<uint8_t>(addr & 0xFF));       
  waddr.push_back(static_cast<uint8_t>((addr >> 8) & 0xFF));

  std::vector<uint8_t> data = i2c_.general_write_read(econ_base_, waddr, nbytes);
  for (size_t i = 0; i < data.size(); i++) {
    printf("%02zu : %02x\n", i, data[i]);
  }
}

void ECON::setValue(int page, int offset, uint8_t value) {
  // TODO: implement writing a register to the hardware
}
  
}  // namespace pflib
