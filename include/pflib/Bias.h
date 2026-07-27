#ifndef PFLIB_MAX5825_H_
#define PFLIB_MAX5825_H_

#include <unistd.h>

#include <vector>
#include <optional>

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
  MAX5825(std::shared_ptr<I2C> i2c, uint8_t max_addr);

  /**
   * send the SW_RESET command to the MAX5825
   * 
   * "All CODE, DAC, and Control Register values are returned to 
   * their power-on reset values"
   */
  void reset();

  /**
   * Get the settings for the DACs on this MAX
   *
   * We return the bytes requested i.e. further parsing
   * will be necessary especially in the per-DAC case where
   * the settings are the twelve MSBs from two concatenated
   * returned bytes.
   */
  std::vector<uint8_t> get(uint8_t channel);

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
  void set(uint8_t channel, uint16_t data_bytes);

  /**
   * Set reference voltage
   *
   * @param[in] level (0 - external, 1 - 2.5V, 2 - 2.048V, 3 - 4.096V)
   */
  void setRefVoltage(int level);

 private:
  /// our connection
  std::shared_ptr<I2C> i2c_;
  /// our addr on the chip
  uint8_t our_addr_;
};  // MAX5825

/**
 * The HGC ROC has 4 MAX5825 chips doing the DAC for the bias voltages.
 * Two of the chips handle the 16 LED bias voltages and the other
 * two handle the 16 SiPM bias voltages.
 *
 * Both the LED and SiPM bias voltages are indexed from 0 to 15.
 * The conversion from that index to a chip and DAC index on that chip
 * is done in this class.
 *
 * The readback of the DAC settings in the MAX5825 seem to require
 * a "repeated start" I2C transaction where the register to be read
 * and the read command are sent before the next I2C stop.
 * Originally pointed out by Lennart at Lund, this "repeated start"
 * transaction *cannot* be done by the lpGBT, so *actual* readback
 * of the DAC settings is not supported in fiberfull setups.
 */
class Bias {
 public:
  static const int N_CHANNELS = 16;
  static const uint8_t ADDR_LED_0;
  static const uint8_t ADDR_LED_1;
  static const uint8_t ADDR_SIPM_0;
  static const uint8_t ADDR_SIPM_1;

 public:
  /**
   * Wrap I2C objects for communicating with all the DAC chips.
   *
   * The bus is 4 + <board-number>, so we set the default to 4 for
   * the case where we only have one board with bus number 0.
   *
   * @param[in] bias_i2c I2C object for interacting with MAX5825 bias DACs
   * @param[in] board_i2c I2C object for talking with HGCROC board periferials
   * @param[in] use_cache whether to use the software cache for readback (true)
   * or not (false). This is necessary for readback to function on fiberless
   * setups.
   */
  Bias(std::shared_ptr<I2C> bias_i2c, std::shared_ptr<I2C> board_i2c, bool use_cache);

  /// dont copy construct this class
  Bias(const Bias&) = delete;
  /// dont copy assign this class
  Bias& operator=(const Bias&) = delete;

  /**
   * Initialize to standard settings
   */
  void initialize();
  
  /**
   * Read the temperature from the temp sensor on the HGCROC board
   */
  double readTemp();

  std::optional<int> readSiPM(uint8_t i_sipm);
  std::optional<int> readLED(uint8_t i_led);
  void setSiPM(uint8_t i_sipm, uint16_t code);
  void setLED(uint8_t i_led, uint16_t code);
 private:
  /// I2C comms with bias DACs MAX5825
  std::shared_ptr<I2C> i2c_bias_;
  /// I2C comms with HGCROC board periferials
  std::shared_ptr<I2C> i2c_board_;
  /// LED bias chips
  std::vector<MAX5825> led_;
  /// SiPM bias chips
  std::vector<MAX5825> sipm_;
  /// whether to use the software cache of the settings
  bool use_cache_;
  /// cache of sipm settings
  std::array<std::optional<int>, N_CHANNELS> sipm_cache_;
  /// cache of led settings
  std::array<std::optional<int>, N_CHANNELS> led_cache_;
};  // Bias

}  // namespace pflib

#endif  // PFLIB_MAX5825_H_
