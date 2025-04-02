#ifndef PFLIB_I2C_Linux_H_
#define PFLIB_I2C_Linux_H_

#include <string>

#include "pflib/I2C.h"

namespace pflib {

/**
 * @class I2C
 * @brief Base class which encapsulates the I2C interface, represents a single
 * bus
 */
class I2C_Linux : public I2C {
 public:
  I2C_Linux(const std::string& devfile);

  ~I2C_Linux();

  /**
   * Set the speed for the bus in kbps
   */
  virtual void set_bus_speed(int speed = 100);

  /**
   * Get the speed for the bus in kbps
   */
  virtual int get_bus_speed();

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
   * Carry out a write of zero or more bytes followed a by a read of zero or
   more bytes.  If either write or read is zero bytes long, it is omitted.

   * @throws pflib::Exception in the case of I2C communication failure
   */
  std::vector<uint8_t> general_write_read(uint8_t i2c_dev_addr,
                                          const std::vector<uint8_t>& wdata,
                                          int nread = 0);

 private:
  int handle_;
  std::string dev_;
};

}  // namespace pflib

#endif  // PFLIB_I2C_H_
