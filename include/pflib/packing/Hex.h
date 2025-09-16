#pragma once

#include <iomanip>

namespace pflib::packing {

/**
 * @struct hex
 * A very simple wrapper enabling us to more easily
 * tell the output stream to style the input word
 * in hexidecimal format.
 *
 * @tparam[in] WordType type of word for styling
 */
template <
  typename WordType,
  std::size_t HexWidth = 2*sizeof(WordType)
> struct hex {
  WordType& word_;
  hex(WordType& w) : word_{w} {}
  friend inline std::ostream& operator<<(
      std::ostream& os, const pflib::packing::hex<WordType, HexWidth>& h) {
    os << "0x" << std::setfill('0') << std::setw(HexWidth) << std::hex
       << h.word_ << std::dec;
    return os;
  }
};

}  // namespace pflib::packing
