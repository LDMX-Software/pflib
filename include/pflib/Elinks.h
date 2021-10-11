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
  int getBitslip(int ilink);
  uint32_t getStatusRaw(int ilink);

  void scanAlign(int ilink);
  
};

  
}

#endif // pflib_Elinks_h_
