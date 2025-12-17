#include "pflib/DAQ.h"
#include "pflib/zcu/UIO.h"

namespace pflib {
namespace zcu {

/**
 * Data capture on ZCU when connected with an optical fiber
 */
class ZCU_Capture : public DAQ {
 public:
  virtual ~ZCU_Capture() = default;
  ZCU_Capture();
  virtual void reset() final;
  virtual int getEventOccupancy() final;
  virtual void setupLink(int ilink, int l1a_delay, int l1a_capture_width) final {
    // none of these parameters are relevant for the econd capture, which is
    // data-pattern based
  }
  virtual void getLinkSetup(int ilink, int& l1a_delay, int& l1a_capture_width) final {
    l1a_delay = -1;
    l1a_capture_width = -1;
  }
  virtual void bufferStatus(int ilink, bool& empty, bool& full) final;
  virtual void setup(int econid, int samples_per_ror, int soi) final;
  virtual void enable(bool doenable) final;
  virtual bool enabled() final;
  virtual std::vector<uint32_t> getLinkData(int ilink) final;
  virtual void advanceLinkReadPtr() final;
  virtual std::map<std::string, uint32_t> get_debug(uint32_t ask) final;


  /**
   * read out a whole event given the current setup
   *
   * since the ZCU firmware does not insert some headers we expect
   * in the final system using Bittwares, we attempt to emulate that
   * behavior and manually insert them.
   */
  std::vector<uint32_t> read_event_sw_headers();

 private:
  UIO capture_;
  bool per_econ_;
};

}  // namespace zcu
}  // namespace pflib
