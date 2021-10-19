#include "pflib/Bias.h"

#include <iostream>
#include <bitset>

namespace pflib {

MAX5825::MAX5825(I2C& i2c, uint8_t addr, int bus) : i2c_{i2c}, our_addr_{addr}, bus_{bus} {}

std::vector<uint16_t> MAX5825::getCODE(unsigned int i_dac) {
  i2c_.set_active_bus(bus_);
  i2c_.set_bus_speed(1300);

  // change i DAC to all DACs if
  // it goes over the number of DACs on this chip
  int num_dacs = 1;
  uint8_t request = (11 << 4) + i_dac;
  if (i_dac > 7) {
    request = (11 << 4) + 8;
    num_dacs = 8;
  }

  int num_bytes_to_read = 2*num_dacs;
  std::cout << "Reading " << num_dacs << " DACs " << i_dac 
    << " for " << num_bytes_to_read << " bytes" <<  std::endl;

  std::vector<unsigned char> instructions = { request };

  std::cout << "Instructions: ";
  for (auto const& i : instructions) std::cout << std::bitset<8>(i) << " ";
  std::cout << std::endl;
  
  auto read_bytes{i2c_.general_write_read(our_addr_, instructions, num_bytes_to_read)};
 
  std::cout << "Got " << read_bytes.size() << " bytes..." << std::endl;
  
  std::vector<uint16_t> codes(num_dacs);
  for (unsigned int j{0}; j < num_dacs; j++) {
    codes[j] = (read_bytes.at(2*j)<<4)+(read_bytes.at(2*j+1)>>4);
    std::cout << std::bitset<8>(read_bytes.at(2*j)) 
      << " " << std::bitset<8>(read_bytes.at(2*j+1))
      << std::endl; 
  }

  return codes;
}

void MAX5825::setCODE(unsigned int i_dac, uint16_t code) {
  i2c_.set_active_bus(bus_);
  i2c_.set_bus_speed(1300);

  // change i DAC to all DACs if
  // it goes over the number of DACs on this chip
  int num_dacs = 1;
  if (i_dac > 7) {
    i_dac = 8;
    num_dacs = 8;
  }

  std::cout << "Writing " << num_dacs << " DACs " << i_dac 
    << " with CODE " << code << std::endl;

  /**
   * CODE is a 12-bit integer where the 8 MSBs are in the first data byte
   * and the 4 LSBs are the 4 MSBs of the second data byte
   */
  code &= 0xFFF; // only 12-bit integers allowed
  code <<= 4; // left-shift four so code can now be split into two bytes
  std::vector<unsigned char> instructions = { 
    //addr(MAX5825::ADDR::NC, MAX5825::ADDR::GND, false), //writing
    (11 << 4) + i_dac, // CODE for this dac
    code >> 8, // first data byte
    code & 0xFF, // second data byte
  };

  std::cout << "Instructions: ";
  for (auto const& i : instructions) std::cout << std::bitset<8>(i) << " ";
  std::cout << std::endl;

  i2c_.general_write_read(our_addr_, instructions);

  return;
}

Bias::Bias(I2C& i2c, int bus) {
  led_.emplace_back(i2c, Bias::ADDR_LED_0, bus);
  led_.emplace_back(i2c, Bias::ADDR_LED_1 , bus);
  sipm_.emplace_back(i2c, Bias::ADDR_SIPM_0, bus);
  sipm_.emplace_back(i2c, Bias::ADDR_SIPM_1 , bus);
}

void Bias::setLED_CODE(uint8_t i_led, uint16_t code) {
  int i_chip = (i_led > 7);
  led_.at(i_chip).setCODE(i_led - i_chip*8, code);
}

}
