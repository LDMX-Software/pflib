#include "pflib/algorithm/tp50_scan.h"

#include "pflib/DecodeAndBuffer.h"
#include "pflib/utility/string_format.h"

namespace pflib::algorithm {

std::array<int, 2> tp50_scan(Target* tgt, ROC roc, std::array<int, 72> calibs) {
  static auto the_log_{::pflib::logging::get("tp50_scan")};

  std::array<int, 2> target;

  return target;
}

}  // namespace pflib::algorithm
