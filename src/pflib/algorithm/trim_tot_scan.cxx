#include "pflib/algorithm/trim_tot_scan.h"

#include <cmath>

#include "pflib/DecodeAndBuffer.h"
#include "pflib/utility/string_format.h"
#include "pflib/utility/efficiency.h"

namespace pflib::algorithm {

std::map<std::string, std::map<std::string, uint64_t>> trim_tot_scan(
    Target* tgt, ROC roc, std::array<int, 72> calibs) {
  static auto the_log_{::pflib::logging::get("trim_tot_scan")};

  // Iterate through each channel. For each channel, trim down the tot threshold
  // and set the parameter to the point which minimizes abs(tot_efficiency-0.5).

  static const std::size_t n_events = 100;
  DecodeAndBuffer buffer{n_events};  // working in buffer, not in writer

  std::map<std::string, std::map<std::string, uint64_t>> settings;
  for (int ch{0}; ch < 72; ch++) {
    int target{0};
    // Set the calib value
    int i_link = ch / 36;
    auto refvol_page =
        pflib::utility::string_format("REFERENCEVOLTAGE_%d", i_link);
    auto ch_page = pflib::utility::string_format("CH_%d", ch);
    auto calib_handle =
        roc.testParameters().add(refvol_page, "CALIB", calibs[ch]).apply();
    // Run the threshold scan TP50 that gives the tot_vref value as output
    for (int trim_tot{0}; trim_tot < 64; trim_tot++) {
      pflib_log(info) << "testing trim_tot = " << trim_tot;
      auto test_handle =
          roc.testParameters().add(ch_page, "TRIM_TOT", trim_tot).apply();
      usleep(10);
      tgt->daq_run("CHARGE", buffer, n_events, 100);
      auto data = buffer.get_buffer();
      std::vector<int> tots(data.size());
      for (std::size_t i{0}; i < tots.size(); i++) {
        tots[i] = data[i].channel(ch).tot();
      }
      auto efficiency = pflib::utility::efficiency(tots);
      if (efficiency < 0.5) {
        target = trim_tot;
        continue;
      } else {
        if (std::abs(target - 0.5) <= std::abs(trim_tot - 0.5)) {
          target = trim_tot - 1;
          break;
        } else {
          target = trim_tot;
          break;
        }
      }
    }
    pflib_log(info) << "Final trim_tot = " << target;
    settings[ch_page]["TRIM_TOT"] = target;
  }
  pflib_log(info) << "Trim_tot retrieved for all channels";
  return settings;
}

}  // namespace pflib::algorithm
