/**
 * @file utility.h some random utilities that have been used in more than one place
 */
#include <functional>
#include <string>

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

}  // namespace pflib
