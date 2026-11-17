#include "pflib/EcalModule.h"

#include "pflib/lpgbt/I2C.h"
#include "pflib/utility/string_format.h"

namespace pflib {

static constexpr int I2C_ECON_D = 0x64;
static constexpr int I2C_ECON_T = 0x24;
static constexpr int I2C_ROCS[] = {0x08, 0x18, 0x28, 0x48, 0x58, 0x68};

EcalModule::EcalModule(lpGBT& lpgbt, int i2cbus, int modulenumber,
                       uint8_t roc_mask)
    : lpGBT_{lpgbt}, i2cbus_{i2cbus}, imodule_{modulenumber} {
  n_rocs_ = 0;
  n_econs_ = 0;
  i2c_ = std::make_shared<pflib::lpgbt::I2C>(lpGBT_, i2cbus_);
  econs_[ECON_D] = std::make_unique<ECON>(i2c_, I2C_ECON_D, "econd");
  n_econs_++;
  econs_[ECON_T] = std::make_unique<ECON>(i2c_, I2C_ECON_T, "econt");
  n_econs_++;
  for (int i = 0; i < rocs_.size(); i++) {
    if ((roc_mask & (1 << i)) == 0) continue;
    rocs_[i] = std::make_unique<ROC>(i2c_, I2C_ROCS[i], "si_rocv3b");
    n_rocs_++;
  }
}

int EcalModule::nrocs() const { return n_rocs_; }

int EcalModule::necons() const { return n_econs_; }

bool EcalModule::have_roc(int which) const {
  if (which < 0 or which >= rocs_.size()) {
    return false;
  }
  return bool(rocs_[which]);
}

bool EcalModule::have_econ(int which) const {
  if (which < 0 or which >= econs_.size()) {
    return false;
  }
  return bool(econs_[which]);
}

std::vector<int> EcalModule::roc_ids() const {
  std::vector<int> ids;
  for (int i = 0; i < rocs_.size(); i++) {
    if (rocs_[i]) {
      ids.push_back(i);
    }
  }
  return ids;
}

std::vector<int> EcalModule::econ_ids() const {
  std::vector<int> ids;
  for (int i = 0; i < econs_.size(); i++) {
    if (econs_[i]) {
      ids.push_back(i);
    }
  }
  return ids;
}

ROC& EcalModule::roc(int which) {
  if (which < 0 || which >= rocs_.size()) {
    PFEXCEPTION_RAISE("InvalidROC",
                      "ROC " + std::to_string(which) + " is invalid");
  }
  if (not rocs_[which]) {
    PFEXCEPTION_RAISE("DisabledROC",
                      "ROC " + std::to_string(which) + " is disabled");
  }
  return *(rocs_[which]);
}

ECON& EcalModule::econ(int which) {
  if (which < 0 || which >= econs_.size()) {
    PFEXCEPTION_RAISE("InvalidECON",
                      "ECON " + std::to_string(which) + " is invalid");
  }
  if (not econs_[which]) {
    PFEXCEPTION_RAISE("DisabledECON",
                      "ECON " + std::to_string(which) + " is disabled");
  }
  return *(econs_[which]);
}

using pflib::utility::string_format;

void EcalModule::hardResetROCs() {
  GPIO& gpio = lpGBT_.gpio_interface();
  gpio.setGPO(string_format("M%d_ROC_RE_Hb", imodule_), false);
  gpio.setGPO(string_format("M%d_ROC_RE_Hb", imodule_), true);
}
void EcalModule::hardResetECONs() {
  GPIO& gpio = lpGBT_.gpio_interface();
  gpio.setGPO(string_format("M%d_ECON_RE_Hb", imodule_), false);
  gpio.setGPO(string_format("M%d_ECON_RE_Hb", imodule_), true);
}
void EcalModule::softResetROC() {
  GPIO& gpio = lpGBT_.gpio_interface();
  gpio.setGPO(string_format("M%d_ROC_RE_Sb", imodule_), false);
  gpio.setGPO(string_format("M%d_ROC_RE_Sb", imodule_), true);
}
void EcalModule::softResetECON() {
  GPIO& gpio = lpGBT_.gpio_interface();
  gpio.setGPO(string_format("M%d_ECON_RE_Sb", imodule_), false);
  gpio.setGPO(string_format("M%d_ECON_RE_Sb", imodule_), true);
}

const std::vector<std::pair<int, int>> EcalModule::ROC_ERX_MAPPING_DAQ = {
    {9, 10}, {5, 6}, {0, 1}, {11, 8}, {7, 4}, {3, 2}};

/**
 * TRIG eLinks 1 and 3 are ignored since they are all-zero
 * when summing by 9 channels in the HGCROC.
 * Since each half of the HGCROC produces a pair of TRIG eLinks,
 * but we only need one of the two, we re-index and have the
 * TRIG eLinks indexed by HGCROC half like is done with the DAQ eLinks.
 *
 *
 * This reference table was manually copied from the hardware schematic.
 *
 * Sector | Side | Elink | ROC | TRIG | DIN
 * -------|------|-------|-----|------|----
 *  2     | L    | 2     | 2   | 2    | 0
 *  2     | R    | 0     | 3   | 0    | 1
 *  2     | L    | 0     | 2   | 0    | 2
 *  2     | R    | 2     | 3   | 2    | 3
 *  1     | L    | 2     | 0   | 2    | 4
 *  1     | R    | 0     | 1   | 0    | 5
 *  1     | L    | 0     | 0   | 0    | 6
 *  1     | R    | 2     | 1   | 2    | 7
 *  3     | R    | 0     | 5   | 0    | 8
 *  3     | L    | 2     | 4   | 2    | 9
 *  3     | R    | 2     | 5   | 2    | 10
 *  3     | L    | 0     | 4   | 0    | 11
 */
const std::vector<std::pair<int, std::vector<int>>>
    EcalModule::ROC_ERX_MAPPING_TRG = {
        {1, {6, 4}}, {1, {5, 7}},  {1, {2, 0}},
        {1, {1, 3}}, {1, {11, 9}}, {1, {8, 10}},
};

const std::vector<std::pair<int, int>>&
EcalModule::getHardwareRocErxMappingDAQ() {
  return ROC_ERX_MAPPING_DAQ;
}

const std::vector<std::pair<int, std::vector<int>>>&
EcalModule::getHardwareRocErxMappingTRG() {
  return ROC_ERX_MAPPING_TRG;
}

}  // namespace pflib
