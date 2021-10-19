#ifndef pflib_Elinks_h_
#define pflib_Elinks_h_

#include "pflib/WishboneTarget.h"
#include <vector>

namespace pflib {

class Elinks : public WishboneTarget {
 public:
  Elinks(WishboneInterface* wb, int target = tgt_Elinks) : WishboneTarget(wb,target) { }
  
  std::vector<uint8_t> spy(int ilink);
  void setBitslip(int ilink, int bitslip);
  void setBitslipAuto(int ilink);
  bool isBitslipAuto(int ilink);
  int getBitslip(int ilink);
  uint32_t getStatusRaw(int ilink);
  void clearErrorCounters(int ilink);
  void readCounters(int link, uint32_t& nonidles, uint32_t& resyncs);

  /** Mode 0 -> software, Mode 1 -> L1A, Mode 2 -> Non-idle */
  void setupBigspy(int mode, int ilink, int presamples);
  void getBigspySetup(int& mode, int& ilink, int& presamples);
  bool bigspyDone();  
  std::vector<uint32_t> bigspy();

  void resetHard();
  
  void scanAlign(int ilink);
  void setDelay(int ilink, int idelay);
  
};

  
}

#endif // pflib_Elinks_h_
