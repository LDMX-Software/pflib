#include "pflib/algorithm/tot_vref_scan.h"

#include "pflib/DecodeAndBuffer.h"
#include "pflib/algorithm/get_calibs.h"
#include "pflib/algorithm/tp50_scan.h"
#include "pflib/algorithm/trim_tot_scan.h"
#include "pflib/utility/string_format.h"

namespace pflib::algorithm {

std::map<std::string, std::map<std::string, uint64_t>> tot_vref_scan(
    Target* tgt, ROC roc) {
  static auto the_log_{::pflib::logging::get("tot_vref_scan")};

  // Iterate through each channel. First, find the CALIB that corresponds to
  // a maximum ADC of 950. Second, do an efficiency scan to find the 50%
  // efficiency mark. Set the tot_vref to the point closest to this mark. Then,
  // move on to the next channel. Find the CALIB. Scan the efficiency for the
  // current tot_vref. If the efficiency is < 0.5, then move on the the next
  // channel. If the efficiency is > 0.5, raise the tot_vref by increments of 1
  // until the efficiency is < 0.5.

  // Get the calibs for all channels
  int target_adc = 950;  // What target does CMS use? This should be hardcoded
  auto calibs = get_calibs(tgt, roc, target_adc);

  std::array<int, 2>
      target;  // tot_vref is a global parameter (1 value per link)
  
  // Get tot_vref values
  target = tp50_scan(tgt, roc, calibs)

  std::map<std::string, std::map<std::string, uint64_t>> settings;
  for (int i_link{0}; i_link < 2; i_link++) {
    auto refvol_page =
        pflib::utility::string_format("REFERENCEVOLTAGE_%d", i_link);
    settings[refvol_page]["TOT_VREF"] = target[i_link];
  }

  return settings;
}

}  // namespace pflib::algorithm
