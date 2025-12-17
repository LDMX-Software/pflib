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
  econs_.emplace(std::piecewise_construct,
      std::forward_as_tuple(0),
      std::forward_as_tuple(econ_i2c, (0x60 | 0), "econd")
  );
  econs_.emplace(std::piecewise_construct,
      std::forward_as_tuple(1),
      std::forward_as_tuple(econ_i2c, (0x20 | 0), "econt")
  );
  econs_.emplace(std::piecewise_construct,
      std::forward_as_tuple(2),
      std::forward_as_tuple(econ_i2c, (0x20 | 1), "econt")
  );

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
    rocs_.emplace(std::piecewise_construct,
        std::forward_as_tuple(ibd),
        std::forward_as_tuple(roc_i2c, (0x20 | (ibd * 8)), "sipm_rocv3b")
    );
    biases_.emplace(std::piecewise_construct,
        std::forward_as_tuple(ibd),
        std::forward_as_tuple(bias_i2c, board_i2c)
    );
    i2c_[pflib::utility::string_format("HGCROC_%d", ibd)] = roc_i2c;
    i2c_[pflib::utility::string_format("BOARD_%d", ibd)] = board_i2c;
    i2c_[pflib::utility::string_format("BIAS_%d", ibd)] = bias_i2c;
  }
}


bool HcalBackplane::have_roc(int iroc) const {
  return rocs_.find(iroc) != rocs_.end();
}

bool HcalBackplane::have_econ(int iecon) const {
  return econs_.find(iecon) != econs_.end();
}

const std::vector<std::pair<int, int>>& HcalBackplane::getRocErxMapping() {
  return roc_to_erx_map_;
}

std::vector<int> HcalBackplane::roc_ids() const {
  std::vector<int> ids;
  ids.reserve(rocs_.size());
  for (const auto& [id, _conn] : rocs_) {
    ids.push_back(id);
  }
  return ids;
}

std::vector<int> HcalBackplane::econ_ids() const {
  std::vector<int> ids;
  ids.reserve(econs_.size());
  for (const auto& [id, _conn] : econs_) {
    ids.push_back(id);
  }
  return ids;
}

ROC& HcalBackplane::roc(int which) {
  auto roc_it{rocs_.find(which)};
  if (roc_it == rocs_.end()) {
    PFEXCEPTION_RAISE("InvalidROCid", pflib::utility::string_format(
                                          "Unknown ROC id %d", which));
  }
  return roc_it->second;
}

ECON& HcalBackplane::econ(int which) {
  auto econ_it{econs_.find(which)};
  if (econ_it == econs_.end()) {
    PFEXCEPTION_RAISE("InvalidECONid", pflib::utility::string_format(
                                           "Unknown ECON id %d", which));
  }
  return econ_it->second;
}

Bias HcalBackplane::bias(int which) {
  auto bias_it{biases_.find(which)};
  if (bias_it == biases_.end()) {
    PFEXCEPTION_RAISE("InvalidBoardId", pflib::utility::string_format(
                                            "Unknown board id %d", which));
  }

  return bias_it->second;
}

}  // namespace pflib
