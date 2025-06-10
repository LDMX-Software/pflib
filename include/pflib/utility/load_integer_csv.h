#pragma once

#include <functional>
#include <string>

namespace pflib::utility {

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
void load_integer_csv(const std::string& file_name,
                      const std::function<void(const std::vector<int>&)>& Action);

}
