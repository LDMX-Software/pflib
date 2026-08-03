#pragma once
#ifndef PFLIB_PACKING_DECODEAEBM_H
#define PFLIB_PACKING_DECODEAEBM_H

#include "pflib/packing/Mask.h"

namespace pflib::packing {

/**
 * Generically decompress AE+BM encoding
 *
 * AE+BM stands for A bits of exponnent and B bits for a
 * mantissa giving a total compressed size of A+B bits.
 * At time of writing, we've seen two copies of this
 * encoding: The ROC's Trigger Cells use 4E+3M and the
 * ECON-T's Super Trigger Cells use 5E+4M.
 *
 * This decompression assembles the encoded value assigning
 * the B least significant bits to be the mantissa value
 * and the A most significant bits to be the exponent.
 *
 * `unsigned long` is used to manipulate the integers to avoid
 * size issues, use `static_cast` to intentially cut the return
 * value down to a smaller size if desired.
 *
 * @tparam A number of bits for the exponent
 * @tparam B number of bits for the mantissa
 * @param[in] encoded compressed value to decompress
 * @return decompressed value
 */
template <unsigned short A, unsigned short B>
unsigned long decompressAEBM(unsigned long encoded) {
  static_assert(A + B < 12);  // arbitrary limit to avoid typos
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

}  // namespace pflib::packing

#endif
