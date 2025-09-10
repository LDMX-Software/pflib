#include "pflib/utility/crc.h"

#include <boost/crc.hpp>
#include <boost/endian/conversion.hpp>
#include <vector>
#include <iostream>
#include <iomanip>

namespace pflib::utility {

uint32_t hgcroc_crc32(std::span<uint32_t> data) {
  /**
   * We need to flip the endian-ness of the words so we have
   * to make our own copy of the data words
   */
  std::vector<uint32_t> words{data.begin(), data.end()};
  std::transform(words.begin(), words.end(), words.begin(),
                 [](uint32_t w) { return boost::endian::endian_reverse(w); });
  auto input_ptr = reinterpret_cast<const unsigned char*>(words.data());
  return boost::crc<32, 0x04c11db7, 0x0, 0x0, false, false>(input_ptr,
                                                            words.size() * 4);
}

uint32_t econd_crc32(std::span<uint32_t> data) {
  /**
   * Pretty confident we flip the endian-ness so that the bytes are processed
   * MSB to LSB after casting our array of words into an array of bytes.
   */
  std::vector<uint32_t> words{data.begin(), data.end()};
  std::transform(words.begin(), words.end(), words.begin(),
                 [](uint32_t w) { return boost::endian::endian_reverse(w); });
  auto input_ptr = reinterpret_cast<const unsigned char*>(words.data());
  /**
   * @note these CRC parameters are copied from the HGCROC but are probably
   * wrong since the CRC calculation does not match the example packet Cristina shared
   */
  return boost::crc<32, 0x04c11db7, 0x0, 0x0, false, false>(input_ptr,
                                                            words.size() * 4);
}


uint8_t econd_crc8(uint64_t data) {
  /**
   * Reverse endian-ness so when we cast to an array of bytes,
   * we read the bytes from most-significant to least-significant.
   */
  uint64_t w = boost::endian::endian_reverse(data);
  auto input_ptr = reinterpret_cast<const unsigned char*>(&w);
  /**
   * These CRC parameters are what I can find online for 8bit Bluetooth CRC,
   * but it is not matching the example shown in the ECOND Specification Figure 34.
   */
  return boost::crc<8, 0xa7, 0x00, 0x00, true, true>(input_ptr, 8);
}

}  // namespace pflib::utility
