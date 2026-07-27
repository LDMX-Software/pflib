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

  i2c_->set_bus_speed(100);
  std::vector<uint8_t> retval = i2c_->general_write_read(our_addr_, {cmd}, 2);

  return retval;
}

void MAX5825::reset() {
  i2c_->set_bus_speed(100);
  i2c_->general_write_read(our_addr_, {0x35, 0x96, 0x30}, 0);
}

void MAX5825::set(uint8_t channel, uint16_t code) {
  uint8_t cmd = (uint8_t)(0xB0 | (channel & 0x07));

  i2c_->set_bus_speed(100);
  std::vector<uint8_t> retval =
      i2c_->general_write_read(our_addr_,
                               {cmd, static_cast<uint8_t>((code << 4) >> 8),
                                static_cast<uint8_t>((code << 4) & 0xFF)},
                               0);
}

void MAX5825::setRefVoltage(int level) {
  if (level < 0 or level > 3) {
    PFEXCEPTION_RAISE(
        "BadLevel",
        "The MAX5825 reference voltage setting needs to be 0, 1, 2, or 3. " +
            std::to_string(level) + " is out of this range.");
  }
  uint8_t cmd = 0;
  cmd |= 0x20;  // tell MAX5825 we are configuring the REF
  cmd |= 0x04;  // turn on DAC
  cmd |= static_cast<uint8_t>(level & 0x3);  // ref voltage
  i2c_->set_bus_speed(100);
  i2c_->general_write_read(our_addr_, {cmd, 0x00, 0x00}, 0);
}

/// DAC chip addresses taken from HGCROC specs
const uint8_t Bias::ADDR_LED_0 = 0x18;
const uint8_t Bias::ADDR_LED_1 = 0x1A;
const uint8_t Bias::ADDR_SIPM_0 = 0x10;
const uint8_t Bias::ADDR_SIPM_1 = 0x12;

Bias::Bias(std::shared_ptr<I2C> i2c_bias, std::shared_ptr<I2C> i2c_board,
           bool use_cache)
    : i2c_bias_{i2c_bias}, i2c_board_{i2c_board}, use_cache_{use_cache} {
  led_.emplace_back(i2c_bias, Bias::ADDR_LED_0);
  led_.emplace_back(i2c_bias, Bias::ADDR_LED_1);
  sipm_.emplace_back(i2c_bias, Bias::ADDR_SIPM_0);
  sipm_.emplace_back(i2c_bias, Bias::ADDR_SIPM_1);
}

void Bias::initialize() {
  /// reset all CODEs, DACs, and CRs to zero
  for (int i_chip{0}; i_chip < 2; i_chip++) {
    led_[i_chip].reset();
    sipm_[i_chip].reset();
  }
  sipm_cache_.fill(0);
  led_cache_.fill(0);

  /// Set internal ref on DAC to 4.096 V
  for (int i_chip{0}; i_chip < 2; i_chip++) {
    led_[i_chip].setRefVoltage(3);
    sipm_[i_chip].setRefVoltage(3);
  }

  /// Set up the GPIO device MCP23008
  i2c_board_->set_bus_speed(100);
  i2c_board_->general_write_read(0x20, {0x00, 0x70}, 0);

  /// Turn on the status LED
  i2c_board_->general_write_read(0x20, {0x09, 0x80}, 0);

  /// Set up the board temperature sensor TMP101
  i2c_board_->general_write_read(0x4A, {0x01, 0x60}, 0);

  /// Set up the two onewire-to-I2C devices DS2482
  // Reset
  i2c_board_->general_write_read(0x1C, {0xF0}, 0);
  // Standard speed, weak pull-up, no active pull-up
  i2c_board_->general_write_read(0x1C, {0xC3, 0xF0}, 0);
  i2c_board_->general_write_read(0x1D, {0xF0}, 0);
  i2c_board_->general_write_read(0x1D, {0xC3, 0xF0}, 0);
}

double Bias::readTemp() {
  i2c_board_->set_bus_speed(100);
  i2c_board_->general_write_read(0x4A, {0x00}, 0);
  usleep(250);  // Response is a bit slow
  std::vector<uint8_t> ret = i2c_board_->general_write_read(0x4A, {}, 2);

  int dec = 625 * ((ret.at(0) * 256 + ret.at(1)) >> 4);
  int integer = dec / 10000;
  int decimal = dec - integer * 10000;
  char cs[10];
  snprintf(cs, 10, "%d.%d", integer, decimal);
  std::string str = cs;
  return std::stod(str);
}

std::optional<int> Bias::readSiPM(uint8_t channel) {
  if (channel >= N_CHANNELS) {
    PFEXCEPTION_RAISE("BadChannel", "Channel number " +
                                        std::to_string(channel) +
                                        " is out of range for the bias chip");
  }
  if (use_cache_) {
    return sipm_cache_[channel];
  }
  int i_chip = (channel > 7);
  std::vector<uint8_t> data = sipm_.at(i_chip).get((channel & 0x07));
  return ((data.at(0) * 256 + data.at(1)) >> 4);
}

std::optional<int> Bias::readLED(uint8_t channel) {
  if (channel >= N_CHANNELS) {
    PFEXCEPTION_RAISE("BadChannel", "Channel number " +
                                        std::to_string(channel) +
                                        " is out of range for the bias chip");
  }
  if (use_cache_) {
    return led_cache_[channel];
  }
  int i_chip = (channel > 7);
  std::vector<uint8_t> data = led_.at(i_chip).get((channel & 0x07));
  return ((data.at(0) * 256 + data.at(1)) >> 4);
}

void Bias::setSiPM(uint8_t channel, uint16_t code) {
  if (channel >= N_CHANNELS) {
    PFEXCEPTION_RAISE("BadChannel", "Channel number " +
                                        std::to_string(channel) +
                                        " is out of range for the bias chip");
  }
  sipm_cache_[channel] = code;
  int i_chip = (channel > 7);
  sipm_.at(i_chip).set((channel & 0x07), code);
}

void Bias::setLED(uint8_t channel, uint16_t code) {
  if (channel >= N_CHANNELS) {
    PFEXCEPTION_RAISE("BadChannel", "Channel number " +
                                        std::to_string(channel) +
                                        " is out of range for the bias chip");
  }
  led_cache_[channel] = code;
  int i_chip = (channel > 7);
  led_.at(i_chip).set((channel & 0x07), code);
}

}  // namespace pflib
