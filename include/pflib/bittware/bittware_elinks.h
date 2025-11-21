#pragma once

#include "pflib/Elinks.h"
#include "pflib/bittware/bittware_axilite.h"

namespace pflib {
namespace bittware {

class OptoElinksBW : public Elinks {
 public:
  OptoElinksBW(int ilink, const char* dev = "/dev/datadev_0");
  virtual std::vector<uint32_t> spy(int ilink);
  virtual void setBitslip(int ilink, int bitslip) {
    /// only in ECON
  }
  virtual int getBitslip(int ilink) { return 0; }
  virtual int scanBitslip(int ilink) { return -1; }
  virtual uint32_t getStatusRaw(int ilink) { return 0; }
  virtual void clearErrorCounters(int ilink) {}
  virtual void resetHard() {
    // not meaningful here
  }

 private:
  int ilinkOpto_;
  AxiLite axil_;
};
}  // namespace bittware
}  // namespace pflib
