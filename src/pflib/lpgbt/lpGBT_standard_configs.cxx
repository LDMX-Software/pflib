#include "pflib/lpgbt/lpGBT_standard_configs.h"

namespace pflib {
namespace lpgbt {
namespace standard_config {

void setup_hcal_daq(pflib::lpGBT& lpgbt) {
  // setup the reset lines
  lpgbt.gpio_cfg_set(
      0, lpGBT::GPIO_IS_OUTPUT | lpGBT::GPIO_IS_PULLUP | lpGBT::GPIO_IS_STRONG,
      "HGCROC2_HRST");
  lpgbt.gpio_set(0, true);
  lpgbt.gpio_cfg_set(
      1, lpGBT::GPIO_IS_OUTPUT | lpGBT::GPIO_IS_PULLUP | lpGBT::GPIO_IS_STRONG,
      "HGCROC2_SRST");
  lpgbt.gpio_set(1, true);
  lpgbt.gpio_cfg_set(
      2, lpGBT::GPIO_IS_OUTPUT | lpGBT::GPIO_IS_PULLUP | lpGBT::GPIO_IS_STRONG,
      "HGCROC1_HRST");
  lpgbt.gpio_set(2, true);
  lpgbt.gpio_cfg_set(
      4, lpGBT::GPIO_IS_OUTPUT | lpGBT::GPIO_IS_PULLUP | lpGBT::GPIO_IS_STRONG,
      "HGCROC1_SRST");
  lpgbt.gpio_set(4, true);
  lpgbt.gpio_cfg_set(
      8, lpGBT::GPIO_IS_OUTPUT | lpGBT::GPIO_IS_PULLUP | lpGBT::GPIO_IS_STRONG,
      "HGCROC_I2C_RST");
  lpgbt.gpio_set(8, true);
  lpgbt.gpio_cfg_set(
      11, lpGBT::GPIO_IS_OUTPUT | lpGBT::GPIO_IS_PULLUP | lpGBT::GPIO_IS_STRONG,
      "TRIG_LPGBT_RSTB");
  lpgbt.gpio_set(11, true);

  // setup clocks
  lpgbt.setup_eclk(0, 320);  // HGCROC3_CLK320
  lpgbt.setup_eclk(1, 320);  // HGCROC0_CLK320
  lpgbt.setup_eclk(2, 320);  // HGCROC2_CLK320
  lpgbt.setup_eclk(3, 320);  // HGCROC1_CLK320
  lpgbt.setup_eclk(4, 320);  // ECON-T0_CLK320
  lpgbt.setup_eclk(6, 320);  // ECON-T1_CLK320
  lpgbt.setup_eclk(5, 320);  // ECON-D_CLK320
  lpgbt.setup_eclk(7, 40);   // REFCLK_TO_TRIG

  // setup fast commands
  lpgbt.setup_etx(0, true);  // HGCROC2_FCMD
  lpgbt.setup_etx(1, true);  // HGCROC0_FCMD
  lpgbt.setup_etx(2, true);  // HGCROC3_FCMD
  lpgbt.setup_etx(3, true);  // ECON-T0_FCMF
  lpgbt.setup_etx(4, true);  // ECON-D_FCMD
  lpgbt.setup_etx(5, true);  // ECON-T1_FCMD
  lpgbt.setup_etx(6, true);  // HGCROC1_FCMD

  // setup the one input...
  lpgbt.setup_erx(0, 0);

  // setup the EC link
  lpgbt.setup_ec(false, 4, false, 0, false, true, false, true);

  // finalize the setup
  lpgbt.finalize_setup();
}

void setup_hcal_trig(pflib::lpGBT& lpgbt) {
  // setup the reset lines
  lpgbt.gpio_cfg_set(
      4, lpGBT::GPIO_IS_OUTPUT | lpGBT::GPIO_IS_PULLUP | lpGBT::GPIO_IS_STRONG,
      "HGCROC0_HRST");
  lpgbt.gpio_set(4, true);
  lpgbt.gpio_cfg_set(
      7, lpGBT::GPIO_IS_OUTPUT | lpGBT::GPIO_IS_PULLUP | lpGBT::GPIO_IS_STRONG,
      "HGCROC0_SRST");
  lpgbt.gpio_set(7, true);
  lpgbt.gpio_cfg_set(
      6, lpGBT::GPIO_IS_OUTPUT | lpGBT::GPIO_IS_PULLUP | lpGBT::GPIO_IS_STRONG,
      "HGCROC3_HRST");
  lpgbt.gpio_set(6, true);
  lpgbt.gpio_cfg_set(
      3, lpGBT::GPIO_IS_OUTPUT | lpGBT::GPIO_IS_PULLUP | lpGBT::GPIO_IS_STRONG,
      "HGCROC3_SRST");
  lpgbt.gpio_set(3, true);

  // setup the high speed inputs
  for (int i = 0; i < 6; i++) {
    lpgbt.setup_erx(i, 0);
  }
}

}  // namespace standard_config
}  // namespace lpgbt
}  // namespace pflib
