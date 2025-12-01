#include "pflib/Elinks.h"
#include "pflib/lpGBT.h"
#include "pflib/zcu/UIO.h"

namespace pflib {
namespace zcu {

/** Currently represents all elinks for dual-link configuration */
class OptoElinksZCU : public Elinks {
 public:
  OptoElinksZCU(lpGBT* lpdaq, lpGBT* lptrig, int itarget);
  virtual std::vector<uint32_t> spy(int ilink);
  virtual void setBitslip(int ilink, int bitslip);
  virtual int getBitslip(int ilink);
  virtual int scanBitslip(int ilink) { return -1; }
  virtual uint32_t getStatusRaw(int ilink) { return 0; }
  virtual void clearErrorCounters(int ilink) {}
  virtual void resetHard() {
    // not meaningful here
  }

 private:
  lpGBT *lp_daq_, *lp_trig_;
  UIO uiodecoder_;
};

}  // namespace zcu
}  // namespace pflib
