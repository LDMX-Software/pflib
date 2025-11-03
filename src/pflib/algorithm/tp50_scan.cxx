#include "pflib/algorithm/tp50_scan.h"

#include "pflib/DecodeAndBuffer.h"
#include "pflib/utility/string_format.h"

namespace pflib::algorithm {

int tp50_scan(Target* tgt, ROC roc, int calib, int tot_vref = -1) {
  static auto the_log_{::pflib::logging::get("tp50_scan")};

  // If a tot_vref is NULL, then we run a bisectional search for the 50
  // efficiency mark. If tot_vref is given, we raise the tot_vref if it is above
  // 50% until it is below.

  return tot_vref;
}

}  // namespace pflib::algorithm
