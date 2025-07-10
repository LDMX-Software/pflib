#ifndef ZCU_UIO_H_INCLUDED
#define ZCU_UIO_H_INCLUDED
/**
This class exists to enable reliable, safe operation of UIO
due to issues observed with very-closely spaced writes causing
crashes.
*/
#include <stdint.h>
#include <unistd.h>

#include <string>

namespace pflib {

class UIO {
 public:
  /** Open the UIO device given a name, mapping the amount of memory indicated
   */
  UIO(const std::string& name, size_t size = 4096);

  /** Open the UIO device given, mapping the amount of memory indicated
   */
  ~UIO();

  /** Read the given word from the UIO device register space */
  uint32_t read(size_t i) { return (i < size_) ? (ptr_[i]) : (0xDEADB33F); }

  /** Write the given value to the UIO device register */
  void write(size_t where, uint32_t what);

  /** read with a mask (including shifts) */
  uint32_t readMasked(size_t where, uint32_t mask) {
    return (read(where) & mask) >> first_bit_set(mask);
  }

  /** write with a mask (including shifts) */
  void writeMasked(size_t where, uint32_t mask, uint32_t val) {
    rmw(where, mask, (val << first_bit_set(mask)) & mask);
  }

  /** Generate an RMW cycle (read, apply INVERSE OF MASK, apply OR) */
  void rmw(size_t where, uint32_t bits_to_modify, uint32_t newval);

 private:
  /** Open a given device file directly */
  void iopen(const std::string& dev, size_t size = 4096);


  int first_bit_set(uint32_t mask) {
    int i;
    for (i = 0; i < 32; i++)
      if ((mask & (1 << i)) != 0) return i;
    return 32;
  }
  size_t size_;
  uint32_t* ptr_;
  int handle_;
};
}  // namespace pflib
#endif
