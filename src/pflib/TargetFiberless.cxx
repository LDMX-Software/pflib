#include "pflib/Target.h"
#include "pflib/I2C_Linux.h"

namespace pflib {

class TargetFiberless : public Target {
 public:
  TargetFiberless() : Target() {
    
    i2croc_=std::shared_ptr<I2C>(new I2C_Linux("/dev/i2c-24"));
    i2cboard_=std::shared_ptr<I2C>(new I2C_Linux("/dev/i2c-23"));

    i2c_["HGCROC"]=i2croc_;
    i2c_["BOARD"]=i2cboard_;
    i2c_["BIAS"]=i2cboard_;

    std::vector<std::shared_ptr<I2C>> roc_i2cs;
    roc_i2cs.push_back(i2croc_);

    hcal_=std::shared_ptr<Hcal>(new Hcal(roc_i2cs));
    fc_=std::shared_ptr<FastControl>(make_FastControlCMS_MMap());
  }
 private:
  std::shared_ptr<I2C> i2croc_;
  std::shared_ptr<I2C> i2cboard_;
};

Target* makeTargetFiberless() { return new TargetFiberless(); }

}
