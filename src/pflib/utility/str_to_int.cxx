#include "pflib/utility/str_to_int.h"

namespace pflib::utility {

int str_to_int(std::string str) {
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
    }
  } else if (str[0] == '0' and str.length() > 1) {
    // octal
    base = 8;
    str = str.substr(1);
  }

  return std::stoi(str, nullptr, base);
}

}
