#include "lpgbt_mezz_tester.h"

LPGBT_Mezz_Tester::LPGBT_Mezz_Tester() : uio_("/dev/uio4") {
  printf("%08x %08x\n",uio_.read(0x20),uio_.read(0x21));
}

std::vector<float> LPGBT_Mezz_Tester::clock_rates() {
  std::vector<float> retvals;
  for (int iclk=0; iclk<8; iclk++) {
    uint32_t val=uio_.read(0x20+8+iclk);
    retvals.push_back(val/1e4);
  }
  return retvals;
}
