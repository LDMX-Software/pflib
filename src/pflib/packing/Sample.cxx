
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

const std::string Sample::to_csv_header =
  "Tp,Tc,adc_tm1,adc,tot,toa";

void Sample::to_csv(std::ofstream& f) const {
  f << Tp() << ',' << Tc() << ','
    << adc_tm1() << ',' << adc() << ',' << tot() << ',' << toa();
}

}  // namespace pflib::packing
