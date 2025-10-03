#include "pflib/ECON.h"

#include <yaml-cpp/yaml.h>

#include <fstream>
#include <iostream>

#include "pflib/packing/Hex.h"
#include "pflib/utility/load_integer_csv.h"

namespace pflib {

ECON::ECON(I2C& i2c, uint8_t econ_base_addr, const std::string& type_version)
    : i2c_{i2c},
      econ_base_{econ_base_addr}
      //compiler_{Compiler::get(type_version)}
{
  pflib_log(debug) << "base addr " << packing::hex(econ_base_);
}
  
void ECON::setRunMode(bool active) {
  // TODO: implement writing run mode to the hardware
}

uint32_t getParam(const std::vector<uint8_t>& data, size_t shift, uint32_t mask) {
  // combine bytes into a little-endian integer
  // data[0] is the least significant byte, etc
  uint64_t value = 0;
  for (size_t i = 0; i < data.size(); ++i) {
    value |= (static_cast<uint64_t>(data[i]) << (8 * i));
  }
  
  // now extract the field
  return (value >> shift) & mask;
}

std::vector<uint8_t> newParam(std::vector<uint8_t> prev_value, uint16_t reg_addr, int nbytes, uint32_t mask, int shift, uint32_t value) {
  // combine bytes into a single integer
  uint32_t reg_val = 0;
  for (int i = 0; i < nbytes; ++i) {
    reg_val |= static_cast<uint32_t>(prev_value[i]) << (8 * i);
  }

  reg_val &= ~(mask << shift);
  reg_val |= ((value & mask) << shift);

  // split back into bytes
  std::vector<uint8_t> new_data;
  for (int i = 0; i < nbytes; ++i) {
    new_data.push_back(static_cast<uint8_t>((reg_val >> (8 * i)) & 0xFF));
  }

  printf("To update register 0x%04x: bits [%d:%d] set to 0x%x\n",
         reg_addr, shift + static_cast<int>(log2(mask)), shift, value);
  
  return new_data;  
}
  
bool ECON::isRunMode() {

  std::vector<uint8_t> data_03e4 = getValues(0x3e4, 1);
  uint8_t reg_val_erxenable = getParam(data_03e4, 0, 1);
  std::cout << "Register 0x3e4 erx0 enable: " << int(reg_val_erxenable) << std::endl;

  std::vector<uint8_t> data_03e5 = getValues(0x3e5, 1);
  std::cout << "Register 0x3e5 erx1 enable: " << int(getParam(data_03e5, 0, 1)) << std::endl;
  
  std::vector<uint8_t> data_03AB = getValues(0x03AB, 1);
  std::cout << "FCtrl_Global_command_rx_inverted value value " << getParam(data_03AB, 0, 1) << " locked " << getParam(data_03AB, 1, 1) << std::endl;
  
  // Read 3-byte register at 0x03C5 and extract run bit
  std::vector<uint8_t> data_03C5 = getValues(0x03C5, 3);  
  const uint32_t MASK_RUNBIT  = 1;
  const int      SHIFT_RUNBIT = 23;
  uint32_t pusm_run_value = getParam(data_03C5, SHIFT_RUNBIT, MASK_RUNBIT);
  std::cout << "PUSM run value: " << pusm_run_value << std::endl;

  std::vector<uint8_t> new_data_03C5 = newParam(data_03C5, 0x03C5, 3, MASK_RUNBIT, SHIFT_RUNBIT, 1);  // set RUNBIT = 1
  //setValues(0x03C5, new_data_03C5);

  pusm_run_value = getParam(data_03C5, SHIFT_RUNBIT, MASK_RUNBIT);
  std::cout << "new PUSM run value: " << pusm_run_value << std::endl;
  
  // Read 4-byte register at 0x03DF and extract PUSM state
  std::vector<uint8_t> data_03DF = getValues(0x3df, 4);
  const uint32_t MASK_PUSMSTATE = 15;
  const int      SHIFT_PUSMSTATE = 0;
  uint32_t pusm_state_value = getParam(data_03DF, SHIFT_PUSMSTATE, MASK_PUSMSTATE);
  std::cout << "PUSM state value: " << pusm_state_value << std::endl;

  return pusm_run_value == 1 && pusm_state_value == 8;
}
  
std::vector<uint8_t> ECON::getValues(int reg_addr, int nbytes) {
  if(nbytes < 1) {
    pflib_log(error) << "Invalid nbytes = " << nbytes;
  }
  
  pflib_log(info) << "ECON::getValues(" << packing::hex(reg_addr) << ", " << nbytes << ") from " << packing::hex(econ_base_);

  std::vector<uint8_t> waddr;
  waddr.push_back(static_cast<uint8_t>((reg_addr >> 8) & 0xFF));
  waddr.push_back(static_cast<uint8_t>(reg_addr & 0xFF));       

  std::vector<uint8_t> data = i2c_.general_write_read(econ_base_, waddr, nbytes);
  for (size_t i = 0; i < data.size(); i++) {
    printf("%02zu : %02x\n", i, data[i]);
  }

  return data;
}

void ECON::setValues(int reg_addr, const std::vector<uint8_t>& values) {
  if (values.empty()) {
    pflib_log(error) << "ECON::setValues called with empty data vector";
    return;
  }

  pflib_log(info) << "ECON::setValues(" 
                  << packing::hex(reg_addr) 
                  << ", nbytes = " << values.size() 
                  << ") to " << packing::hex(econ_base_);

  // write buffer
  std::vector<uint8_t> wbuf;
  wbuf.push_back(static_cast<uint8_t>((reg_addr >> 8) & 0xFF));
  wbuf.push_back(static_cast<uint8_t>(reg_addr & 0xFF));       
  wbuf.insert(wbuf.end(), values.begin(), values.end());

  // Perform write
  i2c_.general_write_read(econ_base_, wbuf, values.size());

  // Log written data
  printf("Wrote %zu bytes to register 0x%04x:\n", values.size(), reg_addr);
  for (size_t i = 0; i < values.size(); ++i) {
    printf("  %02zu : %02x\n", i, values[i]);
  }
}
  
}  // namespace pflib
