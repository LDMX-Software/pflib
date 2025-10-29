#include "pflib/algorithm/tot_vref_scan.h"

#include "pflib/algorithm/get_calibs.h"

#include "pflib/DecodeAndBuffer.h"
#include "pflib/utility/efficiency.h"
#include "pflib/utility/string_format.h"

namespace pflib::algorithm {

std::map<std::string, std::map<std::string, uint64_t>> tot_vref_scan(
    Target* tgt, ROC roc) {
  static auto the_log_{::pflib::logging::get("tot_vref_scan")};

  // Iterate through each channel. First, find the CALIB that corresponds to
  // a maximum ADC of 950. Second, do an efficiency scan to find the 50% efficiency mark.
  // Set the tot_vref to the point closest to this mark. 
  // Then, move on to the next channel. Find the CALIB. Scan the efficiency for the
  // current tot_vref. If the efficiency is < 0.5, then move on the the next channel.
  // If the efficiency is > 0.5, raise the tot_vref by increments of 1 until the 
  // efficiency is < 0.5.

  // Get the calibs for all channels
  int target_adc = 950;
  auto calibs = get_calibs(tgt, roc, target_adc);

  std::array<int, 2>
      target;  // tot_vref is a global parameter (1 value per link)
  
  for (int ch = 0; ch < 72; i++) {
    // Set the calib value
    int i_link = ch / 36;
    auto refvol_page = pflib::utility::string_format("REFERENCEVOLTAGE_%d", i_link);
    auto calib_handle = roc.testParameters()
                           .add(refvol_page, "CALIB", calib)
                           .apply();
    // Run the threshold scan T_50 that gives the tot_vref value as output
    if (target[i_link]) {
      int tot_vref = T50(tgt, roc, calibs[ch], tot_vref=target[i_link]);
    }
    else {
      int tot_vref = T50(tgt, roc, calibs[ch]);
    }
    // Save to target
    target[i_link] = tot_vref;
  }
  std::map<std::string, std::map<std::string, uint64_t>> settings;
  for (int i_link{0}; i_link < 2; i_link++) {
    auto refvol_page = pflib::utility::string_format("REFERENCEVOLTAGE_%d", i_link);
    settings[refvol_page]["TOT_VREF"] = target[i_link];
  }
  return settings;
}

}  // namespace pflib::algorithm
