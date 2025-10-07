#ifndef pflib_lpgbt_I2C_h_included
#define pflib_lpgbt_I2C_h_included 1

#include "pflib/I2C.h"
#include "pflib/lpGBT.h"

namespace pflib {
namespace lpgbt {

/** Synchronous I2C implementation */
class I2C : public ::pflib::I2C {
 public:
  I2C(lpGBT& lpGBT, int ibus) : lpgbt_{lpGBT}, ibus_{ibus}, ispeed_{100} {}

  virtual void set_bus_speed(int speed = 100);
  virtual int get_bus_speed();
  virtual void write_byte(uint8_t i2c_dev_addr, uint8_t data);
  virtual uint8_t read_byte(uint8_t i2c_dev_addr);
  virtual std::vector<uint8_t> general_write_read(
      uint8_t i2c_dev_addr, const std::vector<uint8_t>& wdata, int nread = 0);

 private:
  lpGBT& lpgbt_;
  int ibus_;
  int ispeed_;
};

/** Synchronous I2C implementation */
class I2CwithMux : public ::pflib::lpgbt::I2C {
 public:
  I2CwithMux(lpGBT& lpGBT, int ibus, uint8_t muxaddr, uint8_t tochoose)
      : ::pflib::lpgbt::I2C{lpGBT, ibus}, muxaddr_{muxaddr}, wval_{tochoose} {}

  virtual void write_byte(uint8_t i2c_dev_addr, uint8_t data);
  virtual uint8_t read_byte(uint8_t i2c_dev_addr);
  virtual std::vector<uint8_t> general_write_read(
      uint8_t i2c_dev_addr, const std::vector<uint8_t>& wdata, int nread = 0);

 private:
  uint8_t muxaddr_, wval_;
};
}  // namespace lpgbt
}  // namespace pflib

#endif  // pflib_lpgbt_I2C_h_included
