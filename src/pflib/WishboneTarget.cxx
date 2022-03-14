#include "pflib/WishboneTarget.h"

namespace pflib {

void WishboneTarget::wb_rmw(uint32_t addr, uint32_t data, uint32_t mask) {
  uint32_t val=wb_read(addr);
  uint32_t imask=0xffffffffu^mask;
  val=(val&imask) | (data & mask);
  wb_write(addr,val);
}

int WishboneTarget::firmware_version() {
  return wb_read(0)&0xffff;
}
  
}
