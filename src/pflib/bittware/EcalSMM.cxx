#include "pflib/EcalSingleModuleMotherboard.h"
#include "pflib/bittware/bittware_FastControl.h"
#include "pflib/bittware/bittware_daq.h"
#include "pflib/bittware/bittware_elinks.h"
#include "pflib/bittware/bittware_optolink.h"
#include "pflib/utility/string_format.h"

namespace pflib {

static constexpr int ADDR_ECAL_SMM_DAQ = 0x78 | 0x04;
static constexpr int ADDR_ECAL_SMM_TRIG = 0x78;
static constexpr int I2C_BUS_M0 = 1;

class EcalSMMTargetBW : public EcalSingleModuleMotherboard {
  mutable logging::logger the_log_{logging::get("EcalSMMBW")};

 public:
  EcalSMMTargetBW(int itarget, uint8_t roc_mask, const char* dev) {
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

    init(*daq_lpgbt_, *trig_lpgbt_, I2C_BUS_M0, roc_mask);

    elinks_ = std::make_unique<OptoElinksBW>(itarget, dev);
    daq_ = std::make_unique<bittware::HcalBackplaneBW_Capture>(dev);
    fc_ = std::make_shared<bittware::BWFastControl>(dev);
  }

  virtual Elinks& elinks() override { return *elinks_; }

  virtual DAQ& daq() override { return *daq_; }

  virtual FastControl& fc() override { return *fc_; }

  virtual std::vector<uint32_t> read_event() override {
    if (format_ == Target::DaqFormat::ECOND_SW_HEADERS) {
      return daq().read_event_sw_headers();
    } else {
      PFEXCEPTION_RAISE("NoImpl",
                        "EcalSMMTargetBW::read_event not implemented "
                        "for provided DaqFormat");
    }
    return {};
  }

 private:
  std::unique_ptr<lpGBT> daq_lpgbt_, trig_lpgbt_;
  std::unique_ptr<pflib::bittware::OptoElinksBW> elinks_;
  std::unique_ptr<bittware::HcalBackplaneBW_Capture> daq_;
  std::shared_ptr<pflib::bittware::BWFastControl> fc_;
};

Target* makeTargetEcalSMMBittware(int ilink, uint8_t roc_mask,
                                  const char* dev) {
  return new EcalSMMTargetBW(ilink, roc_mask, dev);
}

}  // namespace pflib
