#pragma once

#include <functional>
#include <optional>
#include <string>

namespace pflib::utility {

/**
 * Load an integer CSV file, doing some Action on each row
 *
 * This function skips blank lines and comment lines that start with the `#` character.
 *
 * Header rows are *not* skipped.
 * If you want to skip the header, prepend it with the `#` character so that it is treated as a comment line.
 * You can also use the HeaderAction argument to handle the header row yourself.
 *
 * @param[in] file_name path to CSV to open and read
 * @param[in] Action function that receives the row of integer values and does something with them
 * @param[in] HeaderAction optional function that receives a header row of strings, default nothing
 *
 * @throws Exception if no file is provided or file cannot be opened
 */
void load_integer_csv(const std::string& file_name,
                      std::function<void(const std::vector<int>&)> Action,
                      std::optional<std::function<void(const std::vector<std::string>&)>> HeaderAction = std::nullopt);

}
