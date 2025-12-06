#include "pflib/Ecal.h"
#include "pflib/Target.h"
#include "pflib/bittware/bittware_FastControl.h"
#include "pflib/bittware/bittware_daq.h"
#include "pflib/bittware/bittware_elinks.h"
#include "pflib/bittware/bittware_optolink.h"
#include "pflib/lpgbt/I2C.h"
#include "pflib/lpgbt/lpGBT_standard_configs.h"
#include "pflib/utility/string_format.h"

namespace pflib {

static constexpr int ADDR_ECAL_SMM_DAQ = 0x78 | 0x04;
static constexpr int ADDR_ECAL_SMM_TRIG = 0x78;
static constexpr int I2C_BUS_M0 = 1;

class EcalSMMTargetBW : public Target {
  mutable logging::logger the_log_{logging::get("EcalSMMBW")};

 public:
  EcalSMMTargetBW(int itarget, const char* dev) {
    using namespace pflib::bittware;
    // first, setup the optical links
    auto daq_olink = std::make_shared<BWOptoLink>(itarget, dev);
    opto_["DAQ"] = daq_olink;
    opto_["TRG"] = std::make_shared<BWOptoLink>(itarget + 1, *daq_olink);

    // then get the lpGBTs
    daq_lpgbt_ =
        std::make_unique<pflib::lpGBT>(opto_["DAQ"]->lpgbt_transport());
    trig_lpgbt_ =
        std::make_unique<pflib::lpGBT>(opto_["TRG"]->lpgbt_transport());

    ecalModule_ =
        std::make_shared<pflib::EcalModule>(*daq_lpgbt_, I2C_BUS_M0, 0);

    elinks_ = std::make_unique<OptoElinksBW>(itarget, dev);
    daq_ = std::make_unique<bittware::HcalBackplaneBW_Capture>(dev);

    using namespace pflib::lpgbt::standard_config;

    try {
      int daq_pusm = daq_lpgbt_->status();

      if (daq_pusm == 19) {
        pflib_log(debug) << "DAQ lpGBT is PUSM READY (19)";
      } else {
        pflib_log(debug)
            << "DAQ lpGBT is not ready, attempting standard config";
        try {
          setup_ecal(*daq_lpgbt_,
                     ECAL_lpGBT_Config::DAQ_SingleModuleMotherboard);
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

    try {
      int trg_pusm = trig_lpgbt_->status();
      if (trg_pusm == 19) {
        pflib_log(debug) << "TRG lpGBT is PUSM READY (19)";
      } else {
        pflib_log(debug)
            << "TRG lpGBT is not ready, attempting standard config";
        try {
          setup_ecal(*trig_lpgbt_,
                     ECAL_lpGBT_Config::TRIG_SingleModuleMotherboard);
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

    fc_ = std::make_shared<bittware::BWFastControl>(dev);
  }

  virtual int nrocs() { return ecalModule_->nrocs(); }
  virtual int necons() { return ecalModule_->necons(); }
  virtual bool have_roc(int iroc) const { return ecalModule_->have_roc(iroc); }
  virtual bool have_econ(int iecon) const {
    return ecalModule_->have_econ(iecon);
  }
  virtual std::vector<int> roc_ids() const { return ecalModule_->roc_ids(); }
  virtual std::vector<int> econ_ids() const { return ecalModule_->econ_ids(); }

  virtual ROC& roc(int which) { return ecalModule_->roc(which); }
  virtual ECON& econ(int which) { return ecalModule_->econ(which); }

  virtual void softResetROC(int which) override { ecalModule_->softResetROC(); }

  virtual void softResetECON(int which = -1) override {
    ecalModule_->softResetECON();
  }

  virtual void hardResetROCs() override { ecalModule_->hardResetROCs(); }

  virtual void hardResetECONs() override { ecalModule_->hardResetECONs(); }

  virtual Elinks& elinks() override { return *elinks_; }

  virtual DAQ& daq() override { return *daq_; }

  virtual FastControl& fc() override { return *fc_; }

  virtual void setup_run(int irun, Target::DaqFormat format, int contrib_id) {
    format_ = format;
    contrib_id_ = contrib_id;
  }

  virtual std::vector<uint32_t> read_event() override {
    PFEXCEPTION_RAISE("NoImpl", "EcalSMMTargetBW::read_event not implemented.");
    return {};
  }

 private:
  std::shared_ptr<EcalModule> ecalModule_;
  std::unique_ptr<lpGBT> daq_lpgbt_, trig_lpgbt_;
  std::unique_ptr<pflib::bittware::OptoElinksBW> elinks_;
  std::unique_ptr<bittware::HcalBackplaneBW_Capture> daq_;
  std::shared_ptr<pflib::bittware::BWFastControl> fc_;
  Target::DaqFormat format_;
  int contrib_id_;
};

Target* makeTargetEcalSMMBittware(int ilink, const char* dev) {
  return new EcalSMMTargetBW(ilink, dev);
}

}  // namespace pflib
