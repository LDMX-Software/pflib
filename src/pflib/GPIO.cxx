#include "pflib/GPIO.h"

#include <stdio.h>

#include "pflib/Exception.h"

namespace pflib {

bool GPIO::getGPI(int ibit) {
  if (ibit >= ngpi_) {
    char message[120];
    snprintf(message, 120, "GPI bit %d is out of range with maxGPI %d", ibit,
             ngpi_);
    PFEXCEPTION_RAISE("GPIOError", message);
  }

  std::vector<bool> bits = getGPI();
  return bits[ibit];
}

void GPIO::setGPO(int ibit, bool toTrue) {
  if (ibit >= ngpo_) {
    char message[120];
    snprintf(message, 120, "GPO bit %d is out of range with maxGPO=%d", ibit,
             ngpo_);
    PFEXCEPTION_RAISE("GPIOError", message);
  }
  std::vector<bool> bits = getGPO();
  bits[ibit] = toTrue;
  setGPO(bits);
}

}  // namespace pflib
