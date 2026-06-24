#include "pflib/Elinks.h"
#include "pflib/lpGBT.h"
#include "pflib/zcu/UIO.h"

namespace pflib {
namespace zcu {

/** Currently represents all elinks for dual-link configuration */
class OptoElinksZCU : public Elinks {
 public:
  OptoElinksZCU(lpGBT* lpdaq, lpGBT* lptrig, int itarget);
  virtual std::vector<uint32_t> spy(int ilink, bool new_capture) final;
  virtual void setBitslip(int ilink, int bitslip) final;
  virtual int getBitslip(int ilink) final;
  virtual int scanBitslip(int ilink) final { return -1; }
  virtual uint32_t getStatusRaw(int ilink) final { return 0; }
  virtual void clearErrorCounters(int ilink) final {}
  virtual void resetHard() final {
    // not meaningful here
  }

 private:
  lpGBT *lp_daq_, *lp_trig_;
  UIO uiodecoder_;
};

}  // namespace zcu
}  // namespace pflib
