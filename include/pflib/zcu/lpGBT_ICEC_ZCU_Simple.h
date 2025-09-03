#ifndef PFLIB_lpGBT_ICEC_H_INCLUDED
#define PFLIB_lpGBT_ICEC_H_INCLUDED

#include "pflib/lpGBT.h"
#include "pflib/zcu/UIO.h"

namespace pflib {

namespace zcu {

/**
   * @class lpGBT configuration transport interface class partially specified
   * for IC/EC communication
   */
class lpGBT_ICEC_Simple : public lpGBT_ConfigTransport {
 public:
  lpGBT_ICEC_Simple(const std::string& target, bool isEC,
                    uint8_t lpgbt_i2c_addr);
  virtual ~lpGBT_ICEC_Simple() {}

  virtual std::vector<uint8_t> read_regs(uint16_t reg, int n);
  virtual void write_regs(uint16_t reg, const std::vector<uint8_t>& value);
  virtual uint8_t read_reg(uint16_t reg);
  virtual void write_reg(uint16_t reg, uint8_t value);

 private:
  /// Offset depending on EC/IC
  int offset_;
  /// i2c address of the device
  uint8_t lpgbt_i2c_addr_;
  /// UIO block
  UIO transport_;
};

}  // namespace zcu
}  // namespace pflib

#endif  // PFLIB_lpGBT_ICEC_H_INCLUDED
