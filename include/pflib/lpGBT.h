#ifndef PFLIB_lpGBT_H_INCLUDED
#define PFLIB_lpGBT_H_INCLUDED

#include "pflib/Exception.h"
#include <stdint.h>
#include <vector>

namespace pflib {

/**
 * @class lpGBT configuration transport inferface class
 */
class lpGBT_ConfigTransport {
 public:
  virtual ~lpGBT_ConfigTransport() { }
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

/** Class which provides an interface with an lpGBT ASIC.
    Includes both low-level and medium-level interfaces.
    Uses an instance of a lpGBT_ConfigTransport for communication.
*/
class lpGBT {
 public:
  lpGBT(lpGBT_ConfigTransport& transport);


  void write(uint16_t reg, uint8_t value) { tport_.write_reg(reg, value); }
  uint8_t read(uint16_t reg) { return tport_.read_reg(reg); }
  
  typedef std::pair<uint16_t,uint8_t> RegisterValue;
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
  /** Set the given gpio bit */
  void gpio_set(int ibit, bool high);
  /** Get the given gpio bit */
  bool gpio_get(int ibit);
  /** Set the full GPIO array */
  void gpio_set(uint16_t values);
  /** Get the full GPIO array */
  uint16_t gpio_set();
  
  
 private:
  lpGBT_ConfigTransport& tport_;
};
  
}

#endif // PFLIB_lpGBT_H_INCLUDED
