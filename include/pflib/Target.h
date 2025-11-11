#ifndef PFLIB_TARGET_H_INCLUDED
#define PFLIB_TARGET_H_INCLUDED

#include "pflib/FastControl.h"
#include "pflib/Hcal.h"
#include "pflib/I2C.h"

namespace pflib {

class OptoLink;

/**
 * @class Target class for encapulating a given setup's access rules
 */
class Target {
 public:
  ~Target() {}

  std::vector<std::string> i2c_bus_names();
  I2C& get_i2c_bus(const std::string& name);

  Hcal& hcal() { return *hcal_; }

  FastControl& fc() { return *fc_; }

  const std::vector<OptoLink*>& optoLinks() const { return opto_; }

  /**
   * types of daq formats that we can do
   */
  enum class DaqFormat {
    /// simple format for direct HGCROC connection
    SIMPLEROC = 1,
    /// ECON-D without applying zero suppression
    ECOND_NO_ZS = 2
  };

  virtual void setup_run(int irun, DaqFormat format, int contrib_id = -1) {}
  virtual std::vector<uint32_t> read_event() = 0;
  virtual bool has_event() { return hcal().daq().getEventOccupancy() > 0; }

 protected:
  std::map<std::string, std::shared_ptr<I2C> > i2c_;

  std::shared_ptr<Hcal> hcal_;
  std::shared_ptr<FastControl> fc_;
  std::vector<OptoLink*> opto_;
  mutable logging::logger the_log_{logging::get("Target")};
};

Target* makeTargetFiberless();
Target* makeTargetHcalBackplaneZCU(int ilink, uint8_t board_mask);

}  // namespace pflib

#endif  // PFLIB_TARGET_H_INCLUDED
