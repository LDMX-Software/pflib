/**
This class exists to enable reliable, safe operation of UIO
due to issues observed with very-closely spaced writes causing 
crashes.
*/
#include <unistd.h>
#include <stdint.h>
#include <string>

namespace pflib {

  class UIO {
  public:
    /** Open the UIO device given, mapping the amount of memory indicated
     */
    UIO(const std::string& dev, size_t size=4096);
    ~UIO();

    /** Read the given word from the UIO device register space */
    uint32_t read(size_t i) { return (i<size_)?(ptr_[i]):(0xDEADBEEF); }

    /** Write the given value to the UIO device register */
    void write(size_t where, uint32_t what);

    /** Generate an RMW cycle (read, apply INVERSE OF MASK, apply OR) */
    void rmw(size_t where, uint32_t bits_to_modify, uint32_t newval);
  private:
    size_t size_;
    uint32_t* ptr_;
    int handle_;
  };
}
