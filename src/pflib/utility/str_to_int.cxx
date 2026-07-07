#include "pflib/utility/str_to_int.h"

#include <limits>
#include <regex>

namespace pflib::utility {

bool is_integer(const std::string& str) {
  static const std::regex is_integer(
      R"REGEX(^\s*(0x[0-9a-f]+|0b[0-1]+|0[0-8]+|[0-9]+)\s*$)REGEX",
      std::regex_constants::icase);
  return std::regex_match(str, is_integer);
}

unsigned long long int str_to_ullint(std::string str) {
  if (str == "0") return 0;
  int base = 10;
  if (str[0] == '0' and str.length() > 2) {
    // binary or hexadecimal
    if (str[1] == 'b' or str[1] == 'B') {
      base = 2;
      str = str.substr(2);
    } else if (str[1] == 'x' or str[1] == 'X') {
      base = 16;
      str = str.substr(2);
    } else {
      // octal
      base = 8;
      str = str.substr(1);
    }
  } else if (str[0] == '0' and str.length() > 1) {
    // octal
    base = 8;
    str = str.substr(1);
  }

  return std::stoull(str, nullptr, base);
}

int str_to_int(std::string str) {
  unsigned long long raw_val = str_to_ullint(str);
  if (raw_val > std::numeric_limits<int>::max()) {
    throw std::range_error("the string '" + str + "' results in a value '" +
                           std::to_string(raw_val) +
                           "' unable to fit within an 'int'.");
  }
  return static_cast<int>(raw_val);
}

}  // namespace pflib::utility
