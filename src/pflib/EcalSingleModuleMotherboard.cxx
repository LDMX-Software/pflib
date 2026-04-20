#include "pflib/EcalSingleModuleMotherboard.h"

#include "pflib/lpgbt/lpGBT_standard_configs.h"

namespace pflib {

void EcalSingleModuleMotherboard::init(lpGBT& daq_lpgbt, lpGBT& trg_lpgbt,
                                       int module_i2c_bus, int roc_mask) {
  // Setup DAQ lpGBT
  try {
    int daq_pusm = daq_lpgbt.status();
    pflib::lpgbt::standard_config::setup_ecal_daq_gpio(daq_lpgbt);

    if (daq_pusm == 19) {
      pflib_log(debug) << "DAQ lpGBT is PUSM READY (19)";
    } else {
      pflib_log(debug) << "DAQ lpGBT is not ready, attempting standard config";
      try {
        pflib::lpgbt::standard_config::setup_ecal(
            daq_lpgbt, pflib::lpgbt::standard_config::ECAL_lpGBT_Config::
                           DAQ_SingleModuleMotherboard);
      } catch (const pflib::Exception& e) {
        pflib_log(warn) << "Failure to apply standard config [" << e.name()
                        << "]: " << e.message();
      }
    }
  } catch (const pflib::Exception& e) {
    pflib_log(debug) << "unable to I2C transact with lpGBT, advising user to "
                        "check Optical links";
    pflib_log(warn) << "Failure to check DAQ lpGBT status [" << e.name()
                    << "]: " << e.message();
    pflib_log(warn) << "Go into OPTO and make sure the link is READY"
                    << " and then re-open pftool.";
  }

  // Setup TRG lpGBT
  try {
    int trg_pusm = trg_lpgbt.status();
    if (trg_pusm == 19) {
      pflib_log(debug) << "TRG lpGBT is PUSM READY (19)";
    } else {
      pflib_log(debug) << "TRG lpGBT is not ready, attempting standard config";
      try {
        // was the DAQ_SMM setup config on the ZCU
        pflib::lpgbt::standard_config::setup_ecal(
            trg_lpgbt, pflib::lpgbt::standard_config::ECAL_lpGBT_Config::
                           TRIG_SingleModuleMotherboard);
      } catch (const pflib::Exception& e) {
        pflib_log(info) << "Not Critical Problem setting up TRIGGER lpGBT.";
        pflib_log(info) << "Failure to apply standard config [" << e.name()
                        << "]: " << e.message();
      }
    }
  } catch (const pflib::Exception& e) {
    pflib_log(info) << "(Not Critical) Failure to check TRG lpGBT status ["
                    << e.name() << "]: " << e.message();
  }

  the_module_ =
      std::make_shared<EcalModule>(daq_lpgbt, module_i2c_bus, 0, roc_mask);
}

const std::vector<std::pair<int, int>>&
EcalSingleModuleMotherboard::getRocErxMapping() {
  return EcalModule::getRocErxMapping();
}

int EcalSingleModuleMotherboard::nrocs() { return the_module_->nrocs(); }

int EcalSingleModuleMotherboard::necons() { return the_module_->necons(); }

bool EcalSingleModuleMotherboard::have_roc(int iroc) const {
  return the_module_->have_roc(iroc);
}

bool EcalSingleModuleMotherboard::have_econ(int iecon) const {
  return the_module_->have_econ(iecon);
}

std::vector<int> EcalSingleModuleMotherboard::roc_ids() const {
  return the_module_->roc_ids();
}

std::vector<int> EcalSingleModuleMotherboard::econ_ids() const {
  return the_module_->econ_ids();
}

ROC& EcalSingleModuleMotherboard::roc(int which) {
  return the_module_->roc(which);
}

ECON& EcalSingleModuleMotherboard::econ(int which) {
  return the_module_->econ(which);
}

void EcalSingleModuleMotherboard::softResetROC(int which) {
  /// the soft reset is applied to all ROCs on the board
  return the_module_->softResetROC();
}

void EcalSingleModuleMotherboard::softResetECON(int which) {
  /// the soft reset is applied to all ECONs on the board
  return the_module_->softResetECON();
}

void EcalSingleModuleMotherboard::hardResetROCs() {
  return the_module_->hardResetROCs();
}

void EcalSingleModuleMotherboard::hardResetECONs() {
  return the_module_->hardResetECONs();
}

void EcalSingleModuleMotherboard::setup_run(int irun, Target::DaqFormat format,
                                            int contrib_id) {
  format_ = format;
  contrib_id_ = contrib_id;

  // this reset and clear_run were not present in the EcalSMMBittware
  daq().reset();
  fc().clear_run();
}

}  // namespace pflib
