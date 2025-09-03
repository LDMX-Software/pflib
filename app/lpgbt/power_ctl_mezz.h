#ifndef POWER_CTL_MEZZ_H_INCLUDED
#define POWER_CTL_MEZZ_H_INCLUDED

#include "ad5593r.h"

namespace pflib {

class power_ctl_mezz {
  static constexpr int POWER_GD = 0;
  static constexpr int VECON = 1;
  static constexpr int ISENSE = 2;
  static constexpr int MARGSEL = 3;
  static constexpr int MARGTOL = 4;
  static constexpr int VTUNE0 = 5;
  static constexpr int VTUNE1 = 6;
  static constexpr int VTUNE2 = 7;

 public:
  power_ctl_mezz(const std::string& path, bool keep_settings = false)
      : regctl_(path, 0x11) {
    regctl_.setup_gpi(POWER_GD);
    regctl_.setup_adc(VECON);
    regctl_.setup_adc(ISENSE);
    if (!keep_settings) {
      // default is margining off
      regctl_.setup_gpi(MARGSEL);
      // default is 3% margin
      regctl_.setup_gpi(MARGTOL);
      // default is 1.2v
      regctl_.setup_gpo(VTUNE2, 0);
      regctl_.setup_gpo(VTUNE1, 1);
      regctl_.setup_gpo(VTUNE0, 1);
    }
  }

 private:
  void vtune(int ibit, int val) {
    const int pinmap[] = {VTUNE0, VTUNE1, VTUNE2};
    if (ibit < 0 || ibit > 2) return;
    int pin = pinmap[ibit];
    if (val < 0) {
      regctl_.setup_gpi(pin, false);
    } else {
      regctl_.setup_gpo(pin, (val > 0));
    }
  }

 public:
  void set_volts(float voltage) {
    int ivolts = int((voltage + 0.02) / 50e-3) - (16);
    const int maptable[15][3] = {
        {0, 0, 0},   {0, 0, -1}, {0, 0, 1},   {0, -1, 0},   {0, -1, -1},
        {0, -1, 1},  {0, 1, 0},  {0, 1, -1},  {0, 1, 1},    {-1, 0, 0},
        {-1, 0, -1}, {-1, 0, 1}, {-1, -1, 0}, {-1, -1, -1}, {-1, -1, 1}};
    // eventually, we could add margining to tune even more...
    if (ivolts >= 0 && ivolts < 15) {
      vtune(0, maptable[ivolts][2]);
      vtune(1, maptable[ivolts][1]);
      vtune(2, maptable[ivolts][0]);
    }
  }

  float get_setting() {
    const int pinmap[] = {VTUNE2, VTUNE1, VTUNE0};
    int index = 0;
    for (int i = 0; i < 3; i++) {
      int pin = pinmap[i];
      index = index * 3;
      if (regctl_.is_gpo(pin)) {
        if (regctl_.gpo_get_value(pin)) index = index + 2;
      } else
        index = index + 1;
    }
    return ((index + 16) * 50e-3);
  }

  float econ_volts() { return regctl_.adc_volts(VECON); }

  float econ_current_mA() {
    float deltaV = regctl_.adc_volts(ISENSE) - regctl_.adc_volts(VECON);
    return deltaV / 50e-3 * 1000;
  }

 private:
  AD5593R regctl_;
};

}  // namespace pflib

#endif  // POWER_CTL_MEZZ_H_INCLUDED
