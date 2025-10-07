#include "pflib/Target.h"
#include "pflib/lpgbt/I2C.h"
#include "pflib/utility/string_format.h"
#include "pflib/zcu/lpGBT_ICEC_ZCU_Simple.h"
#include "pflib/zcu/zcu_optolink.h"

namespace pflib {

static constexpr int ADDR_HCAL_BACKPLANE_DAQ = 0x78 | 0x04;
static constexpr int ADDR_HCAL_BACKPLANE_TRIG = 0x78;
static constexpr int I2C_BUS_HGCROCS = 1;  // DAQ
static constexpr int I2C_BUS_BIAS = 1;     // TRIG
static constexpr int I2C_BUS_BOARD = 0;    // TRIG
static constexpr int ADDR_MUX_BIAS = 0x70;
static constexpr int ADDR_MUX_BOARD = 0x71;

class HcalBackplaneZCUTarget : public Target, public Hcal {
 public:
  HcalBackplaneZCUTarget(int itarget, uint8_t board_mask) {
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
      add_roc(ibd, roc_i2c_, bias_i2c, board_i2c);
    }

    // next, create the elinks object

    hcal_ = std::shared_ptr<Hcal>(this);
  }
  virtual void softResetROC(int which) {
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
  virtual void hardResetROCs() {
    trig_lpgbt_->gpio_interface().setGPO("HGCROC0_HRST", false);
    trig_lpgbt_->gpio_interface().setGPO("HGCROC0_HRST", true);
    daq_lpgbt_->gpio_interface().setGPO("HGCROC1_HRST", false);
    daq_lpgbt_->gpio_interface().setGPO("HGCROC1_HRST", true);
    daq_lpgbt_->gpio_interface().setGPO("HGCROC2_HRST", false);
    daq_lpgbt_->gpio_interface().setGPO("HGCROC2_HRST", true);
    trig_lpgbt_->gpio_interface().setGPO("HGCROC3_HRST", false);
    trig_lpgbt_->gpio_interface().setGPO("HGCROC3_HRST", true);
  }
  virtual Elinks& elinks() {}
  virtual DAQ& daq() {}
  virtual std::vector<uint32_t> read_event() {
    std::vector<uint32_t> empty;
    return empty;
  }

 private:
  std::unique_ptr<pflib::zcu::lpGBT_ICEC_Simple> daq_tport_, trig_tport_;
  std::unique_ptr<lpGBT> daq_lpgbt_, trig_lpgbt_;
  std::shared_ptr<pflib::I2C> roc_i2c_;
};

Target* makeTargetHcalBackplaneZCU(int ilink, uint8_t board_mask) {
  return new HcalBackplaneZCUTarget(ilink, board_mask);
}

}  // namespace pflib
