#include "pflib/EcalSingleModuleMotherboard.h"
#include "pflib/lpgbt/lpGBT_standard_configs.h"
#include "pflib/utility/string_format.h"
#include "pflib/zcu/zcu_daq.h"
#include "pflib/zcu/zcu_elinks.h"
#include "pflib/zcu/zcu_optolink.h"

namespace pflib {

static constexpr int I2C_BUS_M0 = 1;

class EcalSMMTargetZCU : public EcalSingleModuleMotherboard {
 public:
  EcalSMMTargetZCU(int itarget, uint8_t roc_mask) {
    using namespace pflib::zcu;
    // first, setup the optical links
    std::string uio_coder =
        pflib::utility::string_format("standardLpGBTpair-%d", itarget);

    opto_["DAQ"] =
        std::make_shared<ZCUOptoLink>(uio_coder, 2 * itarget + 0, true);
    opto_["TRG"] =
        std::make_shared<ZCUOptoLink>(uio_coder, 2 * itarget + 1, false);

    daq_lpgbt_ =
        std::make_unique<pflib::lpGBT>(opto_["DAQ"]->lpgbt_transport());
    trig_lpgbt_ =
        std::make_unique<pflib::lpGBT>(opto_["TRG"]->lpgbt_transport());

    init(*daq_lpgbt_, *trig_lpgbt_, I2C_BUS_M0, roc_mask);

    elinks_ = std::make_unique<OptoElinksZCU>(daq_lpgbt_.get(),
                                              trig_lpgbt_.get(), itarget);
    daq_ = std::make_unique<ZCU_Capture>(itarget);

    fc_ = std::shared_ptr<FastControl>(make_FastControlCMS_MMap());

    /// try to make a trig object, but ok to fail
    try {
      trig_ = std::make_unique<ZCUtrig>();
      trig_->set_l1a_per_ror(daq().samples_per_ror());
      pflib_log(warn)
          << "created trig object for EcalSMM but has gone untested!";
    } catch (pflib::Exception& e) {
      pflib_log(info) << "failed to create TRIG connection with " << e.what();
      pflib_log(info)
          << "(only necessary if you are trying to capture the trigger path)";
    }
  }

  virtual Elinks& elinks() override { return *elinks_; }

  virtual DAQ& daq() override { return *daq_; }

  virtual FastControl& fc() override { return *fc_; }

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
  std::unique_ptr<lpGBT> daq_lpgbt_, trig_lpgbt_;
  std::unique_ptr<pflib::zcu::OptoElinksZCU> elinks_;
  std::unique_ptr<pflib::zcu::ZCU_Capture> daq_;
  std::shared_ptr<pflib::FastControl> fc_;
};

Target* makeTargetEcalSMMZCU(int ilink, uint8_t roc_mask) {
  return new EcalSMMTargetZCU(ilink, roc_mask);
}

}  // namespace pflib
