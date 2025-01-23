#ifndef PFLIB_TARGET_H_INCLUDED
#define PFLIB_TARGET_H_INCLUDED

#include "pflib/I2C.h"
#include "pflib/Hcal.h"
#include "pflib/FastControl.h"

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
  
 protected:
  std::map<std::string, std::shared_ptr<I2C> > i2c_;

  std::shared_ptr<Hcal> hcal_;
  std::shared_ptr<FastControl> fc_;
  
};

  Target* makeTargetFiberless();
  
}

#endif // PFLIB_TARGET_H_INCLUDED
