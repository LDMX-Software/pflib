#include "pflib/DAQ.h"
#include "pflib/zcu/UIO.h"

namespace pflib {
namespace zcu {

class ZCU_Capture : public DAQ {
 public:
  ZCU_Capture();
  virtual void reset();
  virtual int getEventOccupancy();
  virtual void setupLink(int ilink, int l1a_delay, int l1a_capture_width) {
    // none of these parameters are relevant for the econd capture, which is
    // data-pattern based
  }
  virtual void getLinkSetup(int ilink, int& l1a_delay, int& l1a_capture_width) {
    l1a_delay = -1;
    l1a_capture_width = -1;
  }
  virtual void bufferStatus(int ilink, bool& empty, bool& full);
  virtual void setup(int samples_per_ror, int soi);
  virtual void setEconId(int ilink, int econid);
  virtual void enable(bool doenable);
  virtual bool enabled();
  virtual std::vector<uint32_t> getLinkData(int ilink);
  virtual void advanceLinkReadPtr(int ilink);
  virtual std::map<std::string, uint32_t> get_debug(uint32_t ask);

 private:
  UIO capture_;
  bool per_econ_;
};

}  // namespace zcu
}  // namespace pflib
