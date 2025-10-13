#include "pflib/Target.h"
#include "pflib/lpgbt/I2C.h"
#include "pflib/lpgbt/lpGBT_standard_config.h"
#include "pflib/utility/string_format.h"
#include "pflib/zcu/lpGBT_ICEC_ZCU_Simple.h"
#include "pflib/zcu/zcu_optolink.h"

namespace pflib {

static constexpr int ADDR_HCAL_BACKPLANE_DAQ = 0x78 | 0x04;
static constexpr int ADDR_HCAL_BACKPLANE_TRIG = 0x78;
static constexpr int I2C_BUS_ECONS = 0; // DAQ
static constexpr int I2C_BUS_HGCROCS = 1;  // DAQ
static constexpr int I2C_BUS_BIAS = 1;     // TRIG
static constexpr int I2C_BUS_BOARD = 0;    // TRIG
static constexpr int ADDR_MUX_BIAS = 0x70;
static constexpr int ADDR_MUX_BOARD = 0x71;

class HcalBackplaneZCU : public Hcal {
 public:
  HcalBackplaneZCU(int itarget, uint8_t board_mask) {
    // first, setup the optical links
    std::string uio_coder =
        pflib::utility::string_format("standardLpGBTpair-%d", itarget);
    daq_tport_ = std::make_unique<pflib::zcu::lpGBT_ICEC_Simple>(
        uio_coder, false, ADDR_HCAL_BACKPLANE_DAQ);
    trig_tport_ = std::make_unique<pflib::zcu::lpGBT_ICEC_Simple>(
        uio_coder, true, ADDR_HCAL_BACKPLANE_TRIG);
    daq_lpgbt_ = std::make_unique<pflib::lpGBT>(*daq_tport_);
    trig_lpgbt_ = std::make_unique<pflib::lpGBT>(*trig_tport_);

    // next, create the Hcal I2C objects
    econ_i2c_ = std::make_shared<pflib::lpgbt::I2C>(*daq_lpgbt_, I2C_BUS_ECONS);
    econ_i2c_->set_bus_speed(1000);
    add_econ(0, 0x60 | 0, "econd", econ_i2c_)
    add_econ(0, 0x20 | 0, "econt", econ_i2c_)
    add_econ(1, 0x20 | 1, "econt", econ_i2c_)

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
      // TODO allow for board->typ_version configuration to be passed here
      // right now its hardcoded because everyone has one of these
      // but we could modify this constructor and its calling factory
      // function in order to pass in a configuration
      add_roc(
        ibd, 0x28 | (ibd*8), "sipm_rocv3b",
        roc_i2c_, bias_i2c, board_i2c
      );
    }

    // TODO create ELinks and DAQ objects

    pflib::lpgbt::standard_config::setup_hcal_trig(*trig_lpgbt_);
    pflib::lpgbt::standard_config::setup_hcal_daq(*daq_lpgbt_);
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

  virtual void softResetECONs() override {
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

  virtual Elinks& elinks() override {
    PFEXCEPTION_RAISE("NoImpl", "HcalBackplaneZCU::elinks not implemented");
  }

  virtual DAQ& daq() override {
    PFEXCEPTION_RAISE("NoImpl", "HcalBackplaneZCU::daq not implemented");
  }

 private:
  /// let the target that holds this Hcal see our members
  friend class HcalBackplaneZCUTarget;
  std::unique_ptr<pflib::zcu::lpGBT_ICEC_Simple> daq_tport_, trig_tport_;
  std::unique_ptr<lpGBT> daq_lpgbt_, trig_lpgbt_;
  std::shared_ptr<pflib::I2C> roc_i2c_, econ_i2c_;
};

class HcalBackplaneZCUTarget: public Target {
 public:
  HcalBackplaneZCUTarget(int ilink, uint8_t board_mask): Target() {
    auto hcal_ptr = std::make_shared<HcalBackplaneZCU>(ilink, board_mask);
    hcal_ = hcal_ptr;

    // copy I2C connections into Target
    // in case user wants to do raw I2C transactions for testing
    for (auto [bid, conn]: hcal_ptr->roc_connections_) {
      i2c_[pflib::utility::string_format("HGCROC_%d", bid)] = conn.roc_i2c_;
      i2c_[pflib::utility::string_format("BOARD_%d", bid)] = conn.board_i2c_;
      i2c_[pflib::utility::string_format("BIAS_%d", bid)] = conn.bias_i2c_;
    }
    for (auto [bid, conn]: hcal_ptr->econ_connections_) {
      i2c_[pflib::utility::string_format("ECON_%d", bid)] = conn.i2c_;
    }

    // TODO make FastControl object
  }

  virtual std::vector<uint32_t> read_event() override {
    PFEXCEPTION_RAISE("NoImpl", "HcalBackplaneZCUTarget::read_event not implemented");
    std::vector<uint32_t> empty;
    return empty;
  }
};

Target* makeTargetHcalBackplaneZCU(int ilink, uint8_t board_mask) {
  return new HcalBackplaneZCUTarget(ilink, board_mask);
}

}  // namespace pflib
