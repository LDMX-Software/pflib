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

MAX5825::MAX5825(std::shared_ptr<I2C> i2c, uint8_t addr)
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

/// DAC chip addresses taken from HGCROC specs
const uint8_t Bias::ADDR_LED_0 = 0x18;
const uint8_t Bias::ADDR_LED_1 = 0x1A;
const uint8_t Bias::ADDR_SIPM_0 = 0x10;
const uint8_t Bias::ADDR_SIPM_1 = 0x12;

Bias::Bias(std::shared_ptr<I2C> i2c) {
  i2c_ = i2c;

  led_.emplace_back(i2c, Bias::ADDR_LED_0);
  led_.emplace_back(i2c, Bias::ADDR_LED_1);
  sipm_.emplace_back(i2c, Bias::ADDR_SIPM_0);
  sipm_.emplace_back(i2c, Bias::ADDR_SIPM_1);
}

void Bias::initialize() {
  // Reset all DAC:s
  i2c_->general_write_read(0x10, {0x35, 0x96, 0x30}, 0);
  i2c_->general_write_read(0x12, {0x35, 0x96, 0x30}, 0);
  i2c_->general_write_read(0x14, {0x35, 0x96, 0x30}, 0);
  i2c_->general_write_read(0x18, {0x35, 0x96, 0x30}, 0);

  // Set internal ref on DAC to 4.096 V
  i2c_->general_write_read(0x10, {0x27, 0x00, 0x00}, 0);
  i2c_->general_write_read(0x12, {0x27, 0x00, 0x00}, 0);
  i2c_->general_write_read(0x14, {0x27, 0x00, 0x00}, 0);
  i2c_->general_write_read(0x18, {0x27, 0x00, 0x00}, 0);

  // Set up the GPIO device MCP23008
  i2c_->general_write_read(0x20, {0x00, 0x70}, 0);

  // Turn on the status LED
  i2c_->general_write_read(0x20, {0x09, 0x80}, 0);

  // Set up the board temperature sensor TMP101
  i2c_->general_write_read(0x4A, {0x01, 0x60}, 0);

  // Set up the two onewire-to-I2C devices DS2482
  // Reset
  i2c_->general_write_read(0x1C, {0xF0}, 0);
  // Standard speed, weak pull-up, no active pull-up
  i2c_->general_write_read(0x1C, {0xC3, 0xF0}, 0);
  i2c_->general_write_read(0x1D, {0xF0}, 0);
  i2c_->general_write_read(0x1D, {0xC3, 0xF0}, 0);
}

double Bias::readTemp() {
  i2c_->general_write_read(0x4A, {0x00}, 0);
  usleep(250);  // Response is a bit slow
  std::vector<uint8_t> ret = i2c_->general_write_read(0x4A, {}, 2);

  int dec = 625 * ((ret.at(0) * 256 + ret.at(1)) >> 4);
  int integer = dec / 10000;
  int decimal = dec - integer * 10000;
  char cs[10];
  snprintf(cs, 10, "%d.%d", integer, decimal);
  std::string str = cs;
  return std::stod(str);
  ;
}

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
