#include "pflib/bittware/bittware_elinks.h"
#include <unistd.h>

namespace pflib {
namespace bittware {

static constexpr uint32_t QUAD_CODER0_BASE_ADDRESS =
    0x3000;  // compiled into the firmware

OptoElinksBW::OptoElinksBW(int ilink, const char* dev)
    : Elinks(1), ilinkOpto_(ilink), axil_(QUAD_CODER0_BASE_ADDRESS, dev) {}
std::vector<uint32_t> OptoElinksBW::spy(int ilink) {
  std::vector<uint32_t> retval;
  // spy now...
  static constexpr int REG_PULSE = 0x100;
  static constexpr int BIT_SPY_START = 16;
  static constexpr int REG_READ_WHICH = 0x600;
  static constexpr int MASK_READ_WHICH = 0xFF00;
  static constexpr int REG_SPY_BASE = 0x880;

  axil_.write(REG_PULSE, (1 << BIT_SPY_START));
  axil_.writeMasked(REG_READ_WHICH, MASK_READ_WHICH, ilink);
  usleep(1000);
  for (int i = 0; i < 32; i++) {
    usleep(1);
    retval.push_back(axil_.read(REG_SPY_BASE + i * 4));
  }
  return retval;
}

}  // namespace bittware
}  // namespace pflib
