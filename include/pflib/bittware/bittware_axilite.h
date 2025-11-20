#ifndef PFLIB_BITTWARE_AXILITE
#define PFLIB_BITTWARE_AXILITE 1

#include <stdint.h>

namespace pflib {
namespace bittware {

/** This class provides access to read and write via the ioctl path to the
   AXILite targets within the Bittware firwmare. All addresses are full bus
   addresses relative to the base_address, including the two LSB which should
   always be zero.
*/
class AxiLite {
 public:
  AxiLite(const uint32_t base_address, const char* dev,
          const uint32_t mask_space = 0x3FFFFF);
  ~AxiLite();

  /// get the device path this AxiLite is connected to
  const char* dev() const;

  /// Read a register, throws an exception if bits are set outside the addr mask
  /// space (or in the two LSB)
  uint32_t read(uint32_t addr);
  /// Write a register, throws an exception if bits are set outside the addr
  /// mask space (or in the two LSB)
  void write(uint32_t addr, uint32_t value);
  /// Read a register, doing shifts as necessary, throws an exception if bits
  /// are set outside the addr mask space (or in the two LSB)
  uint32_t readMasked(uint32_t addr, uint32_t mask);
  /// Write a register, doing shifts as necessary, throws an exception if bits
  /// are set outside the addr mask space (or in the two LSB)
  void writeMasked(uint32_t addr, uint32_t mask, uint32_t value);
  /// check to see if a given bit is set
  bool isSet(uint32_t addr, int ibit) {
    return readMasked(addr, (1 << ibit)) != 0;
  }
  /// set or clear a given bit
  void setclear(uint32_t addr, int ibit, bool true_for_set = false) {
    writeMasked(addr, (1 << ibit), (true_for_set) ? (1) : (0));
  }

 private:
  const char* dev_;    /// path to device
  uint32_t base_;      // base address
  uint32_t mask_;      // mask (for safety)
  uint32_t antimask_;  // mask (for safety)
  int handle_;
  bool waswrite_;
};

}  // namespace bittware
}  // namespace pflib

#endif  // PFLIB_BITTWARE_AXILITE
