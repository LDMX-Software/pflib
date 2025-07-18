#ifndef PFLIB_I2C_H_
#define PFLIB_I2C_H_

#include <stdint.h>

#include <vector>

#include "pflib/Exception.h"
#include "pflib/Logging.h"

namespace pflib {

/**
 * @class I2C
 * @brief Base class which encapsulates the I2C interface, represents a single
 * bus
 */
class I2C {
 protected:
  mutable logging::logger the_log_{logging::get("I2C")};
  I2C() {}

 public:
  /**
   * Set the speed for the bus in kbps
   */
  virtual void set_bus_speed(int speed = 100) = 0;

  /**
   * Get the speed for the bus in kbps
   */
  virtual int get_bus_speed() = 0;

  /**
   * Write a single byte using the efficient interface, and wait for completion
   * @throws pflib::Exception in the case of I2C communication failure
   */
  virtual void write_byte(uint8_t i2c_dev_addr, uint8_t data) = 0;

  /**
   * Read a single byte using the efficient interface
   * @throws pflib::Exception in the case of I2C communication failure
   */
  virtual uint8_t read_byte(uint8_t i2c_dev_addr) = 0;

  /**
   * Carry out a write of zero or more bytes followed a by a read of zero or
   more bytes.  If either write or read is zero bytes long, it is omitted.

   * @throws pflib::Exception in the case of I2C communication failure
   */
  virtual std::vector<uint8_t> general_write_read(
      uint8_t i2c_dev_addr, const std::vector<uint8_t>& wdata,
      int nread = 0) = 0;
  
  virtual std::vector<uint8_t> general_write_read_ioctl(
      int i2c_dev_addr, const std::vector<uint8_t>& wdata,
      int nread = 0) = 0;
};

}  // namespace pflib

#endif  // PFLIB_I2C_H_
