#include "pflib/Hcal.h"

namespace pflib {

Hcal::Hcal(const std::vector<std::shared_ptr<I2C>>& roc_i2c, const std::vector<std::shared_ptr<I2C>>& bias_i2c)
    : roc_i2c_{roc_i2c}, bias_i2c_{bias_i2c} {
  nhgcroc_ = int(roc_i2c.size());
}

ROC Hcal::roc(int which, const std::string& roc_type_version) {
  if (which < 0 || which >= nhgcroc_) {
    PFEXCEPTION_RAISE("InvalidROCid", "Requested out-of-range ROC id");
  }
  return ROC(*roc_i2c_[which], 0x20 | (which * 8), roc_type_version);
}

Bias Hcal::bias(int which) {
  if (which<0 || which>=nhgcroc_) {
    PFEXCEPTION_RAISE("InvalidBoardId","Requested out-of-range Board id");
  }

  return Bias(bias_i2c_.at(which));
}


void Hcal::hardResetROCs() {}

void Hcal::softResetROC(int which) {}

uint32_t Hcal::getFirmwareVersion() { return 0; }

}  // namespace pflib
