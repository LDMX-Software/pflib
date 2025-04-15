#ifndef PFLIB_lpGBT_ConfigTransport_I2C_H_INCLUDED
#define PFLIB_lpGBT_ConfigTransport_I2C_H_INCLUDED

#include "pflib/lpGBT.h"
#include <string>

namespace pflib {

class lpGBT_ConfigTransport_I2C : public lpGBT_ConfigTransport {
 public:
  lpGBT_ConfigTransport_I2C(uint8_t i2c_addr, const std::string& bus_dev);
  virtual ~lpGBT_ConfigTransport_I2C();

  void write_raw(uint8_t a);
  void write_raw(uint8_t a, uint8_t b);
  void write_raw(const std::vector<uint8_t>& a);

  uint8_t read_raw();
  std::vector<uint8_t> read_raw(int n);
  
  virtual uint8_t read_reg(uint16_t reg) final;
  virtual void write_reg(uint16_t reg, uint8_t value) final;

  virtual std::vector<uint8_t> read_regs(uint16_t reg, int n);
  virtual void write_regs(uint16_t reg, const std::vector<uint8_t>& value);

 private:
  int handle_;
};

}


#endif // PFLIB_lpGBT_ConfigTransport_I2C_H_INCLUDED
