#include "pflib/Bias.h"

#include <bitset>
#include <iostream>

namespace pflib {

// Board Commands
const uint8_t MAX5825::WDOG = 1 << 4;

// DAC Commands
//  add the DAC selection to these commands
//  to get the full command byte
const uint8_t MAX5825::RETURNn = 7 << 4;
const uint8_t MAX5825::CODEn = 8 << 4;
const uint8_t MAX5825::LOADn = 9 << 4;
const uint8_t MAX5825::CODEn_LOADALL = 10 << 4;
const uint8_t MAX5825::CODEn_LOADn = 11 << 4;
const uint8_t MAX5825::REFn = 2 << 4;
const uint8_t MAX5825::POWERn = 4 << 4;


MAX5825::MAX5825(std::shared_ptr<I2C>& i2c, uint8_t addr)
    : i2c_{i2c}, our_addr_{addr} {}



std::vector<uint8_t> MAX5825::get(uint8_t channel) {
  uint8_t cmd = (uint8_t)(MAX5825::CODEn | (channel & 0x07));
  
  std::vector<uint8_t> retval =
      i2c_->general_write_read_ioctl(our_addr_, {cmd}, 2);

  return retval;
}

void MAX5825::set(uint8_t channel, uint16_t code){
  uint8_t cmd = (uint8_t)(0xB0 | (channel & 0x07));
  
  std::vector<uint8_t> retval =
      i2c_->general_write_read_ioctl(our_addr_, {cmd, (code << 4) >> 8, (code << 4) & 0xFF}, 0);
}

/*
void MAX5825::set(uint8_t cmd, uint16_t data_bytes) {
  int savebus = i2c_.get_active_bus();
  int savespeed = i2c_.get_bus_speed();
  i2c_.set_active_bus(bus_);
  i2c_.set_bus_speed(100);
  i2c_.backplane_hack(true);

  std::vector<unsigned char> instructions = {
      cmd,                         // CODE for this dac
      uint8_t(data_bytes >> 8),    // first data byte
      uint8_t(data_bytes & 0xFF),  // second data byte
  };

  i2c_.general_write_read(our_addr_, instructions);

  i2c_.set_active_bus(savebus);
  i2c_.set_bus_speed(savespeed);
  i2c_.backplane_hack(false);
  return;
}
*/

/*
void MAX5825::setRefVoltage(int level) {
  uint8_t cmd = REFn | 0x4 | (level & 0x3);
  set(cmd, 0);
}
*/

/*
std::vector<uint16_t> MAX5825::getByDAC(uint8_t i_dac, uint8_t cmd) {
  uint8_t num_dacs = 1;
  if (i_dac > 7) {
    i_dac = 8;
    num_dacs = 8;
  }
  auto bytes{get(cmd + i_dac, 2 * num_dacs)};
  std::vector<uint16_t> vals(num_dacs);
  for (unsigned int j{0}; j < num_dacs; j++) {
    vals[j] = (bytes.at(2 * j) << 4) + (bytes.at(2 * j + 1) >> 4);
  }
  return vals;
}
*/

/*
void MAX5825::setByDAC(uint8_t i_dac, uint8_t cmd, uint16_t data_bytes) {
  if (i_dac > 7) i_dac = 8;
  // for the MAX5825, the voltages are 12-bits,
  // so the 4 LSBs of the two data bytes will be ignored.
  data_bytes <<= 4;
  set(cmd + i_dac, data_bytes);
}
*/

/// DAC chip addresses taken from HGCROC specs
const uint8_t Bias::ADDR_LED_0 = 0x18;
const uint8_t Bias::ADDR_LED_1 = 0x1A;
const uint8_t Bias::ADDR_SIPM_0 = 0x10;
const uint8_t Bias::ADDR_SIPM_1 = 0x12;


Bias::Bias(std::shared_ptr<I2C>& i2c) {
  led_.emplace_back(i2c, Bias::ADDR_LED_0);
  led_.emplace_back(i2c, Bias::ADDR_LED_1);
  sipm_.emplace_back(i2c, Bias::ADDR_SIPM_0);
  sipm_.emplace_back(i2c, Bias::ADDR_SIPM_1);
}


/*
void Bias::initialize() {
  led_[0].setRefVoltage(3);
  led_[1].setRefVoltage(3);
  sipm_[0].setRefVoltage(3);
  sipm_[1].setRefVoltage(3);

  led_[0].set(MAX5825::POWERn, 0xFF00);
}
*/

/*
void Bias::cmdLED(uint8_t i_led, uint8_t cmd, uint16_t twelve_bit_setting) {
  int i_chip = (i_led > 7);
  led_.at(i_chip).setByDAC(i_led - i_chip * 8, cmd, twelve_bit_setting);
}
*/

/*
void Bias::cmdSiPM(uint8_t i_sipm, uint8_t cmd, uint16_t twelve_bit_setting) {
  std::cout << "Bias::cmdSiPM i_sipm " << i_sipm << " cmd " << cmd << std::endl;
  int i_chip = (i_sipm > 7);
  std::cout << "i_chip " << i_chip << std::endl;
  sipm_.at(i_chip).setByDAC(i_sipm - i_chip * 8, cmd, twelve_bit_setting);
}
*/

int Bias::readSiPM(uint8_t channel){
  int i_chip = (channel > 7);
  std::vector<uint8_t> data = sipm_.at(i_chip).get((channel & 0x07));
  return ((data.at(0)*256 +data.at(1)) >> 4);
}

int Bias::readLED(uint8_t channel){
  int i_chip = (channel > 7);
  std::vector<uint8_t> data = led_.at(i_chip).get((channel & 0x07));
  return ((data.at(0)*256 +data.at(1)) >> 4);
}

void Bias::setSiPM(uint8_t channel, uint16_t code) {
  int i_chip = (channel > 7);
  sipm_.at(i_chip).set((channel & 0x07), code);
}

void Bias::setLED(uint8_t channel, uint16_t code) {
  int i_chip = (channel > 7);
  led_.at(i_chip).set((channel & 0x07), code);
}

//void Bias::setSiPM(uint8_t i_sipm, uint16_t code) {
//  cmdSiPM(i_sipm, MAX5825::CODEn_LOADn, code);
//}


}  // namespace pflib
