
#include "pflib/packing/Sample.h"

#include "pflib/packing/Mask.h"

namespace pflib::packing {

bool Sample::Tc() const { return ((this->word >> 31) & 0b1) == 1; }

bool Sample::Tp() const { return ((this->word >> 30) & 0b1) == 1; }

int Sample::toa() const { return (this->word & mask<10>); }

int Sample::adc_tm1() const {
  // weird situation without pre-sample
  if (Tc() and not Tp()) return -1;
  // otherwise, just the 10 bits after the two flags
  return ((this->word >> 20) & mask<10>);
}

int Sample::adc() const {
  if (not Tc()) {
    return ((this->word >> 10) & mask<10>);
  } else if (not Tp()) {
    return ((this->word >> 20) & mask<10>);
  } else {
    return -1;
  }
}

int Sample::tot() const {
  if (Tc()) {
    return ((this->word >> 10) & mask<10>);
  } else {
    return -1;
  }
}

const std::string Sample::to_csv_header = "Tp,Tc,adc_tm1,adc,tot,toa";

void Sample::to_csv(std::ofstream& f) const {
  f << Tp() << ',' << Tc() << ',' << adc_tm1() << ',' << adc() << ',' << tot()
    << ',' << toa();
}

void Sample::from_unpacked(int adc_tm1, int adc, int tot, int toa) {
  /**
   * We assume that negative values have been zero-suppressed by the ECON-D
   * and thus should be represented by a value of zero.
   * The only exception to this is the distinction between TOT and ADC mode.
   * Since the TOT is _never_ zero-suppressed, if it is a positive value
   * we assume that the channel was in TOT mode.
   */
  bool Tc = (tot > 0);
  bool Tp = (tot > 0);
  int msb_meas = (adc_tm1 < 0) ? 0 : adc_tm1;
  int lsb_meas = (toa < 0) ? 0 : toa;
  int mid_meas = (tot > 0) ? tot : ((adc < 0) ? 0 : adc);
  word = (Tc << 31) + (Tp << 30) + ((msb_meas & mask<10>) << 20) + ((mid_meas & mask<10>) << 10) + (lsb_meas & mask<10>);
}

}  // namespace pflib::packing
