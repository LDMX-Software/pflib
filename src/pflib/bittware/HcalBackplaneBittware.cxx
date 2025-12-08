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
static constexpr int I2C_BUS_ECONS = 0;    // DAQ
static constexpr int I2C_BUS_HGCROCS = 1;  // DAQ
static constexpr int I2C_BUS_BIAS = 1;     // TRIG
static constexpr int I2C_BUS_BOARD = 0;    // TRIG
static constexpr int ADDR_MUX_BIAS = 0x70;
static constexpr int ADDR_MUX_BOARD = 0x71;

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

    try {
      int daq_pusm = daq_lpgbt_->status();
      int trg_pusm = trig_lpgbt_->status();
      if (daq_pusm == 19 and trg_pusm == 19) {
        // both lpGBTs are PUSM READY
        pflib_log(debug) << "both lpGBTs are have status PUSM READY (19)";
      } else if (daq_pusm != 19 and trg_pusm == 19) {
        pflib_log(debug) << "TRG lpGBT has status PUSM READY (19)";
        pflib_log(debug) << "applying standard DAQ lpGBT configuration";
        try {
          pflib::lpgbt::standard_config::setup_hcal_daq(*daq_lpgbt_);
        } catch (const pflib::Exception& e) {
          pflib_log(warn) << "Failure to apply standard config [" << e.name()
                          << "]: " << e.message();
        }
      } else if (daq_pusm == 19 and trg_pusm != 19) {
        pflib_log(debug) << "DAQ lpGBT has status PUSM READY (19)";
        pflib_log(debug) << "applying standard TRG lpGBT configuration";
        try {
          pflib::lpgbt::standard_config::setup_hcal_trig(*trig_lpgbt_);
        } catch (const pflib::Exception& e) {
          pflib_log(warn) << "Failure to apply standard config [" << e.name()
                          << "]: " << e.message();
        }
      } else /* both are not PUSM READY */ {
        pflib_log(debug) << "neither lpGBT have status PUSM READY (19)";
        pflib_log(debug) << "applying standard lpGBT configuration";
        try {
          pflib_log(debug) << "applying DAQ";
          pflib::lpgbt::standard_config::setup_hcal_daq(*daq_lpgbt_);
          pflib_log(debug) << "pause to let hardware re-sync";
          sleep(2);
          pflib_log(debug) << "applying TRIG";
          pflib::lpgbt::standard_config::setup_hcal_trig(*trig_lpgbt_);
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
    econ_i2c_ = std::make_shared<pflib::lpgbt::I2C>(*daq_lpgbt_, I2C_BUS_ECONS);
    econ_i2c_->set_bus_speed(1000);
    add_econ(0, 0x60 | 0, "econd", econ_i2c_);
    add_econ(1, 0x20 | 0, "econt", econ_i2c_);
    add_econ(2, 0x20 | 1, "econt", econ_i2c_);

    roc_i2c_ =
        std::make_shared<pflib::lpgbt::I2C>(*daq_lpgbt_, I2C_BUS_HGCROCS);
    roc_i2c_->set_bus_speed(1000);
    for (int ibd = 0; ibd < 4; ibd++) {
      if ((board_mask & (1 << ibd)) == 0) continue;
      std::shared_ptr<pflib::I2C> bias_i2c =
          std::make_shared<pflib::lpgbt::I2CwithMux>(*trig_lpgbt_, I2C_BUS_BIAS,
                                                     ADDR_MUX_BIAS, (1 << ibd));
      std::shared_ptr<pflib::I2C> board_i2c =
          std::make_shared<pflib::lpgbt::I2CwithMux>(
              *trig_lpgbt_, I2C_BUS_BOARD, ADDR_MUX_BOARD, (1 << ibd));

      add_roc(ibd, 0x20 | (ibd * 8), "sipm_rocv3b", roc_i2c_, bias_i2c,
              board_i2c);
    }

    // copy I2C connections into Target
    // in case user wants to do raw I2C transactions for testing
    for (auto [bid, conn] : roc_connections_) {
      i2c_[pflib::utility::string_format("HGCROC_%d", bid)] = conn.roc_i2c_;
      i2c_[pflib::utility::string_format("BOARD_%d", bid)] = conn.board_i2c_;
      i2c_[pflib::utility::string_format("BIAS_%d", bid)] = conn.bias_i2c_;
    }
    for (auto [bid, conn] : econ_connections_) {
      i2c_[pflib::utility::string_format("ECON_%d", bid)] = conn.i2c_;
    }

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
  std::shared_ptr<pflib::I2C> roc_i2c_, econ_i2c_;
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
