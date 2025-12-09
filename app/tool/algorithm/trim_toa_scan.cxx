#include "trim_toa_scan.h"

#include "../daq_run.h"
#include "../tasks/trim_toa_scan.h"
#include "get_toa_efficiencies.h"
#include "pflib/utility/median.h"
#include "pflib/utility/string_format.h"

/**
 * Perform a linear regression using the
 * [Theil-Sen
 * estimator](https://en.wikipedia.org/wiki/Theil%E2%80%93Sen_estimator) which
 * is more robust against outliers.
 *
 * We do the naive algorithm, calculating the slope of all pairs of sample
 * points and then finding the median of that slope. The final intercept is the
 * median of the intercepts from all points using the median slope.
 *
 * @param[in] x_vals list of x-coordinate samples
 * @param[in] y_vals list of y-coordinate samples
 * @return 2-tuple of the form (slope, intercept)
 */
std::tuple<double, double> siegel_regression(
    const std::vector<double>& x_vals, const std::vector<double>& y_vals) {
  if (x_vals.size() != y_vals.size()) {
    throw std::invalid_argument("x_vals and y_vals must be the same size.");
  }
  if (x_vals.empty()) {
    throw std::invalid_argument("Input vectors must not be empty.");
  }

  size_t n = x_vals.size();

  std::vector<double> slopes;
  slopes.reserve(n * n);
  for (size_t i = 0; i < n; ++i) {
    for (size_t j = i + 1; j < n; ++j) {
      /**
       * @note We are ignoring any pairs which would lead to division by zero.
       */
      if (x_vals[j] != x_vals[i]) {
        slopes.push_back((y_vals[j] - y_vals[i]) / (x_vals[j] - x_vals[i]));
      }
    }
  }

  double slope = pflib::utility::median(slopes);

  std::vector<double> intercepts;
  intercepts.reserve(n);
  for (size_t i = 0; i < n; i++) {
    intercepts.push_back(y_vals[i] - slope * x_vals[i]);
  }

  double intercept = pflib::utility::median(intercepts);
  return std::make_tuple(slope, intercept);
}

/**
 * Calculate the TRIM_TOA for each channel that best aligns all of them
 * to a common threshold voltage, charge injection pulse (calib).
 *
 * @note Not currently operational, but a good place to pickup from.
 */
namespace pflib::algorithm {

// Templated helpder function
template <class EventPacket>
static void trim_tof_runs(
    Target* tgt, ROC& roc, size_t n_events,
    std::array<std::array<std::array<double, 72>, 8>, 200>& final_data) {
  // working in buffer, not in writer
  DecodeAndBuffer<EventPacket> buffer{n_events, 2};
  static auto the_log_{::pflib::logging::get("toa_vref_scan")};

  // loop over trim_toa, from trim_toa = 0 to 32 by skipping 4
  // loop over calib, from calib = 0 to 800 by skipping over 4

  for (int trim_toa{0}; trim_toa < 32; trim_toa += 4) {
    pflib_log(info) << "testing trim_toa = " << trim_toa;
    auto trim_toa_test_builder = roc.testParameters();
    for (int ch{0}; ch < 72; ch++) {
      trim_toa_test_builder.add("CH_" + std::to_string(ch), "TRIM_TOA",
                                trim_toa);
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
      daq_run(tgt, "CHARGE", buffer, n_events, 100);

      pflib_log(trace) << "finished trim_toa = " << trim_toa
                       << ", and calib = " << calib << ", getting efficiencies";
      auto efficiencies = get_toa_efficiencies(buffer.get_buffer());
      pflib_log(trace) << "got channel efficiencies, storing now";
      for (int ch{0}; ch < 72; ch++) {
        // need to divide by 4 because index is value/4 from final_data
        // initialization and for loop
        final_data[calib / 4][trim_toa / 4][ch] = efficiencies[ch];
      }
    }
  }
}
std::map<std::string, std::map<std::string, uint64_t>> trim_toa_scan(
    Target* tgt, ROC roc) {
  static auto the_log_{::pflib::logging::get("trim_toa_scan")};

  /**
   * Charge injection scan (100 samples) while varying TRIM_TOA.
   * Purpose is to align TRIM_TOA for each channel.
   * Calculates TOA efficiency while looking at charge injection data.
   * Then uses Siegel Linear Regression to calculate the aligned
   * TRIM_TOA value for each channel to match a common "calib" value.
   *
   * @note Reduce the sample size (ex: 100 to 10) to decrease the scan time.
   */

  static const std::size_t n_events = 100;

  tgt->setup_run(1, pftool::state.daq_format_mode, 1);

  // trim_toa is a channel-wise parameter (1 value per channel)
  std::array<uint64_t, 72> target;

  // 72 channels, 200 calib values, 8 trim_toa values. Only store toa_efficiency
  // here.
  std::array<std::array<std::array<double, 72>, 8>, 200> final_data;

  if (pftool::state.daq_format_mode == Target::DaqFormat::SIMPLEROC) {
    trim_tof_runs<pflib::packing::SingleROCEventPacket>(tgt, roc, n_events,
                                                        final_data);
  } else if (pftool::state.daq_format_mode ==
             Target::DaqFormat::ECOND_SW_HEADERS) {
    trim_tof_runs<pflib::packing::MultiSampleECONDEventPacket>(
        tgt, roc, n_events, final_data);
  } else {
    pflib_log(warn) << "Unsupported DAQ format ("
                    << static_cast<int>(pftool::state.daq_format_mode)
                    << ") in level_pedestals. Skipping pedestal leveling...";
  }

  pflib_log(info) << "sample collections done, deducing settings";

  /**
   * Now that we have the data, we need to analyze it.
   *
   * We'll be looking for the turn-on (threshold) points for each channel
   * at each trim_toa value. The turn-on (threshold) point is the first
   * point where toa_efficiency goes from 0 to non-zero. The toa_efficiency
   * is simply the number of times TOA triggers divided by the sample size, for
   * a given trim_toa/calib/channel combination.
   *
   * We'll be using the Siegel Linear Regression because it's less sensitive to
   * outliers, since sometimes changing the trim_toa causes the threshold
   * (turn-on) points to "wrap around".
   */

  // "threshold_points" is a 2D vector.
  // Column 1 is channel index, Column 2 is calib, Column 3 is trim_toa.
  std::vector<std::vector<int>> threshold_points;

  // Each channel has multiple threshold points, but we don't know how many at
  // the start.

  for (int trim_toa{0}; trim_toa < 32; trim_toa += 4) {
    for (int ch{0}; ch < 72; ch++) {
      for (int calib{0}; calib < 800; calib += 4) {
        // divide by 4 to convert value to index
        if (final_data[calib / 4][trim_toa / 4][ch] > 0.0) {
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

    std::cout << "about to do linear regression" << std::endl;
    std::cout << "x_vals size: " << calib.size()
              << ", y_vals size: " << trim_toa.size() << std::endl;
    auto [slope, intercept] = siegel_regression(calib, trim_toa);
    std::cout << "did regression" << std::endl;
  }

  // now, write the settings, but this is just placeholder for now!

  std::map<std::string, std::map<std::string, uint64_t>> settings;

  std::array<int, 2> targetss = {0, 0};
  for (int i_link{0}; i_link < 2; i_link++) {
    std::string page{pflib::utility::string_format("CH_%d", i_link)};
    settings[page]["CALIB"] = targetss[i_link];
  }

  return settings;
}

}  // namespace pflib::algorithm
