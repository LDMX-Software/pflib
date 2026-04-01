#include "inv_vref_lund.h"

#include "../daq_run.h"
#include "../tasks/inv_vref_scan_lund.h"
#include "pflib/utility/mean.h"
#include "pflib/utility/median.h"
#include "pflib/utility/stdev.h"
#include "pflib/utility/string_format.h"

namespace pflib::algorithm {

DataFitter::DataFitter() {};

void DataFitter::sort_and_append(std::vector<int>& inv_vrefs,
                                 std::vector<int>& pedestals,
                                 std::vector<double>& stdevs, int& step) {
  static auto the_log_{::pflib::logging::get("inv_vref_scan:sort")};
  pflib_log(info) << "Sorting Data";
  // We ignore first and last elements since they miss derivs
  // Guard against empty or too-small vectors
  if (inv_vrefs.size() < 3) {
    pflib_log(error) << "Not enough data points to sort ("
                     << inv_vrefs.size() << "). Need at least 3.";
    return;
  }

  struct DerivPoint {
    int i;
    double LH;
    double LH_stdev;
    double RH;
    double RH_stdev;
  };
  std::vector<DerivPoint> slope_points;

  double flat_threshold = 0.1;
  std::vector<double> LH_derivs;
  std::vector<double> LH_stdevs;
  std::vector<double> RH_derivs;
  for (int i = 1; i < inv_vrefs.size() - 1; i++) {
    double LH = static_cast<double>(pedestals[i] - pedestals[i - 1]) /
                (inv_vrefs[i] - inv_vrefs[i - 1]);
    double RH = static_cast<double>(pedestals[i] - pedestals[i + 1]) /
                (inv_vrefs[i] - inv_vrefs[i + 1]);

    // Threshold check. CMS uses 0.05. This value fits with my analysis as well.
    if (std::abs(LH) < flat_threshold ||
        std::abs(RH) < flat_threshold) {  // flat regime
      nonlinear_.push_back({inv_vrefs[i], pedestals[i], LH, RH});
    } else {  // we're in a linear regime or there's outliers
      double LH_err =
          (stdevs[i] - stdevs[i + 1]) / (inv_vrefs[i] - inv_vrefs[i - 1]);
      double RH_err =
          (stdevs[i] - stdevs[i + 1]) / (inv_vrefs[i] - inv_vrefs[i + 1]);
      slope_points.push_back({i, LH, LH_err, RH, RH_err});
      LH_derivs.push_back(LH);
      LH_stdevs.push_back(LH_err);
      RH_derivs.push_back(RH);
    }
  }
  // Now we get the linear region. CMS removes outliers by using the ADC median.
  // From my analysis I chose to also consider the stdev of each point, which is
  // 0 in the low inv_vref region (inv_vref approx less than 250). NOTE! This
  // seems to vary between boards, and more analysis between boards should be
  // done to find the optimal selections.
  //
  // We could use both LH and RH derivs to improve selections (?).

  LH_std_median_ = pflib::utility::median(LH_stdevs);
  LH_median_ = pflib::utility::median(LH_derivs);

  pflib_log(info) << "Median stdev = " << LH_std_median_;
  pflib_log(info) << "Median LH = " << LH_median_;

  for (const auto& p : slope_points) {
    if ((std::abs(p.LH - LH_median_) < 0.7 * std::abs(LH_median_)) &&
        std::abs(p.LH_stdev) > 0.0001) {  // Linear regime.
      pflib_log(info) << "inv_vref is : " << inv_vrefs[p.i];
      linear_.push_back({inv_vrefs[p.i], pedestals[p.i], p.LH, p.RH});
    }
  }
}

int DataFitter::fit(int target) {
  static auto the_log_{::pflib::logging::get("inv_vref_scan:fit")};
  pflib_log(info) << "Fitting Data";

  if (linear_.empty()) {
    pflib_log(error) << "No linear region found - cannot fit.";
    return -1;
  }

  std::vector<double> intercepts;
  std::vector<double> slopes;
  for (const auto& p : linear_) {
    pflib_log(info) << "linear data:";
    pflib_log(info) << "LH deriv = " << p.LH_ << " at inv_vref = " << p.x_;
    double b = p.y_ - p.LH_ * p.x_;
    intercepts.push_back(b);
    slopes.push_back(p.LH_);
  }
  double median_intercept = pflib::utility::median(intercepts);
  double median_slope = pflib::utility::median(slopes);

  // Guard against zerp slope
  if (std::abs(median_slope) < 0.001) {
    pflib_log(error) << "Median slope is near zero — fit is unreliable.";
    return -1;
  }

  pflib_log(info) << "The median intercept is = " << median_intercept
                  << " and the median deriv is = " << median_slope;

  int inv_vref = 0;
  int adc = 0;
  while (inv_vref < 1024) {
    adc = median_slope * inv_vref + median_intercept;
    if (adc <= target) {
      break;
    }
    inv_vref++;
  }

  // Guard against hitting the upper bound
  if (inv_vref >= 1024) {
    pflib_log(error) << "Fit did not converge within range.";
    return -1;
  }

  pflib_log(info) << "Final inv_vref is " << inv_vref;
  return inv_vref;
}

// helper function to facilitate EventPacket dependent behaviour
template <class EventPacket>
static void inv_vref_scan_getter(Target* tgt, pflib::ROC& roc, size_t nevents,
                                 std::array<int, 2>& channels,
                                 std::array<int, 2>& inv_vref_tgt,
                                 std::array<int, 2>& noinv_vref_tgt) {
  static auto the_log_{::pflib::logging::get("inv_vref_scan:getter")};

  int noinv_vref = 612;
  int target_adc = 200;

  DecodeAndBuffer<EventPacket> buffer{1, 2};

  tgt->setup_run(1 /* dummy - not stored */, pftool::state.daq_format_mode,
                 1 /* dummy */);

  std::array<std::vector<int>, 2> fallbacks = {{std::vector<int>{17, 15, 19, 13}, std::vector<int>{17, 15, 19, 13}}};
  int test_inv_vref = 500;
  // check for bad channels that would be bad for fitting/dead
  auto test_param = roc.testParameters()
                        .add("REFERENCEVOLTAGE_0", "INV_VREF", test_inv_vref)
                        .add("REFERENCEVOLTAGE_1", "INV_VREF", test_inv_vref)
                        .add("REFERENCEVOLTAGE_0", "NOINV_VREF", noinv_vref)
                        .add("REFERENCEVOLTAGE_1", "NOINV_VREF", noinv_vref)
                        .apply();
  daq_run(tgt, "PEDESTAL", buffer, nevents, pftool::state.daq_rate);
  auto test_data = buffer.get_buffer();

  for (int link = 0; link < 2; link++) {
    bool found_good = false;
    for (int candidate : fallbacks[link]) {
      std::vector<int> test_adcs;
      for (std::size_t i = 0; i < test_data.size(); i++) {
        if constexpr (std::is_same_v<EventPacket,
                          pflib::packing::MultiSampleECONDEventPacket>) {
          test_adcs.push_back(
              test_data[i].samples[test_data[i].i_soi].channel(link, candidate).adc());
        } else if constexpr (std::is_same_v<EventPacket,
                                pflib::packing::SingleROCEventPacket>) {
          test_adcs.push_back(test_data[i].channel(candidate).adc());
        }
      }
      double s = pflib::utility::stdev(test_adcs);
      if (s > 0.001 && s < 50.0) {
        channels[link] = candidate;
        pflib_log(info) << "Link " << link << " using channel " << candidate
                        << " (stdev=" << s << ")";
        found_good = true;
        break;
      }
      pflib_log(warn) << "Link " << link << " channel " << candidate
                      << " looks bad (stdev=" << s << "), trying next...";
    }
    if (!found_good) {
      pflib_log(error) << "Link " << link << " has no good channels!";
      return;
    }
  }

  std::vector<int> pedestals_l0;
  std::vector<double> stds_l0;
  std::vector<int> pedestals_l1;
  std::vector<double> stds_l1;
  std::vector<int> inv_vrefs;

  int step = 20;

  for (int inv_vref = 0; inv_vref < 1024; inv_vref += step) {
    pflib_log(info) << "Running INV_VREF = " << inv_vref;
    // set inv_vref simultaneously for both links
    auto test_param = roc.testParameters()
                          .add("REFERENCEVOLTAGE_0", "INV_VREF", inv_vref)
                          .add("REFERENCEVOLTAGE_1", "INV_VREF", inv_vref)
                          .add("REFERENCEVOLTAGE_0", "NOINV_VREF", noinv_vref)
                          .add("REFERENCEVOLTAGE_1", "NOINV_VREF", noinv_vref)
                          .apply();
    // store current scan state in header for writer access
    daq_run(tgt, "PEDESTAL", buffer, nevents, pftool::state.daq_rate);

    auto data = buffer.get_buffer();

    std::vector<int> adcs_l0;
    std::vector<int> adcs_l1;

    for (std::size_t i{0}; i < data.size(); i++) {
      if constexpr (std::is_same_v<
                        EventPacket,
                        pflib::packing::MultiSampleECONDEventPacket>) {
        adcs_l0.push_back(
            data[i].samples[data[i].i_soi].channel(0, channels[0]).adc());
        adcs_l1.push_back(
            data[i].samples[data[i].i_soi].channel(1, channels[1]).adc());
      } else if constexpr (std::is_same_v<
                               EventPacket,
                               pflib::packing::SingleROCEventPacket>) {
        adcs_l0.push_back(data[i].channel(channels[0]).adc());
        adcs_l1.push_back(data[i].channel(channels[1]).adc());
      } else {
        PFEXCEPTION_RAISE("BadConf",
                          "Unable to get adc for the cofigured format");
      }
    }
    pedestals_l0.push_back(pflib::utility::median(adcs_l0));
    stds_l0.push_back(pflib::utility::stdev(adcs_l0));
    pedestals_l1.push_back(pflib::utility::median(adcs_l1));
    stds_l1.push_back(pflib::utility::stdev(adcs_l1));
    inv_vrefs.push_back(inv_vref);
  }
  // sort data and fit
  DataFitter fitter_l0;
  DataFitter fitter_l1;
  fitter_l0.sort_and_append(inv_vrefs, pedestals_l0, stds_l0, step);
  fitter_l1.sort_and_append(inv_vrefs, pedestals_l1, stds_l1, step);
  inv_vref_tgt[0] = fitter_l0.fit(target_adc);
  inv_vref_tgt[1] = fitter_l1.fit(target_adc);
  noinv_vref_tgt[0] = noinv_vref;
  noinv_vref_tgt[1] = noinv_vref;
}

std::map<std::string, std::map<std::string, uint64_t>> inv_vref_lund(
    Target* tgt, ROC& roc) {
  static auto the_log_{::pflib::logging::get("inv_vref_scan")};
  int nevents = pftool::readline_int("Number of events per point: ", 1);
  std::array<int, 2> channels = {17, 51};

  std::array<int, 2> inv_vref;
  std::array<int, 2> noinv_vref;

  if (pftool::state.daq_format_mode == Target::DaqFormat::SIMPLEROC) {
    inv_vref_scan_getter<pflib::packing::SingleROCEventPacket>(
        tgt, roc, nevents, channels, inv_vref, noinv_vref);
  } else if (pftool::state.daq_format_mode ==
             Target::DaqFormat::ECOND_SW_HEADERS) {
    inv_vref_scan_getter<pflib::packing::MultiSampleECONDEventPacket>(
        tgt, roc, nevents, channels, inv_vref, noinv_vref);
  } else {
    pflib_log(warn) << "Unsupported DAQ format ("
                    << static_cast<int>(pftool::state.daq_format_mode)
                    << ") in level_pedestals. Skipping pedestal leveling...";
  }
  std::map<std::string, std::map<std::string, uint64_t>> settings;
  for (int i_link{0}; i_link < 2; i_link++) {
    auto refvol_page =
        pflib::utility::string_format("REFERENCEVOLTAGE_%d", i_link);
    settings[refvol_page]["INV_VREF"] = inv_vref[i_link];
    settings[refvol_page]["NOINV_VREF"] = noinv_vref[i_link];
  }
  return settings;
}

}  // namespace pflib::algorithm
