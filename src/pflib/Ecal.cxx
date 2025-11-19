#include "pflib/Ecal.h"
#include "pflib/lpgbt/I2C.h"
#include "pflib/utility/string_format.h"

namespace pflib {

static constexpr int I2C_ECON_D = 0x64;
static constexpr int I2C_ECON_T = 0x24;
static constexpr int I2C_ROCS[] = { 0x08, 0x18, 0x28, 0x48, 0x58, 0x68 };

EcalModule::EcalModule(lpGBT& lpgbt, int i2cbus, int imodule) : lpGBT_{lpgbt}, i2cbus_{i2cbus}, imodule_{imodule} {
  i2c_=std::make_unique<pflib::lpgbt::I2C>(lpGBT_,i2cbus_);
  econs_.push_back(ECON(*i2c_, I2C_ECON_D, "econd"));
  econs_.push_back(ECON(*i2c_, I2C_ECON_T, "econt"));
  for (int i=0; i<nrocs(); i++)
    rocs_.push_back(ROC(*i2c_,I2C_ROCS[i],"si_rocv3b")); // confirm this is the right version
}

std::vector<int> EcalModule::roc_ids() const {
  std::vector<int> ids;
  for (int i=0; i<nrocs(); i++) ids.push_back(i);
  return ids;
}
std::vector<int> EcalModule::econ_ids() const {
  std::vector<int> ids;
  for (int i=0; i<necons(); i++) ids.push_back(i);
  return ids;
}

ROC& EcalModule::roc(int which) {
  if (which<0 || which>=nrocs()) {
    PFEXCEPTION_RAISE("InvalidROC", "Invalid ROC requested");
  }
  return rocs_[which];
}

ECON& EcalModule::econ(int which) {
  if (which<0 || which>=nrocs()) {
    PFEXCEPTION_RAISE("InvalidECON", "Invalid ECON requested");
  }
  return econs_[which];
}

using pflib::utility::string_format;

void EcalModule::hardResetROCs() {
  GPIO& gpio=lpGBT_.gpio_interface();
  gpio.setGPO(string_format("M%d_ROC_RE_Hb",imodule_),false);
  gpio.setGPO(string_format("M%d_ROC_RE_Hb",imodule_),true);
}
void EcalModule::hardResetECONs() {
  GPIO& gpio=lpGBT_.gpio_interface();
  gpio.setGPO(string_format("M%d_ECON_RE_Hb",imodule_),false);
  gpio.setGPO(string_format("M%d_ECON_RE_Hb",imodule_),true);
}
void EcalModule::softResetROC() {
  GPIO& gpio=lpGBT_.gpio_interface();
  gpio.setGPO(string_format("M%d_ROC_RE_Sb",imodule_),false);
  gpio.setGPO(string_format("M%d_ROC_RE_Sb",imodule_),true);
}
void EcalModule::softResetECON() {
  GPIO& gpio=lpGBT_.gpio_interface();
  gpio.setGPO(string_format("M%d_ECON_RE_Sb",imodule_),false);
  gpio.setGPO(string_format("M%d_ECON_RE_Sb",imodule_),true);
}

}
