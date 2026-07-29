#pragma once
#ifndef PFLIB_PACKING_DECODEAEBM_H
#define PFLIB_PACKING_DECODEAEBM_H

#include "pflib/packing/Mask.h"

namespace pflib::packing {

template<unsigned short A, unsigned short B>
unsigned long decodeAEBM(unsigned long encoded) {
  static_assert(A > 0);
  static_assert(B > 0);
  static_assert(A+B < 32);
  unsigned long mantissa = (encoded & mask<B>);
  unsigned long exponent = ((encoded >> B) & mask<A>);
  if (exponent == 0) {
    return mantissa;
  }
  unsigned long output = (((1 << B) | mantissa) << (exponent - 1));
  if (exponent > 1) {
    output |= (1 << (exponent - 2));
  }
  return output;
}

}

#endif
