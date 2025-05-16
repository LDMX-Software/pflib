#include "pflib/FastControl.h"

namespace pflib {

static const int REG_CONTROL1 = 1;
static const uint32_t CTL_COUNTER_CLEAR = 0x00000001u;
static const int REG_CONTROL2 = 2;
static const uint32_t CTL_TX_PHASE_MASK = 0x0000000Fu;
static const int CTL_TX_PHASE_SHIFT = 0;
static const uint32_t CTL_TX_DRIVE_RESET = 0x00000040u;
static const uint32_t CTL_MULTISAMPLE_ENABLE = 0x00080000u;
static const uint32_t CTL_MULTISAMPLE_MASK = 0x00070000u;
static const int CTL_MULTISAMPLE_SHIFT = 16;

static const int REG_SINGLE_ERROR_COUNTER = 4;
static const int REG_DOUBLE_ERROR_COUNTER = 5;
static const int REG_CMD_COUNTER_BASE = 8;
static const int CMD_COUNT = 8;

std::vector<uint32_t> FastControl::getCmdCounters() {
  std::vector<uint32_t> rv;
  for (int i = 0; i < CMD_COUNT; i++)
    rv.push_back(wb_read(REG_CMD_COUNTER_BASE + i));
  return rv;
}

void FastControl::getErrorCounters(uint32_t& single_bit_errors,
                                   uint32_t& double_bit_errors) {
  single_bit_errors = wb_read(REG_SINGLE_ERROR_COUNTER);
  double_bit_errors = wb_read(REG_DOUBLE_ERROR_COUNTER);
}

void FastControl::resetCounters() { wb_write(REG_CONTROL1, CTL_COUNTER_CLEAR); }

void FastControl::resetTransmitter() {
  wb_write(REG_CONTROL2, 0);  // useful to get a clear on this register
  wb_rmw(REG_CONTROL2, CTL_TX_DRIVE_RESET, CTL_TX_DRIVE_RESET);
  wb_rmw(REG_CONTROL2, 0, CTL_TX_DRIVE_RESET);
}

void FastControl::setupMultisample(bool enable, int extra_samples) {
  wb_rmw(REG_CONTROL2, extra_samples << CTL_MULTISAMPLE_SHIFT,
         CTL_MULTISAMPLE_MASK);
  if (enable) {
    wb_rmw(REG_CONTROL2, CTL_MULTISAMPLE_ENABLE, CTL_MULTISAMPLE_ENABLE);
  } else {
    wb_rmw(REG_CONTROL2, 0, CTL_MULTISAMPLE_ENABLE);
  }
}

void FastControl::getMultisampleSetup(bool& enable, int& extra_samples) {
  uint32_t val = wb_read(REG_CONTROL2);
  enable = ((val & CTL_MULTISAMPLE_ENABLE) != 0);
  extra_samples = ((val & CTL_MULTISAMPLE_MASK) >> CTL_MULTISAMPLE_SHIFT);
}

}  // namespace pflib
