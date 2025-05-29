#include "pflib/utility.h"

#include <fstream>
#include <sstream>
#include <vector>

#include <boost/endian/conversion.hpp>
#include <boost/crc.hpp>

#include "pflib/Exception.h"

namespace pflib {

/**
 * Modern RAII-styled CSV extractor taken from
 * https://stackoverflow.com/a/1120224/17617632
 * this allows us to **discard white space within the cells**
 * making the CSV more human readable.
 */
static std::vector<int> getNextLineAndExtractValues(std::istream& ss) {
  std::vector<int> result;

  std::string line, cell;
  std::getline(ss, line);

  if (line.empty() or line[0] == '#') {
    // empty line or comment, return empty container
    return result;
  }

  std::stringstream line_stream(line);
  while (std::getline(line_stream, cell, ',')) {
    /**
     * std stoi has a auto-detect base feature
     * https://en.cppreference.com/w/cpp/string/basic_string/stol
     * which we can enable by setting the pre-defined base to 0
     * (the third parameter) - this auto-detect base feature
     * can handle hexidecial (prefix == 0x or 0X), octal (prefix == 0),
     * and decimal (no prefix).
     *
     * The second parameter is an address to put the number of characters
     * processed, which I disregard at this time.
     *
     * Do we allow empty cells?
     */
    result.push_back(cell.empty() ? 0 : std::stoi(cell, nullptr, 0));
  }
  // checks for a trailing comma with no data after it
  if (!line_stream and cell.empty()) {
    // trailing comma, put in one more 0
    result.push_back(0);
  }

  return result;
}

void loadIntegerCSV(
    const std::string& file_name,
    const std::function<void(const std::vector<int>&)>& Action) {
  if (file_name.empty()) {
    PFEXCEPTION_RAISE("Filename", "No file provided to load CSV function.");
  }
  std::ifstream f{file_name};
  if (not f.is_open()) {
    PFEXCEPTION_RAISE("File",
                      "Unable to open " + file_name + " in load CSV function.");
  }
  while (f) {
    auto cells = getNextLineAndExtractValues(f);
    // skip comments and blank lines
    if (cells.empty()) continue;
    Action(cells);
  }
}

bool endsWith(const std::string& full, const std::string& ending) {
  if (full.length() < ending.length()) return false;
  return (0 == full.compare(full.length() - ending.length(), ending.length(),
                            ending));
}

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

}  // namespace pflib
