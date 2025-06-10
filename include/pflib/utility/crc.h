#pragma once

#include <span>
#include <cstdint>

namespace pflib::utility {

/**
 * Calculate the CRC checksum
 *
 * @param[in] data 32-bit words to calculate CRC for
 * @return value of CRC
 */
uint32_t crc(std::span<uint32_t> data);

}
