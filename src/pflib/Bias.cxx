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

  std::vector<uint8_t> retval = i2c_->general_write_read(our_addr_, {cmd}, 2);

  return retval;
}

void MAX5825::set(uint8_t channel, uint16_t code) {
  uint8_t cmd = (uint8_t)(0xB0 | (channel & 0x07));

  std::vector<uint8_t> retval =
      i2c_->general_write_read(our_addr_,
                               {cmd, static_cast<uint8_t>((code << 4) >> 8),
                                static_cast<uint8_t>((code << 4) & 0xFF)},
                               0);
}

/*
void MAX5825::setRefVoltage(int level) {
  uint8_t cmd = REFn | 0x4 | (level & 0x3);
  set(cmd, 0);
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

int Bias::readSiPM(uint8_t channel) {
  int i_chip = (channel > 7);
  std::vector<uint8_t> data = sipm_.at(i_chip).get((channel & 0x07));
  return ((data.at(0) * 256 + data.at(1)) >> 4);
}

int Bias::readLED(uint8_t channel) {
  int i_chip = (channel > 7);
  std::vector<uint8_t> data = led_.at(i_chip).get((channel & 0x07));
  return ((data.at(0) * 256 + data.at(1)) >> 4);
}

void Bias::setSiPM(uint8_t channel, uint16_t code) {
  int i_chip = (channel > 7);
  sipm_.at(i_chip).set((channel & 0x07), code);
}

void Bias::setLED(uint8_t channel, uint16_t code) {
  int i_chip = (channel > 7);
  led_.at(i_chip).set((channel & 0x07), code);
}

}  // namespace pflib
