#ifndef PFLIB_TARGET_H_INCLUDED
#define PFLIB_TARGET_H_INCLUDED

#include "pflib/FastControl.h"
#include "pflib/Hcal.h"
#include "pflib/I2C.h"

namespace pflib {

/**
 * @class Target class for encapulating a given setup's access rules
 */
class Target {
 public:
  std::vector<std::string> i2c_bus_names();
  I2C& get_i2c_bus(const std::string& name);

  Hcal& hcal() { return *hcal_; }

  FastControl& fc() { return *fc_; }

  virtual std::vector<uint32_t> read_event() = 0;
  virtual bool has_event() { return hcal().daq().getEventOccupancy() > 0; }

 protected:
  std::map<std::string, std::shared_ptr<I2C> > i2c_;

  std::shared_ptr<Hcal> hcal_;
  std::shared_ptr<FastControl> fc_;
};

Target* makeTargetFiberless();

}  // namespace pflib

#endif  // PFLIB_TARGET_H_INCLUDED
