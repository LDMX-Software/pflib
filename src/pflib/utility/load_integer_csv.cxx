#include "pflib/utility/load_integer_csv.h"

#include <fstream>
#include <optional>
#include <sstream>
#include <vector>

#include "pflib/Exception.h"
#include "pflib/utility/str_to_int.h"

namespace pflib::utility {

/**
 * get next row from the input stream
 *
 * @tparam CellType type held within cell
 * @param[in] ss input stream to read from
 * @param[in] conversion function to convert the cell string into desired type
 * @return optional vector containing cells
 */
template <typename CellType>
static std::optional<std::vector<CellType>> get_next_row(
    std::istream& ss, std::function<CellType(std::string)> conversion) {
  std::string line;
  std::getline(ss, line);

  /**
   * if the line we read is empty or starts with `#`,
   * then return std::nullopt to signal that there wasn't
   * a row
   */
  if (line.empty() or line[0] == '#') {
    // empty line or comment, return nullopt
    return std::nullopt;
  }

  /**
   * when the line is not empty or a comment,
   * fill the vector with cells separated by `,`.
   */
  std::vector<CellType> result;
  std::string cell;
  std::stringstream line_stream(line);
  while (std::getline(line_stream, cell, ',')) {
    result.push_back(conversion(cell));
  }

  // checks for a trailing comma with no data after it
  if (!line_stream and cell.empty()) {
    // trailing comma, put in one more empty cell
    result.push_back(conversion(cell));
  }

  return result;
}

void load_integer_csv(
    const std::string& file_name,
    std::function<void(const std::vector<int>&)> Action,
    std::optional<std::function<void(const std::vector<std::string>&)>>
        HeaderAction) {
  if (file_name.empty()) {
    PFEXCEPTION_RAISE("Filename", "No file provided to load CSV function.");
  }
  std::ifstream f{file_name};
  if (not f.is_open()) {
    PFEXCEPTION_RAISE("File",
                      "Unable to open " + file_name + " in load CSV function.");
  }

  bool handled_header{false};
  while (f) {
    /**
     * If the user provides a HeaderAction and we have not seen a header yet,
     * then get the next line and apply the HeaderAction.
     */
    if (HeaderAction and not handled_header) {
      auto cells =
          get_next_row<std::string>(f, [](std::string s) { return s; });
      if (cells) {
        HeaderAction.value()(cells.value());
        handled_header = true;
      }
    } else {
      /**
       * If the user did not provide a HeaderAction or we already handled
       * the header, get the next line and apply the user's action if
       * the line is not a comment or empty.
       */
      auto cells = get_next_row<int>(f, [](std::string s) {
        if (s.empty()) {
          return 0;
        } else {
          return str_to_int(s);
        }
      });
      if (cells) {
        Action(cells.value());
      }
    }
  }
}

}  // namespace pflib::utility
