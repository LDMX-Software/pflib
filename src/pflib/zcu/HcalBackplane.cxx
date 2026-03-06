#include "pflib/HcalBackplane.h"

#include "pflib/lpgbt/I2C.h"
#include "pflib/lpgbt/lpGBT_standard_configs.h"
#include "pflib/utility/string_format.h"
#include "pflib/zcu/lpGBT_ICEC_ZCU_Simple.h"
#include "pflib/zcu/zcu_daq.h"
#include "pflib/zcu/zcu_elinks.h"
#include "pflib/zcu/zcu_optolink.h"

namespace pflib {

class HcalBackplaneZCU : public HcalBackplane {
 public:
  HcalBackplaneZCU(int itarget, uint8_t board_mask) {
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

    this->init(*daq_lpgbt_, *trig_lpgbt_, board_mask);

    elinks_ = std::make_unique<OptoElinksZCU>(&(*daq_lpgbt_), &(*trig_lpgbt_),
                                              itarget);
    daq_ = std::make_unique<ZCU_Capture>();

    fc_ = std::shared_ptr<FastControl>(make_FastControlCMS_MMap());
  }

  virtual void softResetROC(int which) override {
    // assuming everything was done with the standard config
    if (which < 0 || which == 0) {
      trig_lpgbt_->gpio_interface().setGPO("HGCROC0_SRST", false);
      trig_lpgbt_->gpio_interface().setGPO("HGCROC0_SRST", true);
    }
    if (which < 0 || which == 1) {
      daq_lpgbt_->gpio_interface().setGPO("HGCROC1_SRST", false);
      daq_lpgbt_->gpio_interface().setGPO("HGCROC1_SRST", true);
    }
    if (which < 0 || which == 2) {
      daq_lpgbt_->gpio_interface().setGPO("HGCROC2_SRST", false);
      daq_lpgbt_->gpio_interface().setGPO("HGCROC2_SRST", true);
    }
    if (which < 0 || which == 3) {
      trig_lpgbt_->gpio_interface().setGPO("HGCROC3_SRST", false);
      trig_lpgbt_->gpio_interface().setGPO("HGCROC3_SRST", true);
    }
  }

  virtual void softResetECON(int which = -1) override {
    trig_lpgbt_->gpio_interface().setGPO("ECON_SRST", false);
    trig_lpgbt_->gpio_interface().setGPO("ECON_SRST", true);
  }

  virtual void hardResetROCs() override {
    trig_lpgbt_->gpio_interface().setGPO("HGCROC0_HRST", false);
    trig_lpgbt_->gpio_interface().setGPO("HGCROC0_HRST", true);
    daq_lpgbt_->gpio_interface().setGPO("HGCROC1_HRST", false);
    daq_lpgbt_->gpio_interface().setGPO("HGCROC1_HRST", true);
    daq_lpgbt_->gpio_interface().setGPO("HGCROC2_HRST", false);
    daq_lpgbt_->gpio_interface().setGPO("HGCROC2_HRST", true);
    trig_lpgbt_->gpio_interface().setGPO("HGCROC3_HRST", false);
    trig_lpgbt_->gpio_interface().setGPO("HGCROC3_HRST", true);
  }

  virtual void hardResetECONs() override {
    trig_lpgbt_->gpio_interface().setGPO("ECON_HRST", false);
    trig_lpgbt_->gpio_interface().setGPO("ECON_HRST", true);
  }

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
                        "HcalBackplaneZCUTarget::read_event not implemented "
                        "for provided DaqFormat");
    }
    return {};
  }

 private:
  std::unique_ptr<lpGBT> daq_lpgbt_, trig_lpgbt_;
  std::unique_ptr<pflib::zcu::OptoElinksZCU> elinks_;
  std::unique_ptr<pflib::zcu::ZCU_Capture> daq_;
  std::shared_ptr<pflib::FastControl> fc_;
  Target::DaqFormat format_;
  int contrib_id_;
};

Target* makeTargetHcalBackplaneZCU(int ilink, uint8_t board_mask) {
  return new HcalBackplaneZCU(ilink, board_mask);
}

}  // namespace pflib
