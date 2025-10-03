#include "pflib/Hcal.h"
#include "pflib/utility/string_format.h"

namespace pflib {

Hcal::Hcal() {    
  nhgcroc_ = 0;
  necond_ = 0;
  necont_ = 0;
}
  
void Hcal::add_roc(int iroc, std::shared_ptr<I2C>& roc_i2c, std::shared_ptr<I2C>& bias_i2c, std::shared_ptr<I2C>& board_i2c) {
  if (have_roc(iroc)) {
    PFEXCEPTION_RAISE("DuplicateROC",pflib::utility::string_format("Already have registered ROC with id %d",iroc));
  }
  nhgcroc_++;
  I2C_per_ROC handles;
  handles.roc_i2c_=roc_i2c;
  handles.bias_i2c_=bias_i2c;
  handles.board_i2c_=board_i2c;
  i2c_for_rocbd_[iroc]=handles;
}

ECON Hcal::econ(int which, const std::string& type_econ) {
  if (!have_econ(which, type_econ)) {
    PFEXCEPTION_RAISE("InvalidECONid",
      pflib::utility::string_format("Unknown ECON id %d (type %s)", which, type_econ.c_str()));
  }

  if (type_econ == "econd") {
    return ECON(*i2c_for_econd_[which], 0x60 | (which * 8), type_econ);
  } else if (type_econ == "econt") {
    return ECON(*i2c_for_econt_[which], 0x20 | (which * 8), type_econ);
  } else {
    throw std::runtime_error("Unknown ECON type in econ(): " + type_econ);
  }
}

void Hcal::add_econ(int iecon, std::shared_ptr<I2C> econ_i2c, const std::string& type_econ) {
  if (have_econ(iecon, type_econ)) {
    PFEXCEPTION_RAISE("DuplicateECON",
      pflib::utility::string_format("Already have registered %s with id %d", type_econ.c_str(), iecon));
  }

  if (type_econ == "econd") {
    i2c_for_econd_[iecon] = econ_i2c;
    necond_++;
  } else if (type_econ == "econt") {
    i2c_for_econt_[iecon] = econ_i2c;
    necont_++;
  } else {
    throw std::runtime_error("Unknown ECON type in add_econ: " + type_econ);
  }
}
  
ROC Hcal::roc(int which, const std::string& roc_type_version) {
  if (!have_roc(which)) {
    PFEXCEPTION_RAISE("InvalidROCid", pflib::utility::string_format("Unknown ROC id %d",which));
  }
  return ROC((*i2c_for_rocbd_[which].roc_i2c_), 0x20 | (which * 8), roc_type_version);
}

Bias Hcal::bias(int which) {
  if (!have_roc(which)) {
    PFEXCEPTION_RAISE("InvalidBoardId", pflib::utility::string_format("Unknown board id %d",which));
  }

  return Bias((i2c_for_rocbd_[which].bias_i2c_));
}

void Hcal::hardResetROCs() {}

void Hcal::softResetROC(int which) {}

void Hcal::hardResetECONs() {}

void Hcal::softResetECONs() {}
  
uint32_t Hcal::getFirmwareVersion() { return 0; }
  
}  // namespace pflib
