/**
 * @file utility.h some random utilities that have been used in more than one place
 */
#include <functional>
#include <string>
#include <cstdint>
#include <span>
#include <iostream>
#include <algorithm>
#include <filesystem>
#include <cstdlib>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <string>
#include <array>

namespace pflib {

/**
 * Load an integer CSV file, doing some Action on each row
 *
 * This function skips blank lines and comment lines that start with the `#` character.
 * Header rows are *not* skipped, so it is suggested to write the header row with a
 * leading `#` so that it is treated as a comment line.
 *
 * @param[in] file_name path to CSV to open and read
 * @param[in] Action function that receives the row of integer values and does something with them
 *
 * @throws Exception if no file is provided or file cannot be opened
 */
void loadIntegerCSV(const std::string& file_name,
                    const std::function<void(const std::vector<int>&)>& Action);

/**
 * Check if a given string has a specific ending
 *
 * @param[in] full string to check
 * @param[in] ending string to look for
 * @return true if full's last characters match ending
 */
bool endsWith(const std::string& full, const std::string& ending);

/**
 * Calculate the CRC checksum
 *
 * @param[in] data 32-bit words to calculate CRC for
 * @return value of CRC
 */
uint32_t crc(std::span<uint32_t> data);

/**
 * Execute a command and capture its output into a string
 *
 * The maximum output is 128 characters.
 *
 * @param[in] cmd command C-string to run
 * @return up to 128 characters of output by cmd
 */
std::string exec(const char* cmd);

/**
 * Check if the firmware supporting the HGCROC is active
 *
 * @return true if correct firmware is active
 */
bool is_fw_active();

/**
 * Get the current firmware version
 *
 * This takes a moment because we simply retrieve the version
 * using an rpm query and the exec function.
 *
 * @return string holding the full firmware version
 */
std::string fw_version();

}  // namespace pflib
