#ifndef PFLIB_MAX5825_H_
#define PFLIB_MAX5825_H_

#include "pflib/I2C.h"
#include <vector>

namespace pflib {

/**
 * Class representing communication with 
 * the Digital-Analog Converter on the HGC ROC.
 * MAX5825
 *
 * https://datasheets.maximintegrated.com/en/ds/MAX5823-MAX5825.pdf
 *
 * CODE - future setting for the DAC output
 * LOAD - load CODE into DAC outputs
 * CODE_LOAD - CODE and then LOAD
 *
 * The lower-level set and get functions are for chip-wide
 * configuration (e.g. the watch dog WDOG configuration).
 *
 * Use the {set,get}byDAC functions when setting or getting 
 * DAC parameters.
 */
class MAX5825 {
 public:
  // Board Commands
  static const uint8_t WDOG;

  // DAC Commands
  //  add the DAC selection to these commands
  //  to get the full command byte
  static const uint8_t RETURNn;
  static const uint8_t CODEn;
  static const uint8_t LOADn;
  static const uint8_t CODEn_LOADALL;
  static const uint8_t CODEn_LOADn;
 public:
  /**
   * Wrap an I2C class for communicating with the MAX5825.
   * The bus we are on is the same as the ROC's bus.
   */
  MAX5825(I2C& i2c, uint8_t max_addr, int bus = 4);

  /**
   * Get the settings for the DACs on this MAX
   */
  std::vector<uint8_t> get(uint8_t cmd, int n_return_bytes = 2);

  /**
   * Write a setting for the DACs on this MAX
   */
  void set(uint8_t cmd, uint16_t data_bytes);

  /**
   * get by DAC index
   */
  std::vector<uint16_t> getByDAC(uint8_t i_dac, uint8_t cmd);

  /**
   * set by DAC index
   */
  void setByDAC(uint8_t i_dac, uint8_t cmd, uint16_t data_bytes);

  /**
   * CODE a specific DAC and load that code into action.
   */
  void codeDAC(uint8_t dac, uint16_t twelve_bit_dac_code) {
    setByDAC(dac, CODEn_LOADn, twelve_bit_dac_code);
  }

 private:
  /// our connection
  I2C& i2c_;
  /// our addr on the chip
  uint8_t our_addr_;
  /// our bus
  int bus_;
};  // MAX5825

/**
 * The HGC ROC has 4 MAX5825 chips doing the DAC for the bias voltages.
 * Two of the chips handle the 16 LED bias voltages and the other
 * two handle the 16 SiPM bias voltages.
 *
 * Both the LED and SiPM bias voltages are indexed from 0 to 15.
 * To convert from that index to a chip and DAC index on that chip,
 * just use integer division and modulus.
 *
 *  chip_index = int(index > 7)
 *  dac_index  = index - 8*chip_index;
 */
class Bias {
 public:
  /// DAC chip addresses
  static const uint8_t ADDR_LED_0;
  static const uint8_t ADDR_LED_1;
  static const uint8_t ADDR_SIPM_0;
  static const uint8_t ADDR_SIPM_1;
 public:
  /**
   * Wrap an I2C class for communicating with all the DAC chips.
   */
  Bias(I2C& i2c, int bus = 4);

  /**
   * Set and load the passed CODE for an LED bias
   */
  void codeLED(uint8_t i_led, uint16_t code);

  /**
   * Set and load the passed CODE for an SiPM bias
   */
  void codeSiPM(uint8_t i_sipm, uint16_t code);

 private:
  /// LED bias chips
  std::vector<MAX5825> led_;
  /// SiPM bias chips
  std::vector<MAX5825> sipm_;
};  // Bias

}

#endif // PFLIB_MAX5825_H_
