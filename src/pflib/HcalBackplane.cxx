#include "pflib/HcalBackplane.h"

#include "pflib/lpgbt/lpGBT_standard_configs.h"
#include "pflib/lpgbt/I2C.h"

#include "pflib/utility/string_format.h"

namespace pflib {

static constexpr int I2C_BUS_ECONS = 0;    // DAQ
static constexpr int I2C_BUS_HGCROCS = 1;  // DAQ
static constexpr int I2C_BUS_BIAS = 1;     // TRIG
static constexpr int I2C_BUS_BOARD = 0;    // TRIG
static constexpr int ADDR_MUX_BIAS = 0x70;
static constexpr int ADDR_MUX_BOARD = 0x71;

HcalBackplane::HcalBackplane() {
  nhgcroc_ = 0;
  necon_ = 0;

  // Default HCAL ROC→ECON mapping
  roc_to_erx_map_ = {{3, 2}, {6, 7}, {4, 5}, {1, 0}};
}

void HcalBackplane::init(lpGBT& daq_lpgbt, lpGBT& trig_lpgbt, int hgcroc_boardmask) {
  // Load GPIO configuration for lpGBTs
  pflib::lpgbt::standard_config::setup_hcal_daq_gpio(daq_lpgbt);
  pflib::lpgbt::standard_config::setup_hcal_trig_gpio(trig_lpgbt);

  // Setup lpGBTs
  try {
    int daq_pusm = daq_lpgbt.status();
    int trg_pusm = trig_lpgbt.status();
    if (daq_pusm == 19 and trg_pusm == 19) {
      // both lpGBTs are PUSM READY
      pflib_log(debug) << "both lpGBTs are have status PUSM READY (19)";
    } else if (daq_pusm != 19 and trg_pusm == 19) {
      pflib_log(debug) << "TRG lpGBT has status PUSM READY (19)";
      pflib_log(debug) << "applying standard DAQ lpGBT configuration";
      try {
        pflib::lpgbt::standard_config::setup_hcal_daq(daq_lpgbt);
      } catch (const pflib::Exception& e) {
        pflib_log(warn) << "Failure to apply standard config [" << e.name()
                        << "]: " << e.message();
      }
    } else if (daq_pusm == 19 and trg_pusm != 19) {
      pflib_log(debug) << "DAQ lpGBT has status PUSM READY (19)";
      pflib_log(debug) << "applying standard TRG lpGBT configuration";
      try {
        pflib::lpgbt::standard_config::setup_hcal_trig(trig_lpgbt);
      } catch (const pflib::Exception& e) {
        pflib_log(warn) << "Failure to apply standard config [" << e.name()
                        << "]: " << e.message();
      }
    } else /* both are not PUSM READY */ {
      pflib_log(debug) << "neither lpGBT have status PUSM READY (19)";
      pflib_log(debug) << "applying standard lpGBT configuration";
      try {
        pflib_log(debug) << "applying DAQ";
        pflib::lpgbt::standard_config::setup_hcal_daq(daq_lpgbt);
        pflib_log(debug) << "pause to let hardware re-sync";
        sleep(2);
        pflib_log(debug) << "applying TRIG";
        pflib::lpgbt::standard_config::setup_hcal_trig(trig_lpgbt);
      } catch (const pflib::Exception& e) {
        pflib_log(warn) << "Failure to apply standard config [" << e.name()
                        << "]: " << e.message();
      }
    }
  } catch (const pflib::Exception& e) {
    pflib_log(debug) << "unable to I2C transact with lpGBT, advising user to "
                        "check Optical links";
    pflib_log(warn) << "Failure to check lpGBT status [" << e.name()
                    << "]: " << e.message();
    pflib_log(warn) << "Go into OPTO and make sure the link is READY"
                    << " and then re-open pftool.";
  }

  // next, create the Hcal I2C objects
  auto econ_i2c = std::make_shared<pflib::lpgbt::I2C>(daq_lpgbt, I2C_BUS_ECONS);
  econ_i2c->set_bus_speed(1000);
  i2c_["ECON"] = econ_i2c;
  econs_[0] = std::make_unique<ECON>(econ_i2c, (0x60 | 0), "econd");
  econs_[1] = std::make_unique<ECON>(econ_i2c, (0x20 | 0), "econt");
  econs_[2] = std::make_unique<ECON>(econ_i2c, (0x20 | 1), "econt");

  auto roc_i2c
    = std::make_shared<pflib::lpgbt::I2C>(daq_lpgbt, I2C_BUS_HGCROCS);
  roc_i2c->set_bus_speed(1000);
  for (int ibd = 0; ibd < 4; ibd++) {
    if ((hgcroc_boardmask & (1 << ibd)) == 0) continue;
    std::shared_ptr<pflib::I2C> bias_i2c =
        std::make_shared<pflib::lpgbt::I2CwithMux>(trig_lpgbt, I2C_BUS_BIAS,
                                                   ADDR_MUX_BIAS, (1 << ibd));
    std::shared_ptr<pflib::I2C> board_i2c =
        std::make_shared<pflib::lpgbt::I2CwithMux>(
            trig_lpgbt, I2C_BUS_BOARD, ADDR_MUX_BOARD, (1 << ibd));

    nhgcroc_++;
    rocs_[ibd] = std::make_unique<HGCROCBoard>(
        ROC(roc_i2c, (0x20 | (ibd * 8)), "sipm_rocv3b"),
        Bias(bias_i2c, board_i2c)
    );
    i2c_[pflib::utility::string_format("HGCROC_%d", ibd)] = roc_i2c;
    i2c_[pflib::utility::string_format("BOARD_%d", ibd)] = board_i2c;
    i2c_[pflib::utility::string_format("BIAS_%d", ibd)] = bias_i2c;
  }
}


bool HcalBackplane::have_roc(int iroc) const {
  if (iroc < 0 or iroc >= rocs_.size()) {
    return false;
  }
  return bool(rocs_[iroc]);
}

bool HcalBackplane::have_econ(int iecon) const {
  if (iecon < 0 or iecon >= econs_.size()) {
    return false;
  }
  return bool(econs_[iecon]);
}

const std::vector<std::pair<int, int>>& HcalBackplane::getRocErxMapping() {
  return roc_to_erx_map_;
}

std::vector<int> HcalBackplane::roc_ids() const {
  std::vector<int> ids;
  ids.reserve(rocs_.size());
  for (int i{0}; i < rocs_.size(); i++) {
    if (rocs_[i]) {
      ids.push_back(i);
    }
  }
  return ids;
}

std::vector<int> HcalBackplane::econ_ids() const {
  std::vector<int> ids;
  ids.reserve(econs_.size());
  for (int i{0}; i < econs_.size(); i++) {
    if (econs_[i]) {
      ids.push_back(i);
    }
  }
  return ids;
}

ROC& HcalBackplane::roc(int which) {
  if (which < 0 or which >= rocs_.size()) {
    PFEXCEPTION_RAISE("InvalidROCid", pflib::utility::string_format(
                                          "ROC %d is not a valid ROC ID"));
  }
  if (not rocs_[which]) {
    PFEXCEPTION_RAISE("DisabledROC", pflib::utility::string_format(
                                          "ROC %d is disabled"));
  }
  return rocs_[which]->roc;
}

ECON& HcalBackplane::econ(int which) {
  if (which < 0 or which >= econs_.size()) {
    PFEXCEPTION_RAISE("InvalidROCid", pflib::utility::string_format(
                                          "ROC %d is not a valid ROC ID"));
  }
  if (not econs_[which]) {
    PFEXCEPTION_RAISE("DisabledECON", pflib::utility::string_format(
                                          "ECON %d is disabled"));
    
  }
  return *(econs_[which]);
}

Bias HcalBackplane::bias(int which) {
  if (which < 0 or which >= rocs_.size()) {
    PFEXCEPTION_RAISE("InvalidROCid", pflib::utility::string_format(
                                          "ROC %d is not a valid ROC ID"));
  }
  if (not rocs_[which]) {
    PFEXCEPTION_RAISE("DisabledROC", pflib::utility::string_format(
                                          "ROC %d is disabled"));
  }
  return rocs_[which]->bias;
}

}  // namespace pflib
