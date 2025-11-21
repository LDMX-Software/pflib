#include "pflib/bittware/bittware_axilite.h"

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

#include <map>

#include "pflib/Exception.h"
#include "pflib/utility/string_format.h"
#include "rogue/hardware/drivers/AxisDriver.h"

namespace pflib {
namespace bittware {

static std::map<std::string, int> handle_map;

AxiLite::AxiLite(const uint32_t base_address, const char* dev,
                 const uint32_t mask_space)
    : dev_{dev},
      base_{base_address | 0x00c00000},
      mask_{mask_space & 0xFFFFFFFCu},
      antimask_{0xFFFFFFFFu ^ mask_}, waswrite_{true} {
  auto ptr = handle_map.find(dev);
  if (ptr != handle_map.end())
    handle_ = ptr->second;
  else {
    handle_ = open(dev, O_RDWR);
    if (handle_ < 0) {
      PFEXCEPTION_RAISE(
          "FileOpenException",
          pflib::utility::string_format("Error %s (%d) on opening '%s'",
                                        strerror(errno), errno, dev));
    }
    handle_map[dev] = handle_;
  }
}

AxiLite::~AxiLite() {
  // let the system close them...
}

const char* AxiLite::dev() const { return dev_; }

uint32_t AxiLite::read(uint32_t addr) {
  if ((addr & antimask_) != 0) {
    PFEXCEPTION_RAISE("InvalidAddress", pflib::utility::string_format(
                                            "Address 0x%0x is invalid", addr));
  }
  uint32_t val;
  if (waswrite_) { // seem to need this double read, would be good to fix at firmware level...
      dmaReadRegister(handle_, base_ | addr, &val);
      waswrite_=false;
  }
  dmaReadRegister(handle_, base_ | addr, &val);
  return val;
}

void AxiLite::write(uint32_t addr, uint32_t val) {
  if ((addr & antimask_) != 0) {
    PFEXCEPTION_RAISE("InvalidAddress", pflib::utility::string_format(
                                            "Address 0x%0x is invalid", addr));
  }
  dmaWriteRegister(handle_, base_ | addr, val);
  waswrite_=true;
}

static int first_bit_set(uint32_t mask) {
  int i;
  for (i = 0; i < 32; i++)
    if ((mask & (1 << i)) != 0) return i;
  return 32;
}

uint32_t AxiLite::readMasked(uint32_t addr, uint32_t mask) {
  uint32_t val = read(addr);
  return (val & mask) >> (first_bit_set(mask));
}

void AxiLite::writeMasked(uint32_t addr, uint32_t mask, uint32_t nval) {
  uint32_t val = read(addr);
  val = (val & (0xffffffffu ^ mask)) | ((nval << first_bit_set(mask)) & mask);
  write(addr, val);
}

}  // namespace bittware
}  // namespace pflib
