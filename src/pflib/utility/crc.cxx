#include "pflib/utility/crc.h"
#include <vector>
#include <boost/endian/conversion.hpp>
#include <boost/crc.hpp>

namespace pflib::utility {

uint32_t crc(std::span<uint32_t> data) {
  /**
   * We need to flip the endian-ness of the words so we have
   * to make our own copy of the data words
   */
  std::vector<uint32_t> words{data.begin(), data.end()};
  std::transform( words.begin(), words.end(), words.begin(),
    [](uint32_t w) { return boost::endian::endian_reverse(w); });
  auto input_ptr = reinterpret_cast<const unsigned char*>(words.data());
  return boost::crc<32, 0x04c11db7, 0x0, 0x0, false, false>(input_ptr, words.size()*4);
}

}
