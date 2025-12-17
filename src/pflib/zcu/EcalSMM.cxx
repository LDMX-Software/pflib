#include "pflib/Ecal.h"
#include "pflib/Target.h"
#include "pflib/lpgbt/lpGBT_standard_configs.h"
#include "pflib/utility/string_format.h"
#include "pflib/zcu/zcu_daq.h"
#include "pflib/zcu/zcu_elinks.h"
#include "pflib/zcu/zcu_optolink.h"

namespace pflib {

static constexpr int I2C_BUS_M0 = 1;

class EcalSMMTargetZCU : public Target {
 public:
  EcalSMMTargetZCU(int itarget, uint8_t roc_mask) {
    using namespace pflib::zcu;
    // first, setup the optical links
    std::string uio_coder =
        pflib::utility::string_format("standardLpGBTpair-%d", itarget);

    opto_["DAQ"] = std::make_shared<ZCUOptoLink>(uio_coder);
    opto_["TRG"] = std::make_shared<ZCUOptoLink>(uio_coder, 1, false);

    daq_lpgbt_ =
        std::make_unique<pflib::lpGBT>(opto_["DAQ"]->lpgbt_transport());
    trig_lpgbt_ =
        std::make_unique<pflib::lpGBT>(opto_["TRG"]->lpgbt_transport());

    ecalModule_ = std::make_shared<pflib::EcalModule>(*daq_lpgbt_, I2C_BUS_M0,
                                                      0, roc_mask);

    elinks_ = std::make_unique<OptoElinksZCU>(&(*daq_lpgbt_), &(*trig_lpgbt_),
                                              itarget);
    daq_ = std::make_unique<ZCU_Capture>();

    using namespace pflib::lpgbt::standard_config;

    setup_ecal(*daq_lpgbt_, ECAL_lpGBT_Config::DAQ_SingleModuleMotherboard);

    try {
      setup_ecal(*trig_lpgbt_, ECAL_lpGBT_Config::TRIG_SingleModuleMotherboard);
    } catch (std::exception& e) {
      printf("Problem (non critical) setting up TRIGGER lpgbt\n");
    }

    fc_ = std::shared_ptr<FastControl>(make_FastControlCMS_MMap());
  }

  const std::vector<std::pair<int, int>>& getRocErxMapping() override;

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

    daq().reset();
    fc().clear_run();
  }

  virtual std::vector<uint32_t> read_event() override {
    if (format_ == Target::DaqFormat::ECOND_SW_HEADERS) {
      return daq_->read_event_sw_headers();
    } else {
      PFEXCEPTION_RAISE("NoImpl",
                        "EcalSMMZCU::read_event not implemented "
                        "for provided DaqFormat");
    }
    return {};
  }

 private:
  std::shared_ptr<EcalModule> ecalModule_;
  std::unique_ptr<lpGBT> daq_lpgbt_, trig_lpgbt_;
  std::unique_ptr<pflib::zcu::OptoElinksZCU> elinks_;
  std::unique_ptr<pflib::zcu::ZCU_Capture> daq_;
  std::shared_ptr<pflib::FastControl> fc_;
  Target::DaqFormat format_;
  int contrib_id_;
};

const std::vector<std::pair<int, int>>& EcalSMMTargetZCU::getRocErxMapping() {
  return EcalModule::getRocErxMapping();
}

Target* makeTargetEcalSMMZCU(int ilink, uint8_t roc_mask) {
  return new EcalSMMTargetZCU(ilink, roc_mask);
}

}  // namespace pflib
