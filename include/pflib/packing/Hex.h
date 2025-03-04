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
template <typename WordType>
struct hex {
  static const std::size_t width_{2*sizeof(WordType)};
  WordType& word_;
  hex(WordType& w) : word_{w} {}
  friend inline std::ostream& operator<<(
      std::ostream& os, const pflib::packing::hex<WordType>& h) {
    os << "0x" << std::setfill('0') << std::setw(h.width_) << std::hex
       << h.word_ << std::dec;
    return os;
  }
};

}  // namespace pflib::packing

