#ifndef PFLIB_I2C_H_
#define PFLIB_I2C_H_

#include "pflib/WishboneTarget.h"
#include <vector>

namespace pflib {

/**
 * @class I2C
 * @brief Class which encapsulates the I2C controller in the Polarfire
 */
class I2C : public WishboneTarget {
 public:
  I2C(WishboneInterface* wb, int target = tgt_I2C) 
    : WishboneTarget(wb,target) {}

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
   * Get the speed for the bus in kbps
   */
  int get_bus_speed();
  
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

  /** 
   * Carry out a write of zero or more bytes followed a by a read of zero or more bytes.  If either write or read is zero bytes long, it is omitted.  Currently implemented using software (slow), while waiting for better support in the firmware.
   * @throws pflib::Exception in the case of I2C communication failure
   */
  std::vector<uint8_t> general_write_read(uint8_t i2c_dev_addr, const std::vector<uint8_t>& wdata, int nread=0);


  /** 
   * Turn on/off the horrible hack for the V1 backplane
   */
  void backplane_hack(bool enable);

 private:
  /**
   * microseconds per byte
   */
  int usec_per_byte_;
  
  /** utility for software-mode I2C */
  bool i2c_wait(bool nack_ok=false);
  
};

}

#endif // PFLIB_I2C_H_
