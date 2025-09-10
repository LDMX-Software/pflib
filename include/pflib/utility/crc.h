#pragma once

#include <cstdint>
#include <span>

namespace pflib::utility {

/**
 * Calculate the CRC checksum as it is done on the HGCROC
 *
 * @param[in] data 32-bit words to calculate CRC for
 * @return value of CRC
 */
uint32_t hgcroc_crc32(std::span<uint32_t> data);

/**
 * Calculate the CRC checksum as it is done on the ECOND
 *
 * @param[in] data 32-bit words to calculate CRC for
 * @return value of CRC
 */
uint32_t econd_crc32(std::span<uint32_t> data);

/**
 * Calculate the 8-bit CRC checksum as it is done for the event header on the ECOND
 *
 * @param[in] data 64-bit word containing both ECOND event packet headers
 * @return value of CRC
 */
uint8_t econd_crc8(uint64_t data);

}  // namespace pflib::utility
