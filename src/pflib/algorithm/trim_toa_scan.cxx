#include "pflib/algorithm/trim_toa_scan.h"
#include "pflib/algorithm/toa_vref_scan.h"

#include "pflib/utility/string_format.h"
#include "pflib/utility/efficiency.h"

#include "pflib/DecodeAndBuffer.h"

/**
 * calculate the TRIM_TOA for each channel that aligns them to a common trigger pulse.
 */

 // re-using this code from pflib/algorithm/toa_vref_scan.cxx
 // should put it into its own function if I keep it here.
static std::array<double, 72> get_toa_efficiencies(const std::vector<pflib::packing::SingleROCEventPacket> &data) {
  std::array<double, 72> efficiencies;
  /// reserve a vector of the appropriate size to avoid repeating allocation time for all 72 channels
  std::vector<int> toas(data.size());
  for (int ch{0}; ch < 72; ch++) {
    for (std::size_t i{0}; i < toas.size(); i++) {
      toas[i] = data[i].channel(ch).toa();
    }
    /// we assume that the data provided is not empty otherwise the efficiency calculation is meaningless
    efficiencies[ch] = pflib::utility::efficiency(toas);
  }
  return efficiencies;
}

namespace pflib::algorithm {

std::map<std::string, std::map<std::string, int>>
trim_toa_scan(Target* tgt, ROC roc) {
  static auto the_log_{::pflib::logging::get("trim_toa_scan")};

  /// do a run of 100 samples per trim_toa to measure the TOA
  /// efficiency when looking at charge injection data. 
  /// effectively a 2D scan of calib and trim_toa
  /// maybe could reduce from 100 to 1 if you wanna go fast

  // static const std::size_t n_events = 100;
  static const std::size_t n_events = 5; // just for speed for testing purposes.

  tgt->setup_run(1, Target::DaqFormat::SIMPLEROC, 1);

  std::array<int, 72> target; //trim_toa is a channel-wise parameter (1 value per channel)
  std::array<std::array<std::array<double, 72>, 8>, 200> final_data; // oh man this is probably jank. 72 channels, 200 calib values, 8 trim_toa values. Only store toa_efficiency here.

  DecodeAndBuffer buffer{n_events}; // working in buffer, not in writer

  // loop over trim_toa, from trim_toa = 0 to 32 by skipping 4
  // loop over calib, from calib = 0 to 800 by skipping over 4

  for (int trim_toa{0}; trim_toa < 32; trim_toa += 4) {
    pflib_log(info) << "testing trim_toa = " << trim_toa;
    auto trim_toa_test_builder = roc.testParameters();
    for (int ch{0}; ch < 72; ch++) {
      trim_toa_test_builder.add("CH_"+std::to_string(ch), "TRIM_TOA", trim_toa);
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
      tgt->daq_run("CHARGE", buffer, n_events, 100);

      pflib_log(trace) << "finished trim_toa = " << trim_toa << ", and calib = " << calib << ", getting efficiencies";
      auto efficiencies = get_toa_efficiencies(buffer.get_buffer());
      pflib_log(trace) << "got channel efficiencies, storing now";
      for (int ch{0}; ch < 72; ch++) {
        final_data[calib/4][trim_toa/4][ch] = efficiencies[ch];
      }
    }
  }

  // got data, now do analysis
  // want to make Siegel Linear Regression of trim_toa versus calib for each turn-on point.
  // the turn-on point (threshold point) is the first calib for which toa_efficiency is non-zero
  // for a given channel and trim_toa.

  // However, I'm not sure how to do the Siegel linear regression without just building it
  // outright. So, what we're going to do instead is the following: select only threshold 
  // points that are "lower" than the previous threshold point, excluding the first one.
  // That way, we are less likely to run into a problem with outliers where the next point
  // is way higher than the previous point.

  // will come back and incorporate analysis algorithm. Need to make my own
  // linear regression, since it isn't in C++ natively (Siegel linear regression)

  pflib_log(info) << "sample collections done, deducing settings";

  // first, get the threshold points
  // for each channel and at each trim_toa, count up CALIB from the bottom until you get a non-zero efficiency

  // storage for threshold values
  // 3D vector? 1 for channel, 2 for calib/trim_toa pairs that are thresholds

  // std::vector<std::vector<std::vector<int, 72>, 0>, 0> threshold_points;
  std::vector<std::vector<int>> threshold_points;
  // "threshold_points" is 2D vector. Column 1 is channel index, Column 2 is trim_toa, Column 3 is calib.
  // This way, since each channel will have multiple threshold points! We'll add on rows to the bottom of the
  // vector using the "push_back()"" method.

  // for each trim, look at the calib value at which the specific channel had a threshold point!
  // store the first one, then only store the next one if it's lower.
  int trim_toa = 0; // just do first one.
  for (int ch{0}; ch < 72; ch++) {
    for (int calib{0}; calib < 800; calib += 4) {
      if (final_data[calib/4][trim_toa/4][ch] > 0.0) {
        threshold_points.push_back({ch, trim_toa, calib});
        break;
      }
    }
  }

  // now look at others!
  for (int trim_toa{4}; trim_toa < 32; trim_toa += 4) {
    for (int ch{0}; ch < 72; ch++) {
      for (int calib{0}; calib < 800; calib += 4) {
        if (final_data[calib/4][trim_toa/4][ch] > 0.0) { // count up from the bottom to first non-zero efficiency
          while (final_data[calib/4][trim_toa/4 - 1][ch]) { // only store if previous one is greater
            if (final_data[calib/4][trim_toa/4 - 1][ch] > final_data[calib/4][trim_toa/4][ch]) {
              threshold_points.push_back({ch, trim_toa, calib});
              break;
            }
          }
        }
      }
    }
  }

  // now, we have the threshold points; let's do linear regression on them.

  // linear regression goes here, but don't have it yet.

  // now, write the settings, but this is just placeholder for now!

  std::map<std::string, std::map<std::string, int>> settings;
  std::array<int, 2> targetss = {1, 2};
  for (int i_link{0}; i_link < 2; i_link++) {
    std::string page{pflib::utility::string_format("REFERENCEVOLTAGE_%d", i_link)};
    settings[page]["TOA_VREF"] = targetss[i_link];
  }

  return settings;
}

}