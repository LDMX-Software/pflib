#include "pflib/algorithm/trim_toa_scan.h"
#include "pflib/algorithm/get_toa_efficiencies.h"

#include "pflib/utility/string_format.h"

#include "pflib/DecodeAndBuffer.h"

// probably don't need these, but just double checking
#include <algorithm>
#include <cmath>


// Here's the regression. Main idea is that it finds the median slope from all the pairs of points in the dataset.
// Similarly, does the same for the intercept using the median slope. Supposedly less sensitive to outliers than the standard linear regression.
// Got this function straight from Copilot. Maybe we should put it in algorithm or utility? Not sure if we'll use linear regression for 
// TOT and stuff like that...
void siegelRegression(const std::vector<double>& xVals, const std::vector<double>& yVals, double& slope, double& intercept) {
  if (xVals.size() != yVals.size()) {
    throw std::invalid_argument("xVals and yVals must be the same size.");
  }
  if (xVals.empty()) {
    throw std::invalid_argument("Input vectors must not be empty.");
  }
  
  size_t n = xVals.size();
  std::vector<double> slopes;

  for (size_t i = 0; i < n; ++i) {
    for (size_t j = i + 1; j < n; ++j) {
      if (xVals[j] != xVals[i]) {
        double m = (yVals[j] - yVals[i]) / (xVals[j] - xVals[i]);
        slopes.push_back(m);
      }
    }
  }

  slope = median(slopes);

  std::vector<double> intercepts;
  for (size_t i = 0; i < n; i++) {
    intercepts.push_back(yVals[i] - slope * xVals[i]);
  }

  intercept = median(intercepts);
}

// end vibe coding. back to what we had before

/**
 * Calculate the TRIM_TOA for each channel that best aligns all of them
 * to a common threshold voltage, charge injection pulse (calib).
 */
namespace pflib::algorithm {

std::map<std::string, std::map<std::string, int>>
trim_toa_scan(Target* tgt, ROC roc) {
  static auto the_log_{::pflib::logging::get("trim_toa_scan")};

  /// Charge injection scan (100 samples) while varying TRIM_TOA.
  /// Purpose is to align TRIM_TOA for each channel.
  /// Calculates TOA efficiency while looking at charge injection data. 
  /// Then uses Siegel Linear Regression to calculate the aligned
  /// TRIM_TOA value for each channel to match a common "calib" value.
  /// Note: Reduce the sample size (ex: 100 to 10) to decrease the scan time.

  static const std::size_t n_events = 100;

  tgt->setup_run(1, Target::DaqFormat::SIMPLEROC, 1);

  std::array<int, 72> target; //trim_toa is a channel-wise parameter (1 value per channel)
  std::array<std::array<std::array<double, 72>, 8>, 200> final_data; // 72 channels, 200 calib values, 8 trim_toa values. Only store toa_efficiency here.

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
    usleep(10);
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
      // std::cout << "buffer size: " << buffer.get_buffer().size() << std::endl; // debug
      auto efficiencies = get_toa_efficiencies(buffer.get_buffer());
      pflib_log(trace) << "got channel efficiencies, storing now";
      for (int ch{0}; ch < 72; ch++) {
        final_data[calib/4][trim_toa/4][ch] = efficiencies[ch]; // need to divide by 4 because index is value/4 from final_data initialization
        // std::cout << "calib=" << calib << ", trim_toa=" << trim_toa << ", ch=" << ch<< ", eff=" << efficiencies[ch] << std::endl; //debug
      }
    }
  }

  pflib_log(info) << "sample collections done, deducing settings";

  // Now that we have the data, we need to analyze it.
  // We'll be looking for the turn-on (threshold) points for each channel 
  // at each trim_toa value. The turn-on (threshold) point is the first
  // point where toa_efficiency goes from 0 to non-zero. The toa_efficiency
  // is simply the number of times TOA triggers divided by the sample size, for
  // a given trim_toa/calib/channel combination.

  // We'll be using the Siegel Linear Regression because it's less sensitive to
  // outliers, since sometimes changing the trim_toa causes the threshold (turn-on)
  // points to "wrap around".

  std::vector<std::vector<int>> threshold_points;

  // "threshold_points" is a 2D vector. Column 1 is channel index, Column 2 is calib, Column 3 is trim_toa.
  // Each channel has multiple threshold points, but we don't know how many at the start.

  for (int trim_toa{0}; trim_toa < 32; trim_toa += 4) {
    for (int ch{0}; ch < 72; ch++) {
      for (int calib{0}; calib < 800; calib += 4) {
        if (final_data[calib/4][trim_toa/4][ch] > 0.0) { // again, needing to divide by 4.
          threshold_points.push_back({ch, calib, trim_toa});
          break;
        }
      }
    }
  }

  pflib_log(info) << "got threshold points, getting fit parameters";

  // get vector of data points for each channel.
  for (int ch{0}; ch < 72; ch++) {
    std::vector<double> calib;
    std::vector<double> trim_toa;

    for (const auto& row : threshold_points) {
      if (row[0] == ch) {
        calib.push_back(row[1]);
        trim_toa.push_back(row[2]);
      }
    }

    if (calib.empty() || trim_toa.empty()) {
      std::cout << "skipping channel " << ch << " due to empty data.\n";
      continue;
    }

    double slope, intercept;
    std::cout << "about to do linear regression" << std::endl;
    std::cout << "xVals size: " << calib.size() << ", yVals size: " << trim_toa.size() << std::endl;
    siegelRegression(calib, trim_toa, slope, intercept);
    std::cout << "did regression" << std::endl;
  }

  // now, write the settings, but this is just placeholder for now!

  std::map<std::string, std::map<std::string, int>> settings;
  std::array<int, 2> targetss = {0,0};
  for (int i_link{0}; i_link < 2; i_link++) {
    std::string page{pflib::utility::string_format("CH_%d", i_link)};
    settings[page]["CALIB"] = targetss[i_link];
  }

  return settings;
}

}
