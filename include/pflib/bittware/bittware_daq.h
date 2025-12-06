#pragma once
#include "pflib/DAQ.h"
#include "pflib/bittware/bittware_axilite.h"
#include "pflib/logging/Logging.h"

namespace pflib {
namespace bittware {

class HcalBackplaneBW_Capture : public DAQ {
 public:
  HcalBackplaneBW_Capture(const char* dev);
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
  virtual void setup(int econid, int samples_per_ror, int soi);
  virtual void enable(bool doenable);
  virtual bool enabled();
  virtual void AXIS_enable(bool doenable);
  virtual bool AXIS_enabled();
  virtual std::vector<uint32_t> getLinkData(int ilink);
  virtual void advanceLinkReadPtr();
  virtual std::map<std::string, uint32_t> get_debug(uint32_t ask);

 private:
  AxiLite capture_;
  bool per_econ_;
  mutable logging::logger the_log_;
};

}  // namespace bittware
}  // namespace pflib
