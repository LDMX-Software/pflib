#include "pflib/ROC.h"
#include <iostream>

#include "register_maps/direct_access.h"

namespace pflib {

static const int block_for_chan[] = {261, 260, 259, 258, // 0-3
  265, 264, 263, 262, // 4-7
  269, 268, 267, 266, // 8-11
  273, 272, 271, 270, // 12-15
  294, 256, 277, 295, // 16-19
  278, 279, 280, 281, // 20-23
  282, 283, 284, 285, // 24-27
  286, 287, 288, 289, // 28-31
  290, 291, 292, 293, // 32-35
  5, 4, 3, 2, // 36-39
  9, 8, 7, 6, // 40-43
  13, 12, 11, 10, // 44-47
  17, 16, 15, 14, // 48-51
  38, 0, 21, 39, // 51-55
  22, 23, 24, 25, // 56-59
  26, 27, 28, 29, // 60-63
  30, 31, 32, 33, // 64-67
  34, 35, 36, 37}; // 68-71

ROC::ROC(I2C& i2c, uint8_t roc_base_addr, const std::string& type_version)
  : i2c_{i2c}, roc_base_{roc_base_addr}, compiler_{Compiler::get(type_version)} {}

std::vector<uint8_t> ROC::readPage(int ipage, int len) {
  i2c_.set_bus_speed(1400);

  std::vector<uint8_t> retval;
  for (int i=0; i<len; i++) {
    // set the address
    uint16_t fulladdr=(ipage<<5)|i;
    i2c_.write_byte(roc_base_+0,fulladdr&0xFF);
    i2c_.write_byte(roc_base_+1,(fulladdr>>8)&0xFF);
    // now read
    retval.push_back(i2c_.read_byte(roc_base_+2));
  }
  return retval;
}

uint8_t ROC::getValue(int ipage, int offset) {
  i2c_.set_bus_speed(1400);

  // set the address
  uint16_t fulladdr=(ipage<<5)|offset;
  i2c_.write_byte(roc_base_+0,fulladdr&0xFF);
  i2c_.write_byte(roc_base_+1,(fulladdr>>8)&0xFF);
    // now read
  return i2c_.read_byte(roc_base_+2);
}

bool ROC::getDirectAccess(const std::string& name) {
  auto spec_it = pflib::DIRECT_ACCESS_PARAMETER_LUT.find(name);
  if (spec_it == pflib::DIRECT_ACCESS_PARAMETER_LUT.end()) {
    PFEXCEPTION_RAISE("BadName", "Direct Access parameter named '"+name+"' not found in mapping.");
  }
  return getDirectAccess(spec_it->second.reg, spec_it->second.bit_location);
}

bool ROC::getDirectAccess(int reg, int bit) {
  if (reg < 4 or reg > 7) {
    PFEXCEPTION_RAISE("BadReg", "Direct access registers are the values 4, 5, 6, and 7.");
  }
  if (bit < 0 or bit > 7) {
    PFEXCEPTION_RAISE("BadBit", "Direct access bit locations are the indices from 0 to 7.");
  }
  uint8_t reg_val = i2c_.read_byte(roc_base_+reg);
  return ((reg_val >> bit)&0b1) == 1;
}

void ROC::setDirectAccess(const std::string& name, bool val) {
  auto spec_it = pflib::DIRECT_ACCESS_PARAMETER_LUT.find(name);
  if (spec_it == pflib::DIRECT_ACCESS_PARAMETER_LUT.end()) {
    PFEXCEPTION_RAISE("BadName", "Direct Access parameter named '"+name+"' not found in mapping.");
  }
  setDirectAccess(spec_it->second.reg, spec_it->second.bit_location, val);
}

void ROC::setDirectAccess(int reg, int bit, bool val) {
  if (reg < 4 or reg > 7) {
    PFEXCEPTION_RAISE("BadReg", "Direct access registers are the values 4, 5, 6, and 7.");
  }
  if (reg == 7) {
    PFEXCEPTION_RAISE("BadReg", "Direct access register 7 is read only.");
  }
  if (bit < 0 or bit > 7) {
    PFEXCEPTION_RAISE("BadBit", "Direct access bit locations are the indices from 0 to 7.");
  }
  // get the register
  uint8_t reg_val = i2c_.read_byte(roc_base_+reg);
  // clear the value in that bit position
  reg_val &= ~(static_cast<uint8_t>(1) << bit);
  // set the bit to the same as our value
  reg_val |= (static_cast<uint8_t>(val ? 1 : 0) << bit);
  // write the register back
  i2c_.write_byte(roc_base_+reg, reg_val);
}

static const int TOP_PAGE       =  45;
static const int MASK_RUN_MODE  = 0x3;

void ROC::setRunMode(bool active) {
  uint8_t cval=getValue(TOP_PAGE,0);
  uint8_t imask=0xFF^MASK_RUN_MODE;
  cval&=imask;
  if (active) cval|=MASK_RUN_MODE;
  setValue(TOP_PAGE,0,cval);
}

bool ROC::isRunMode() {
  uint8_t cval=getValue(TOP_PAGE,0);
  return cval&MASK_RUN_MODE;
}
  
std::vector<uint8_t> ROC::getChannelParameters(int ichan) {
  return readPage(block_for_chan[ichan],14);
}

void ROC::setChannelParameters(int ichan, std::vector<uint8_t>& values) {
  std::cout << "I don't do anything right now" << std::endl;
}

void ROC::setValue(int page, int offset, uint8_t value) {
  i2c_.set_bus_speed(1400);
  uint16_t fulladdr=(page<<5)|offset;
  i2c_.write_byte(roc_base_+0,fulladdr&0xFF);
  i2c_.write_byte(roc_base_+1,(fulladdr>>8)&0xFF);
  i2c_.write_byte(roc_base_+2,value&0xFF);
}

void ROC::setRegisters(const std::map<int,std::map<int,uint8_t>>& registers) {
  for (auto& page : registers) {
    for (auto& reg : page.second) {
      this->setValue(page.first,reg.first,reg.second);
    }
  }
}

std::vector<std::string> ROC::parameters(const std::string& page) {
  return compiler_.parameters(page);
}

void ROC::applyParameters(const std::map<std::string,std::map<std::string,int>>& parameters) {
  // 1. get registers YAML file contains by compiling without defaults
  auto touched_registers = compiler_.compile(parameters);
  // 2. get the current register values on the chip
  std::map<int,std::map<int,uint8_t>> chip_reg;
  for (auto& page : touched_registers) {
    int page_id = page.first;
    std::vector<uint8_t> on_chip_reg_values = this->readPage(page_id, 16);
    for (int i{0}; i < 16; i++) {
      // skip un-touched registers
      if (page.second.find(i) == page.second.end()) 
        continue;
      chip_reg[page_id][i] = on_chip_reg_values.at(i);
    }
  }
  // 3. compile this parameter onto those register values
  //    we can use the lower-level compile here because the
  //    compile in step 1 checks that all of the page and param
  //    names are correct
  for (auto& page : parameters) {
    std::string page_name = upper_cp(page.first);
    for (auto& param : page.second) {
      compiler_.compile(page_name,upper_cp(param.first),param.second,chip_reg);
    }
  }
  // 4. put these updated values onto the chip
  this->setRegisters(chip_reg);
}

void ROC::applyParameter(const std::string& page, const std::string& param, const int& val) {
  std::map<std::string,std::map<std::string,int>> p;
  p[page][param] = val;
  this->applyParameters(p);
}

}
