#include "tot_vref_scan.h"

#include "../daq_run.h"
#include "../tasks/tot_vref_scan.h"
#include "get_calibs.h"
#include "tp50_scan.h"
#include "trim_tot_scan.h"
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
  // Find the trim values to set the tot threshold where it is just below 0.5 efficiency.

  // Get the calibs for all channels
  int target_adc = 950;  // What target does CMS use? This should be hardcoded

  std::array<int, 72>
      calibs;
  std::array<int, 2>
      vref_targets;  // tot_vref is a global parameter (1 value per link)
  std::array<int, 72>
      trim_targets;

  size_t n_events = 100;

  tgt->setup_run(1, pftool::state.daq_format_mode, 1);

  if (pftool::state.daq_format_mode == Target::DaqFormat::SIMPLEROC) {
    calibs = get_calibs<pflib::packing::SingleROCEventPacket>
                                              (tgt, roc, target_adc);
    vref_targets = tp50_scan<pflib::packing::SingleROCEventPacket>
                                              (tgt, roc, n_events,
                                              calibs, vref_targets);
    trim_targets = trim_tot_scan<pflib::packing::SingleROCEventPacket>
                                              (tgt, roc, n_events,
                                              calibs, vref_targets, trim_targets);
  } else if (pftool::state.daq_format_mode ==
             Target::DaqFormat::ECOND_SW_HEADERS) {
    calibs = get_calibs<pflib::packing::MultiSampleECONDEventPacket>
                                              (tgt, roc, target_adc); 
    vref_targets = tp50_scan<pflib::packing::MultiSampleECONDEventPacket>
                                              (tgt, roc, n_events,
                                              calibs, vref_targets);
    trim_targets = trim_tot_scan<pflib::packing::MultiSampleECONDEventPacket>
                                              (tgt, roc, n_events, 
                                              calibs, vref_targets, trim_targets);
  } else {
    pflib_log(warn) << "Unsupported DAQ format ("
                    << static_cast<int>(pftool::state.daq_format_mode)
                    << ") in level_pedestals. Skipping pedestal leveling...";
  }

  std::map<std::string, std::map<std::string, uint64_t>> settings;
  for (int i_link{0}; i_link < 2; i_link++) {
    auto refvol_page =
        pflib::utility::string_format("REFERENCEVOLTAGE_%d", i_link);
    settings[refvol_page]["TOT_VREF"] = vref_targets[i_link];
  }

  for (int ch{0}; ch < 72; ch++) {
    auto ch_page = pflib::utility::string_format("CH_%d", ch);
    settings[ch_page]["TRIM_TOT"] = trim_targets[ch];
  }

  return settings;
}

}  // namespace pflib::algorithm
