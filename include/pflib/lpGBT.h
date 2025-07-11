#ifndef PFLIB_lpGBT_H_INCLUDED
#define PFLIB_lpGBT_H_INCLUDED

#include <stdint.h>

#include <vector>

#include "pflib/Exception.h"

namespace pflib {

/**
 * @class lpGBT configuration transport inferface class
 */
class lpGBT_ConfigTransport {
 public:
  virtual ~lpGBT_ConfigTransport() {}
  /** Read the contents of the identified single register
   */
  virtual uint8_t read_reg(uint16_t reg) = 0;
  /** Write the given value to the identified single register
   */
  virtual void write_reg(uint16_t reg, uint8_t value) = 0;
  /** Read the contents of several registers beginning with the listed
      one.  The default implementation uses read_reg repeatedly, but
      faster implementations are possible for I2C and IC/EC
      communication.
   */
  virtual std::vector<uint8_t> read_regs(uint16_t reg, int n);
  /** Write the given values to a sequence of registers beginning with
      the listed one. The default implementation uses write_reg repeatedly, but
      faster implementations are possible for I2C and IC/EC
      communication.
  */
  virtual void write_regs(uint16_t reg, const std::vector<uint8_t>& value);
};

/** Class which provides an interface with an lpGBT ASIC **as mounted
    on an LDMX mezzanine**.  Includes both low-level and medium-level
    interfaces.  Uses an instance of a lpGBT_ConfigTransport for
    communication.  


    The statement **as mounted on an LDMX mezzanine** indicates that
    some channel numbers are tuned/adjusted to match the numbers on
    the mezzanine interface.  For example, the pin labelled GPIO3 on
    the mezzanine is connected to pin GPIO15 on the lpGBT, which is
    hidden by this class such that this signal is considered to be
    GPIO3.
*/
class lpGBT {
 public:
  lpGBT(lpGBT_ConfigTransport& transport);

  void write(uint16_t reg, uint8_t value) { tport_.write_reg(reg, value); }
  uint8_t read(uint16_t reg) { return tport_.read_reg(reg); }

  typedef std::pair<uint16_t, uint8_t> RegisterValue;
  typedef std::vector<RegisterValue> RegisterValueVector;
  /** Configure from a vector of register values */
  void write(const RegisterValueVector& regvalues);

  /** Read values for set of registers */
  RegisterValueVector read(const std::vector<uint16_t>& registers);

  void bit_set(uint16_t reg, int ibit);
  void bit_clr(uint16_t reg, int ibit);

  /* -------------------------------------------------------
     Medium-level interfaces
  */

  /** Get the lpGBT status (power up state machine) */
  int status();
  std::string status_name(int pusm);  
  
  /** Set the given gpio bit */
  void gpio_set(int ibit, bool high);
  /** Get the given gpio bit */
  bool gpio_get(int ibit);
  /** Set the full GPIO array */
  void gpio_set(uint16_t values);
  /** Get the full GPIO array */
  uint16_t gpio_get();

  static const int GPIO_IS_OUTPUT = 0x01;
  static const int GPIO_IS_PULLUP = 0x02;
  static const int GPIO_IS_PULLDOWN = 0x04;
  static const int GPIO_IS_STRONG = 0x08;

  /** Get the GPIO pin configuration */
  int gpio_cfg_get(int ibit);
  /** Set the GPIO pin configuration */
  void gpio_cfg_set(int ibit, int cfg);

  /** Carry out an ADC read for the given pair of channels
      Valid gain values are 1 or 2, 8, 16, and 32.
      
   */
  uint16_t adc_read(int ipos, int ineg, int gain = 1);

  /** Carry out an ADC read for the given channel, using the internal reference voltage for the other side of the measurement, with the current DAC enabled to the given amplitude
      Valid gain values are 1 or 2, 8, 16, and 32.      
   */
  uint16_t adc_resistance_read(int ipos, int current, int gain = 1);

  /** Setup the given eclk 
      \param ieclk Number of the ECLK on the mezzanine interface 
      \param rate LHC-nominal clock rate in MHz -- 0/40/80/160/320/640/1280 -- zero disables the driver
      \param polarity Select the polarity
      \param strength Values between 1-7
   */
  void setup_eclk(int ieclk, int rate, bool polarity = true, int strength = 4);

  /** Setup the given elink-rx 
      \param ierx ERx index (0-5)
      \param
      On the LDMX lpGBT mezzanine, each elink-rx is configured with a single active pair.
   */
  void setup_erx(int ierx, int speed, bool invert = false, bool term = true,
                 int equalization = 0, bool acbias = false);

 private:
  lpGBT_ConfigTransport& tport_;
};

}  // namespace pflib

#endif  // PFLIB_lpGBT_H_INCLUDED
