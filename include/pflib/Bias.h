#ifndef PFLIB_MAX5825_H_
#define PFLIB_MAX5825_H_

#include <vector>

#include "pflib/I2C.h"

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
  static const uint8_t REFn;
  static const uint8_t POWERn;

 public:
  /**
   * Wrap an I2C class for communicating with the MAX5825.
   * The bus we are on is the same as the ROC's bus.
   */
  MAX5825(std::shared_ptr<I2C>& i2c, uint8_t max_addr);

  /**
   * Get the settings for the DACs on this MAX
   *
   * We return the bytes requested i.e. further parsing
   * will be necessary especially in the per-DAC case where
   * the settings are the twelve MSBs from two concatenated
   * returned bytes.
   */
  std::vector<uint8_t> get(uint8_t channel);
  
  void set(uint8_t channel, uint16_t data_bytes);

  /**
   * Write a setting for the DACs on this MAX
   *
   * The input two data bytes is a single 16-bit word
   * where the 8 MSBs are the first data byte and the 8 LSBs
   * are the second.
   * Some pre-parsing may be necessary e.g. 12-bit settings for
   * a specific DAC would need to be passed as
   *   data_bytes = actual_12_bit_setting << 4;
   * Since the chip expects the 12-bit settings to be the 12 MSBs
   * from these two concatenated data words.
   */
  //void set(uint8_t cmd, uint16_t data_bytes);

  /**
   * get by DAC index
   *
   * The per-DAC commands return the 12-bit settings as the 12 MSBs
   * of two adjacent concatenated bits. We do this concatenation
   * in this file as well as determine how many bytes to request
   * depending on if we are requesting a single DAC (i_dac <= 7)
   * or if we are requesting all DACs (i_dac > 7).
   *
   * The per-DAC commands use the 4 MSBs as a command the 4 LSBs
   * to make the DAC selection. Using the commands defined as
   * static constants in this class, the combination of i_dac and
   * command is simply adding them together, which is done here.
   */
  //std::vector<uint16_t> getByDAC(uint8_t i_dac, uint8_t cmd);

  /**
   * set by DAC index
   *
   * The per-DAC commands expect the 12-bit settings as the 12 MSBs
   * of the two adjacent concatenated data bytes. We do the shifting
   * inside of this function, so the twelve_bit_setting should be the
   * value you want the DAC to be set at.
   *
   * The per-DAC commands use the 4 MSBs as a command the 4 LSBs
   * to make the DAC selection. Using the commands defined as
   * static constants in this class, the combination of i_dac and
   * command is simply adding them together, which is done here.
   */
  //void setByDAC(uint8_t i_dac, uint8_t cmd, uint16_t twelve_bit_setting);

  /** Set reference voltage
   * 0 - external
   * 1 - 2.5V
   * 2 - 2.048V
   * 3 - 4.096V
   */
  //void setRefVoltage(int level);

 private:
  /// our connection
  std::shared_ptr<I2C>& i2c_;
  /// our addr on the chip
  uint8_t our_addr_;
  /// our bus
  //int bus_;
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
   *
   * The bus is 4 + <board-number>, so we set the default to 4 for
   * the case where we only have one board with bus number 0.
   */
  Bias(std::shared_ptr<I2C>& i2c);

  /**
   * Initialize to standard settings
   *  Reference voltage - 4.096V
   */
  //void initialize();

  /**
   * Pass a setting to one LED DAC
   */
  //void cmdLED(uint8_t i_led, uint8_t cmd, uint16_t twelve_bit_setting);

  /**
   * Pass a setting to one SiPM DAC
   */
  //void cmdSiPM(uint8_t i_sipm, uint8_t cmd, uint16_t twelve_bit_setting);

  int readSiPM(uint8_t i_sipm);
  int readLED(uint8_t i_led);
  void setSiPM(uint8_t i_sipm, uint16_t code);
  void setLED(uint8_t i_led, uint16_t code);

  /**
   * Set and load the passed CODE for an LED bias
   *
   * Uses MAX5825::CODEn_LOADn
   *
   * This is a common procedure and operates as an example
   * of how to use setLED.
   */
  //void setLED(uint8_t i_led, uint16_t code);

  /**
   * Set and load the passed CODE for an SiPM bias
   *
   * Uses MAX5825::CODEn_LOADn
   *
   * This is a common procedure and operates as an example
   * of how to use setSiPM.
   */
  //void setSiPM(uint8_t i_sipm, uint16_t code);

 private:
  /// LED bias chips
  std::vector<MAX5825> led_;
  /// SiPM bias chips
  std::vector<MAX5825> sipm_;
};  // Bias

}  // namespace pflib

#endif  // PFLIB_MAX5825_H_
