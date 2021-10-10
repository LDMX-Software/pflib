#ifndef PFLIB_I2C_H_
#define PFLIB_I2C_H_

#include "pflib/WishboneTarget.h"

namespace pflib {

/**
 * @class I2C
 * @brief Class which encapsulates the I2C controller in the Polarfire
 */
class I2C : public WishboneTarget {
 public:
  I2C(WishboneInterface* wb, int target = tgt_I2C) : WishboneTarget(wb,target) {
  }

  /**
   * Pick the active bus
   *  Set the value -1 to determ
   */
  void set_active_bus(int ibus);

  /** 
   * Get the currently active bus
   */
  int get_active_bus();

  /** 
   * Get the number of available busses
   */
  int get_bus_count();

  /**
   * Set the speed for the bus in kbps
   */
  void set_bus_speed(int speed=100);
  
  /** 
   * Write a single byte using the efficient interface, and wait for completion
   * @throws pflib::Exception in the case of I2C communication failure
   */
  void write_byte(uint8_t i2c_dev_addr, uint8_t data);

  /** 
   * Read a single byte using the efficient interface 
   * @throws pflib::Exception in the case of I2C communication failure
   */
  uint8_t read_byte(uint8_t i2c_dev_addr);

 private:
  /**
   * microseconds per byte
   */
  int usec_per_byte_;
  
  
};

}

#endif // PFLIB_I2C_H_
