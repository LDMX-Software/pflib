#include "pflib/ROC.h"
#include <iostream>

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

ROC::ROC(const I2C& i2c, int ibus) : i2c_{const_cast<I2C&>(i2c)},ibus_{ibus} {
}

std::vector<uint8_t> ROC::readPage(int ipage, int len) {
  i2c_.set_active_bus(ibus_);
  i2c_.set_bus_speed(1400);

  std::vector<uint8_t> retval;
  for (int i=0; i<len; i++) {
    // set the address
    uint16_t fulladdr=(ipage<<5)|i;
    i2c_.write_byte(0,fulladdr&0xFF);
    i2c_.write_byte(1,(fulladdr>>8)&0xFF);
    // now read
    retval.push_back(i2c_.read_byte(2));
  }
  return retval;
}

std::vector<uint8_t> ROC::getChannelParameters(int ichan) {
  return readPage(block_for_chan[ichan],14);
}

void ROC::setChannelParameters(int ichan, std::vector<uint8_t>& values) {
  std::cout << "I don't do anything right now" << std::endl;
}

void ROC::setValue(int page, int offset, uint32_t value) {
  i2c_.set_active_bus(ibus_);
  i2c_.set_bus_speed(1400);
  uint16_t fulladdr=(page<<5)|offset;
  i2c_.write_byte(0,fulladdr&0xFF);
  i2c_.write_byte(1,(fulladdr>>8)&0xFF);
  i2c_.write_byte(2,value&0xFF);
}

}
