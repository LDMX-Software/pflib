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

// doing some vibe coding here ~ just took straight from copilot

struct Point {
  double calib, trim_toa;
};

double median(std::vector<double>& values) {
  std::sort(values.begin(), values.end());
  size_t n = values.size();
  if (n % 2 == 0) {
    return (values[n/2 - 1] + values[n/2]) / 2.0;
  }
  else {
    return values [n/2];
  }
}

void siegelRegression(const std::vector<double>& xVals, const std::vector<double>& yVals, double& slope, double& intercept) {
  if (xVals.size() != yVals.size()) {
    throw std::invalid_argument("xVals and yVals must be the same size.");
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

// end vibe coding

namespace pflib::algorithm {

std::map<std::string, std::map<std::string, int>>
trim_toa_scan(Target* tgt, ROC roc) {
  static auto the_log_{::pflib::logging::get("trim_toa_scan")};

  /// do a run of 100 samples per trim_toa to measure the TOA
  /// efficiency when looking at charge injection data. 
  /// effectively a 2D scan of calib and trim_toa
  /// maybe could reduce from 100 to 1 if you wanna go fast

  // static const std::size_t n_events = 100;
  static const std::size_t n_events = 2; // just for speed for testing purposes.

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
  // vector using the "push_back()" method.

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

  // and in case it works, let's do another one where it's all the threshold points!
  std::vector<std::vector<int>> threshold_points_all;
  for (int trim_toa{0}; trim_toa < 32; trim_toa += 4) {
    for (int ch{0}; ch < 72; ch++) {
      for (int calib{0}; calib < 800; calib += 4) {
        if (final_data[calib/4][trim_toa/4][ch] > 0.0) {
          threshold_points_all.push_back({ch, trim_toa, calib});
          break;
        }
      }
    }
  }

  // now, we have the threshold points; let's do linear regression on them.
  pflib_log(info) << "got threshold points, getting fit parameters";

  // doing Siegel linear regression, we need the median slope of all the slopes
  // between each pair of points!

  // get vector of data points for each channel.
  for (int ch{0}; ch < 72; ch++) {
    std::vector<std::vector<int>> channel_vector; // reset for each channel

    for (const auto& row : threshold_points_all) {
      if (!row.empty() && row[0] == ch) {
        auto copy_of_row = row;
        copy_of_row.erase(copy_of_row.begin());
        channel_vector.push_back(copy_of_row); // adds the row to the channel_vector, but missing channel!
      }
    }
    std::cout << "the channel " << ch << " has row: " << std::endl;
    for (const auto& row : channel_vector) {
      for (const auto& value : row) {
        std::cout << value << " "; // just printing the row values so we can see.
      }
      std::cout << std::endl;
    }
    // now that we have the channel_vector, we could remove the "channel" column, but we
    // really just need the second and third columns. I already got rid of channel earlier.
    double slope, intercept;
    // std::vector<Point> data = channel_vector;
    std::vector<Point> data;
    // siegelRegression(data, slope, intercept);
    // std::cout << "channel " << ch << ": trim_toa = " << slope << "calib + " << intercept << std::endl;
  }

  // more vibe coding here ~ also copilot
  std::vector<Point> data = {{1, 2}, {2, 3}, {3, 5}, {4, 4}, {5, 6}};
  double slope, intercept;

  // siegelRegression(data, slope, intercept);

  std::cout << "Siegel Regression Line: trim_toa = " << slope << "calib + " << intercept << std::endl;
  // end vibe coding here

  // need to get each channel from the threshold_points

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