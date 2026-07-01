#include "global_pedestal_level.h"

#include "../daq_run.h"
#include "../tasks/global_pedestal_level.h"
#include "pflib/utility/mean.h"
#include "pflib/utility/median.h"
#include "pflib/utility/stdev.h"
#include "pflib/utility/string_format.h"

namespace pflib::algorithm {

DataFitter::DataFitter() {};

void DataFitter::sort_and_append(std::vector<int>& inv_vrefs,
                                 std::vector<int>& pedestals,
                                 std::vector<double>& stdevs, int& step) {
  static auto the_log_{::pflib::logging::get("global_pedestal_level:sort")};
  pflib_log(info) << "Sorting Data";
  // We ignore first and last elements since they miss derivs
  struct DerivPoint {
    int i;
    double LH;
    double LH_stdev;
    double RH;
    double RH_stdev;
  };
  std::vector<DerivPoint> slope_points;

  double flat_threshold = 0.1;
  double linear_threshold = 5;
  std::vector<double> LH_derivs;
  std::vector<double> LH_stdevs;
  std::vector<double> RH_derivs;
  for (int i = 1; i < inv_vrefs.size() - 1; i++) {
    double LH = static_cast<double>(pedestals[i] - pedestals[i - 1]) /
                (inv_vrefs[i] - inv_vrefs[i - 1]);
    double RH = static_cast<double>(pedestals[i] - pedestals[i + 1]) /
                (inv_vrefs[i] - inv_vrefs[i + 1]);

    // Threshold check. CMS uses 0.05. This value fits with my analysis as well.
    if (std::abs(LH) <= flat_threshold || std::abs(RH) <= flat_threshold ||
        std::abs(LH) >= linear_threshold ||
        std::abs(RH) >= linear_threshold) {  // flat regime
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
  static auto the_log_{::pflib::logging::get("global_pedestal_level:fit")};
  pflib_log(info) << "Fitting Data";
  // Calculate the median intercept and slope
  pflib_log(info) << linear_.size();

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
  pflib_log(info) << "The median intercept is = " << median_intercept
                  << " and the median slope is = " << median_slope;

  // Are the intercept and slope reasonable?
  // TODO: Add condition for intercept!
  if ((median_slope >= 0) || (median_slope <= -1e3)) {
    return -1;
  }

  // Find intersect with target. Start at the beginning of the linear regime
  int inv_vref = 0;
  int adc = 0;
  int n = linear_.size();
  while (inv_vref < 1024) {
    adc = median_slope * inv_vref + median_intercept;
    // pflib_log(info) << "inv_vref = " << inv_vref << " with adc = " << adc;
    if (adc <= target) {
      break;
    }
    inv_vref++;
  }
  inv_vref = inv_vref;
  pflib_log(info) << "Final inv_vref is " << inv_vref;
  return inv_vref;
}

int DataFitter::linear_fit(int& target) {
  static auto the_log_{
      ::pflib::logging::get("global_pedestal_level:linear_fit")};
  pflib_log(info) << "Fitting Data";

  // preform a linear fit
  double x_sum = 0;
  double y_sum = 0;

  for (const auto& p : linear_) {
    x_sum += p.x_;
    y_sum += p.y_;
  }

  double x_mean = x_sum / linear_.size();
  double y_mean = y_sum / linear_.size();
  double s_x = 0.0;
  double s_y = 0.0;
  double s_xy = 0.0;

  for (const auto& p : linear_) {
    s_x += (p.x_ - x_mean) * (p.x_ - x_mean);
    s_y += (p.y_ - y_mean) * (p.y_ - y_mean);
    s_xy += (p.x_ - x_mean) * (p.y_ - y_mean);
    pflib_log(info) << "Included inv_vref: " << p.x_;
  }

  double slope = s_xy / s_x;
  double intercept = y_mean - slope * x_mean;

  // determine if slope is reasonable
  if ((slope >= 0) || (slope <= -1e3)) {
    return -1;
  }

  // find target inv_vref
  int inv_vref = 0;
  int adc = 0;
  while (inv_vref < 1024) {
    adc = slope * inv_vref + intercept;
    if (adc <= target) {
      break;
    }
    inv_vref++;
  }

  pflib_log(info) << "Final inv_vref is " << inv_vref;
  return inv_vref;
}

void get_param(Target* tgt, DecodeAndBuffer& buffer, const std::size_t& nevents,
               int& step, std::map<int, std::vector<int>>& pedestals_l0,
               std::map<int, std::vector<double>>& stds_l0,
               std::map<int, std::vector<int>>& pedestals_l1,
               std::map<int, std::vector<double>>& stds_l1,
               std::map<int, std::vector<int>>& inv_vrefs,
               std::map<int, std::array<int, 2>>& noinv_vref) {
  static auto the_log_{
      ::pflib::logging::get("global_pedestal_level:get_param")};
  std::array<int, 2> channels = {17, 51};

  // clear variables
  pedestals_l0.clear();
  stds_l0.clear();
  pedestals_l1.clear();
  stds_l1.clear();
  inv_vrefs.clear();

  for (int inv_vref = 0; inv_vref < 1024; inv_vref += step) {
    pflib_log(info) << "Running INV_VREF = " << inv_vref;
    // set inv_vref simultaneously for both links
    std::map<int, std::map<std::string, std::map<std::string, uint64_t>>>
        parameters;
    for (int i_roc : tgt->roc_ids()) {
      parameters[i_roc]["REFERENCEVOLTAGE_0"]["INV_VREF"] = inv_vref;
      parameters[i_roc]["REFERENCEVOLTAGE_1"]["INV_VREF"] = inv_vref;
      parameters[i_roc]["REFERENCEVOLTAGE_0"]["NOINV_VREF"] =
          noinv_vref[i_roc][0];
      parameters[i_roc]["REFERENCEVOLTAGE_1"]["NOINV_VREF"] =
          noinv_vref[i_roc][1];
    }
    auto test_params = tgt->tempApplyAllROCs(parameters);

    // store current scan state in header for writer access
    daq_run(tgt, "PEDESTAL", buffer, nevents, pftool::state.daq_rate);

    auto data = buffer.get_buffer();

    std::map<int, std::vector<int>> adcs_l0;
    std::map<int, std::vector<int>> adcs_l1;
    for (int i_roc : tgt->roc_ids()) {
      const pflib::packing::SingleECONDRocErxMapping& mapping =
          tgt->getRocErxMapping();
      auto [i_erx_0, i_ch_0] = mapping.toErxChannel(i_roc, channels[0]);
      auto [i_erx_1, i_ch_1] = mapping.toErxChannel(i_roc, channels[1]);
      for (std::size_t i{0}; i < data.size(); i++) {
        adcs_l0[i_roc].push_back(data[i].soi().channel(i_erx_0, i_ch_0).adc());
        adcs_l1[i_roc].push_back(data[i].soi().channel(i_erx_1, i_ch_1).adc());
      }
      pedestals_l0[i_roc].push_back(pflib::utility::median(adcs_l0[i_roc]));
      stds_l0[i_roc].push_back(pflib::utility::stdev(adcs_l0[i_roc]));
      pedestals_l1[i_roc].push_back(pflib::utility::median(adcs_l1[i_roc]));
      stds_l1[i_roc].push_back(pflib::utility::stdev(adcs_l1[i_roc]));
      inv_vrefs[i_roc].push_back(inv_vref);
    }
  }
}

std::map<int, std::map<std::string, std::map<std::string, uint64_t>>>
global_pedestal_level(Target* tgt) {
  static auto the_log_{::pflib::logging::get("global_pedestal_level")};
  static const std::size_t nevents =
      pftool::readline_int("Number of events per point: ", 100);
  int noinv_vref_step = pftool::readline_int("Stepsize for noinv_vref: ", 20);
  // TODO 348

  std::map<int, std::array<int, 2>> inv_vref_tgt;
  std::map<int, std::array<int, 2>> noinv_vref_tgt;
  std::map<int, std::array<bool, 2>> found_parameters;

  for (int i_roc : tgt->roc_ids()) {
    for (int i_link = 0; i_link < 2; i_link++) {
      noinv_vref_tgt[i_roc][i_link] = 612;
      found_parameters[i_roc][i_link] = false;
    }
  }

  int target_adc = 200;

  DecodeAndBuffer buffer{nevents, tgt->nrocs() * 2};

  tgt->setup_run(1, Target::DaqFormat::ECOND_SW_HEADERS, 1);

  std::map<int, std::vector<int>> pedestals_l0;
  std::map<int, std::vector<double>> stds_l0;
  std::map<int, std::vector<int>> pedestals_l1;
  std::map<int, std::vector<double>> stds_l1;
  std::map<int, std::vector<int>> inv_vrefs;
  int step = 20;
  bool keep_running = true;
  while (keep_running) {
    keep_running = false;
    get_param(tgt, buffer, nevents, step, pedestals_l0, stds_l0, pedestals_l1,
              stds_l1, inv_vrefs, noinv_vref_tgt);

    // sort data and fit
    for (int i_roc : tgt->roc_ids()) {
      DataFitter fitter_l0;
      DataFitter fitter_l1;
      fitter_l0.sort_and_append(inv_vrefs[i_roc], pedestals_l0[i_roc],
                                stds_l0[i_roc], step);
      fitter_l1.sort_and_append(inv_vrefs[i_roc], pedestals_l1[i_roc],
                                stds_l1[i_roc], step);

      if (found_parameters[i_roc][0] == false) {
        inv_vref_tgt[i_roc][0] = fitter_l0.linear_fit(target_adc);
      }
      if (found_parameters[i_roc][1] == false) {
        inv_vref_tgt[i_roc][1] = fitter_l1.linear_fit(target_adc);
      }
    }

    for (int i_roc : tgt->roc_ids()) {
      found_parameters[i_roc][0] = true;
      found_parameters[i_roc][1] = true;
    }

    for (int i_roc : tgt->roc_ids()) {
      // Checking if the fit is reasonable and within boundaries
      if (inv_vref_tgt[i_roc][0] == -1) {
        noinv_vref_tgt[i_roc][0] -= noinv_vref_step;
        found_parameters[i_roc][0] = false;
        keep_running = true;
        pflib_log(info) << "Bad slope and/or intercept. "
                        << "Setting noinv_vref of roc " << i_roc
                        << " link 0 to " << noinv_vref_tgt[i_roc][0];
      }
      if (inv_vref_tgt[i_roc][1] == -1) {
        noinv_vref_tgt[i_roc][1] -= noinv_vref_step;
        found_parameters[i_roc][1] = false;
        keep_running = true;
        pflib_log(info) << "Bad slope and/or intercept. "
                        << "Setting noinv_vref of roc " << i_roc
                        << " link 1 to " << noinv_vref_tgt[i_roc][1];
      }
      if ((inv_vref_tgt[i_roc][0] <= 0) || (inv_vref_tgt[i_roc][0] >= 1023)) {
        noinv_vref_tgt[i_roc][0] -= noinv_vref_step;
        found_parameters[i_roc][0] = false;
        keep_running = true;
        pflib_log(info) << "Target inv_vref outside of parameter range. "
                        << "Setting noinv_vref of roc " << i_roc
                        << " link 0 to " << noinv_vref_tgt[i_roc][0];
      }
      if ((inv_vref_tgt[i_roc][1] <= 0) || (inv_vref_tgt[i_roc][1] >= 1023)) {
        noinv_vref_tgt[i_roc][1] -= noinv_vref_step;
        found_parameters[i_roc][1] = false;
        keep_running = true;
        pflib_log(info) << "Target inv_vref outside of parameter range. "
                        << "Setting noinv_vref of roc " << i_roc
                        << " link 1 to " << noinv_vref_tgt[i_roc][1];
      }
    }
  }

  std::map<int, std::map<std::string, std::map<std::string, uint64_t>>>
      settings;

  for (int i_link{0}; i_link < 2; i_link++) {
    for (int i_roc : tgt->roc_ids()) {
      auto refvol_page =
          pflib::utility::string_format("REFERENCEVOLTAGE_%d", i_link);
      settings[i_roc][refvol_page]["INV_VREF"] = inv_vref_tgt[i_roc][i_link];
      settings[i_roc][refvol_page]["NOINV_VREF"] =
          noinv_vref_tgt[i_roc][i_link];
    }
  }
  return settings;
}

}  // namespace pflib::algorithm
