
#include "pflib/OptoLink.h"

namespace pflib {

lpGBT& OptoLink::lpgbt() {
  if (!lpgbt_) lpgbt_ = std::make_unique<lpGBT>(lpgbt_transport());
  return *lpgbt_;
}

void OptoLink::print_status() {
  printf("Polarity -- TX: %d  RX: %d\n", get_tx_polarity(), get_rx_polarity());
  std::map<std::string, uint32_t> info;
  info = opto_status();
  printf("Optical status:\n");
  for (auto i : info) {
    printf("  %-20s : 0x%04x\n", i.first.c_str(), i.second);
  }
  info = opto_rates();
  printf("Optical rates:\n");
  for (auto i : info) {
    printf("  %-20s : %.3f MHz (0x%04x)\n", i.first.c_str(), i.second / 1e3,
           i.second);
  }
}

}  // namespace pflib
