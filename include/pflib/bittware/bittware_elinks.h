#pragma once

#include "pflib/Elinks.h"
#include "pflib/bittware/bittware_axilite.h"

namespace pflib {
namespace bittware {

class OptoElinksBW : public Elinks {
 public:
  OptoElinksBW(int ilink, const char* dev);
  virtual std::vector<uint32_t> spy(int ilink, bool new_capture) final;
  virtual void setBitslip(int ilink, int bitslip) final {
    /// only in ECON
  }
  virtual int getBitslip(int ilink) final { return 0; }
  virtual int scanBitslip(int ilink) final { return -1; }
  virtual uint32_t getStatusRaw(int ilink) final { return 0; }
  virtual void clearErrorCounters(int ilink) final {}
  virtual void resetHard() final {
    // not meaningful here
  }

 private:
  int ilinkOpto_;
  AxiLite axil_;
};
}  // namespace bittware
}  // namespace pflib
