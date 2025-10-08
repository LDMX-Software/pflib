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
      compiler_{Compiler::get(type_version)}
{
  pflib_log(debug) << "ECON base addr " << packing::hex(econ_base_);
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
	 reg_addr, shift + static_cast<int>(log2(mask)), shift, reg_val);
  
  return new_data;  
}

void ECON::setRunMode(bool active) {
  int nbytes = 3;
  const uint32_t MASK_RUNBIT  = 1;
  const int      SHIFT_RUNBIT = 23;
  std::vector<uint8_t> data_03C5 = getValues(0x03C5, 3);
  std::vector<uint8_t> new_data_03C5 = newParam(data_03C5, 0x03C5, nbytes, MASK_RUNBIT, SHIFT_RUNBIT, 1);
  setValues(0x03C5, new_data_03C5);

  std::vector<uint8_t> data_03DF = getValues(0x3df, 4);
  const uint32_t MASK_PUSMSTATE = 15;
  const int      SHIFT_PUSMSTATE = 0;
  uint32_t pusm_state_value = getParam(data_03DF, SHIFT_PUSMSTATE, MASK_PUSMSTATE);
  std::cout << "new PUSM state value: " << pusm_state_value << std::endl;

  std::vector<uint8_t> data_03AB = getValues(0x03AB, 1);
  std::cout << "FCtrl_Global_command_rx_inverted " << getParam(data_03AB, 0, 1) << " locked " << getParam(data_03AB, 1, 1) << std::endl;
}
  
bool ECON::isRunMode() {
  // Read 3-byte register at 0x03C5 and extract run bit
  std::vector<uint8_t> data_03C5 = getValues(0x03C5, 3);  
  const uint32_t MASK_RUNBIT  = 1;
  const int      SHIFT_RUNBIT = 23;
  uint32_t pusm_run_value = getParam(data_03C5, SHIFT_RUNBIT, MASK_RUNBIT);
  std::cout << "PUSM run value: " << pusm_run_value << std::endl;

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
    return {};
  }
  
  //pflib_log(info) << "ECON::getValues(" << packing::hex(reg_addr) << ", " << nbytes << ") from " << packing::hex(econ_base_);

  std::vector<uint8_t> waddr;
  waddr.push_back(static_cast<uint8_t>((reg_addr >> 8) & 0xFF));
  waddr.push_back(static_cast<uint8_t>(reg_addr & 0xFF));       
  std::vector<uint8_t> data = i2c_.general_write_read(econ_base_, waddr, nbytes);
  
  //for (size_t i = 0; i < data.size(); i++) {
  //  printf("%02zu : %02x\n", i, data[i]);
  //}

  return data;
}

void ECON::setValue(int reg_addr, uint64_t value, int nbytes) {
  if (nbytes <= 0 || nbytes > 8) {
    pflib_log(error) << "ECON::setValue called with invalid nbytes = " << nbytes;
    return;
  }

  // split the value into bytes (little endian)
  std::vector<uint8_t> data;
  for (int i = 0; i < nbytes; ++i) {
    data.push_back(static_cast<uint8_t>((value >> (8 * i)) & 0xFF));
  }

  setValues(reg_addr, data);
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

  // perform write
  i2c_.general_write_read(econ_base_, wbuf, values.size());

  // printf("Wrote %zu bytes to register 0x%04x:\n", values.size(), reg_addr);
  // for (size_t i = 0; i < values.size(); ++i) {
  //   printf("  %02zu : %02x\n", i, values[i]);
  // }
}

void ECON::setRegisters(const std::map<int, std::map<int, uint8_t>>& registers) {
  for (auto& page : registers) {
    int page_id = page.first;
    for (auto& reg : page.second) {
      printf("Page %d: setValue(0x%04x, 0x%02x)\n", page_id, reg.first, reg.second);
      //this->setValue(reg.first, reg.second);
    }
  }
}

void ECON::loadParameters(const std::string& file_path, bool prepend_defaults) {
  if (prepend_defaults) {
    /**
     * If we prepend defaults, then ALL of the parameters will be
     * touched and so we do need to bother reading the current
     * values and overlaying the new ones, instead we jump straight
     * to setting the registers.
     */
    auto settings = compiler_.compile(file_path, true);
    setRegisters(settings);
  }
}
  
}  // namespace pflib
