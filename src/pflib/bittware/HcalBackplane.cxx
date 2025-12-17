#include "pflib/HcalBackplane.h"
#include "pflib/bittware/bittware_FastControl.h"
#include "pflib/bittware/bittware_daq.h"
#include "pflib/bittware/bittware_elinks.h"
#include "pflib/bittware/bittware_optolink.h"
#include "pflib/lpgbt/I2C.h"
#include "pflib/lpgbt/lpGBT_standard_configs.h"
#include "pflib/utility/string_format.h"

namespace pflib {

static constexpr int ADDR_HCAL_BACKPLANE_DAQ = 0x78 | 0x04;
static constexpr int ADDR_HCAL_BACKPLANE_TRIG = 0x78;

class HcalBackplaneBW : public HcalBackplane {
  mutable logging::logger the_log_{logging::get("HcalBackplaneBW")};

 public:
  HcalBackplaneBW(int itarget, uint8_t board_mask, const char* dev) {
    auto daq_olink =
        std::make_shared<pflib::bittware::BWOptoLink>(itarget, dev);
    opto_["DAQ"] = daq_olink;
    opto_["TRG"] =
        std::make_shared<pflib::bittware::BWOptoLink>(itarget + 1, *daq_olink);

    // then get the lpGBTs from them
    daq_lpgbt_ =
        std::make_unique<pflib::lpGBT>(opto_["DAQ"]->lpgbt_transport());
    trig_lpgbt_ =
        std::make_unique<pflib::lpGBT>(opto_["TRG"]->lpgbt_transport());

    this->init(*daq_lpgbt_, *trig_lpgbt_, board_mask);

    elinks_ = std::make_unique<bittware::OptoElinksBW>(itarget, dev);

    daq_ = std::make_unique<bittware::HcalBackplaneBW_Capture>(dev);

    fc_ = std::make_shared<bittware::BWFastControl>(dev);
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
    std::vector<uint32_t> buf;

    if (format_ == Target::DaqFormat::ECOND_SW_HEADERS) {
      for (int ievt = 0; ievt < daq().samples_per_ror(); ievt++) {
        // only one elink right now
        std::vector<uint32_t> subpacket = daq().getLinkData(0);
        buf.push_back((0x1 << 28) | ((daq().econid() & 0x3ff) << 18) |
                      (ievt << 13) | ((ievt == daq().soi()) ? (1 << 12) : (0)) |
                      (subpacket.size()));
        buf.insert(buf.end(), subpacket.begin(), subpacket.end());
        daq().advanceLinkReadPtr();
      }
      // FW puts in one last "header" that signals no more packets
      buf.push_back((0x1 << 28) | (0x3ff << 18) | (0x1f << 13));
    } else {
      PFEXCEPTION_RAISE("NoImpl",
                        "HcalBackplaneBWTarget::read_event not implemented "
                        "for provided DaqFormat");
    }

    return buf;
  }

 private:
  std::unique_ptr<pflib::lpGBT> daq_lpgbt_, trig_lpgbt_;
  std::unique_ptr<pflib::bittware::OptoElinksBW> elinks_;
  std::unique_ptr<bittware::HcalBackplaneBW_Capture> daq_;
  std::shared_ptr<pflib::bittware::BWFastControl> fc_;
  Target::DaqFormat format_;
  int contrib_id_;
};

Target* makeTargetHcalBackplaneBittware(int ilink, uint8_t board_mask,
                                        const char* dev) {
  return new HcalBackplaneBW(ilink, board_mask, dev);
}

}  // namespace pflib
