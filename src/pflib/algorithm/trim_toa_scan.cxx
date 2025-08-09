#include "pflib/algorithm/trim_toa_scan.h"
#include "pflib/algorithm/toa_vref_scan.h"

#include "pflib/utility/string_format.h"
#include "pflib/utility/efficiency.h"

#include "pflib/DecodeAndBuffer.h"

/**
 * calculate the TRIM_TOA for each channel that aligns them to a common trigger pulse.
 */

namespace pflib::algorithm {

std::map<std::string, std::map<std::string, int>>
trim_toa_scan(Target* tgt, ROC roc) {
  static auto the_log_{::pflib::logging::get("trim_toa_scan")};

  /// do a run of 100 samples per trim_toa to measure the TOA
  /// efficiency when looking at charge injection data. 
  /// effectively a 2D scan of calib and trim_toa
  /// maybe could reduce from 100 to 1 if you wanna go fast

  static const std::size_t n_events = 100;

  tgt->setup_run(1, Target::DaqFormat::SIMPLEROC, 1);

  std::array<int, 72> target; //trim_toa is a channel-wise parameter (1 value per channel)
  std::array<std::array<std::array<double, 72>, 8>, 200> final_data; // oh man this is probably jank

  DecodeAndBuffer buffer{n_events}; // working in buffer, not in writer

  // loop over runs, from trim_toa = 0 to 32 by skipping 4
  // loop over calib, from calib = 0 to 800 by skipping over 4

  for (int trim_toa{0}; trim_toa < 32; trim_toa += 4) {
    pflib_log(info) << "testing trim_toa = " << trim_toa;
    auto trim_toa_test_builder = roc.testParameters();
    for (int ch{0}; ch < 72; ch++) {
      trim_toa_test_builder.add("CH_"+std::to_string(ch), "TRIM_TOA", TRIM_TOA);
    }
    // set TRIM_TOA for each channel
    auto trim_toa_test = trim_toa_test_builder.apply();
    for (int calib = 0; calib < 800; calib += 4) {
      pflib_log(info) << "Running CALIB = " << calib;
      // set CALIB for each half
      auto calib_test = roc.testParameters()
        .add("REFERENCEVOLTAGE_0", "CALIB", calib)
        .add("REFERENCEVOLTAGE_1", "CALIB", calib)
      .apply();
      usleep(10);
      // not sure what should go in 4th parameter of next line?
      // the uncommented line is what I had for writer last time.
      // tgt->daq_run("CHARGE", buffer, n_events, 100);
      tgt->daq_run("CHARGE", buffer, n_events, pftool::state.daq_rate);

      pflib_log(trace) << "finished trim_toa = " << trim_toa << ", and calib = " << calib << ", getting efficiencies";
      auto efficiencies = pflib::algorithm::toa_vref_scan::get_toa_efficiencies(buffer.get_buffer());
      pflib_log(trace) << "got channel efficiencies, storing now";
      for (int ch{0}; ch < 72; ch++) {
        final_data[calib][trim_toa][ch] = efficiencies[ch];
      }
    }
  }

  // got data, now do analysis
  // want to make Siegel Linear Regression of trim_toa versus calib for each turn-on point.
  // the turn-on point (threshold point) is the first calib for which toa_efficiency is non-zero
  // for a given channel and trim_toa.


  // will come back and incorporate analysis algorithm. Need to make my own
  // linear regression, since it isn't in C++ natively (Siegel linear regression)
  // pflib_log(info) << "sample collections done, deducing settings";
  // for (int i_link{0}; i_link < 2; i_link++) {
  //   int highest_non_zero_eff = -1; // just a placeholder in case it's not found
  //   for (int toa_vref = final_effs[0].size(); toa_vref >= 0; toa_vref--) {
  //     if (final_effs[i_link][toa_vref] > 0.0) {
  //       highest_non_zero_eff = toa_vref + 10; // need to add 10 since we don't want to overlap with highest pedestals!
  //       break; // should break from link 0 into link 1
  //     }
  //   }
  //   target[i_link] = highest_non_zero_eff; //store value
  // }

  // std::map<std::string, std::map<std::string, int>> settings;
  // for (int i_link{0}; i_link < 2; i_link++) {
  //   std::string page{pflib::utility::string_format("REFERENCEVOLTAGE_%d", i_link)};
  //   settings[page]["TOA_VREF"] = target[i_link];
  // }

  return settings;
}

}