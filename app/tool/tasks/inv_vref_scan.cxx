#include "inv_vref_scan.h"

#include <nlohmann/json.hpp>

#include "../daq_run.h"
#include "../econ_links.h"
#include "pflib/utility/median.h"

ENABLE_LOGGING();

DataFitter::DataFitter() {};

void DataFitter::sort_and_append(std::vector<int>& inv_vrefs, 
                                 std::vector<int>& pedestals,
                                 std::vector<double>& stds,
                                 int& step) {
  static auto the_log_{::pflib::logging::get("inv_vref_scan:sort")};
  // We ignore first and last elements since they miss derivs
  struct DerivPoint {
    int i;
    double LH;
    double LH_std;
    double RH;
    double RH_std;
  };
  std::vector<DerivPoint> slope_points;

  double flat_threshold = 0.05 * step;
  std::vector<double> LH_derivs;
  std::vector<double> LH_stds;
  std::vector<double> RH_derivs;
  for (int i = 1; i < inv_vrefs.size()-1; i++) {
    double LH = static_cast<double>(pedestals[i] - pedestals[i-1]) / (inv_vrefs[i] - inv_vrefs[i-1]);
    double RH = static_cast<double>(pedestals[i] - pedestals[i+1]) / (inv_vrefs[i] - inv_vrefs[i+1]);

    // Threshold check. CMS uses 0.05. This value fits with my analysis as well.
    if (std::abs(LH) < flat_threshold || std::abs(RH) < flat_threshold) { // flat regime
      nonlinear_.push_back({inv_vrefs[i], pedestals[i], LH, RH});
    } else { // we're in a linear regime or there's outliers
      double LH_err = std::abs(stds[i] - stds[i-1]) / (inv_vrefs[i] - inv_vrefs[i-1]);
      double RH_err = std::abs(stds[i] - stds[i+1]) / (inv_vrefs[i] - inv_vrefs[i+1]);
      slope_points.push_back({i, LH, LH_err, RH, RH_err});
      LH_derivs.push_back(LH);
      LH_stds.push_back(err);
      RH_derivs.push_back(RH);
    }
  }
  // Now we get the slopey region. CMS removes outliers by using the ADC median.
  // From my analysis I chose to consider the std of each point, which is huge for outliers
  // compared to the points in the linear region. We set a selection of linear points when
  // they have a std < median(std).
  // We could use both LH and RH derivs to improve selections.

  
  LH_std_median_ = pflib::utility::median(LH_stds);
  LH_median_ = pflib::utility::median(LH_derivs);
  
  for (const auto& p : slope_points) {
    if ((std::abs(p.LH - LH_median_) < 0.3*std::abs(LH_median_)) &&
         p.LH_err < LH_std_median_) { // Linear regime. 
      pflib_log(info) << "inv_vref is : " << inv_vrefs[p.i];
      linear_.push_back({inv_vrefs[p.i], pedestals[p.i], p.LH, p.RH});
    }
  }

}

int DataFitter::fit(int target) {
  static auto the_log_{::pflib::logging::get("inv_vref_scan:fit")};
  // Calculate the median intercept and slope

  std::vector<double> intercepts;
  for (const auto& p : linear_) {
    double b = p.y_ - p.LH_ * p.x_;
    intercepts.push_back(b);
  }
  double median_intercept = pflib::utility::median(intercepts);
  pflib_log(info) << "The median intercept is " << median_intercept;

  // Find intersect with target. Start at the beginning of the linear regime
  int inv_vref = 0;
  int adc = 0;
  int n = linear_.size();
  while (inv_vref < 500) {
    adc = RH_median_ * inv_vref + median_intercept;
    //pflib_log(info) << "inv_vref = " << inv_vref << " with adc = " << adc;
    if (adc <= target) {
      break;
    }
    inv_vref++;
  }
  inv_vref = inv_vref;
  pflib_log(info) << "Final inv_vref is " << inv_vref;
  return inv_vref;
}


// helper function to facilitate EventPacket dependent behaviour
template <class EventPacket>
static void inv_vref_scan_writer(Target* tgt, pflib::ROC& roc, size_t nevents,
                                 std::array<int, 2>& channels) {
  int n_links = 2;
  if constexpr (std::is_same_v<EventPacket,
                               pflib::packing::MultiSampleECONDEventPacket>) {
    n_links = determine_n_links(tgt);
  }

  int noinv_vref = 612;
  int target_adc = 200;
  
  DecodeAndBuffer<EventPacket> buffer{1, 2};

  tgt->setup_run(1 /* dummy - not stored */, pftool::state.daq_format_mode,
                 1 /* dummy */);

  std::vector<int> pedestals_l0;
  std::vector<double> stds_l0;
  std::vector<int> pedestals_l1;
  std::vector<double> stds_l1;
  std::vector<int> inv_vrefs;

  int step = 1;

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
    stds_l0.push_back(pflib::utility::std(adcs_l0));
    pedestals_l1.push_back(pflib::utility::median(adcs_l1));
    stds_l1.push_back(pflib::utility::std(adcs_l1));
    inv_vrefs.push_back(inv_vref);
    }
  // sort data and fit
  DataFitter fitter_l0;
  DataFitter fitter_l1;
  fitter_l0.sort_and_append(inv_vrefs, pedestals_l0, stds_l0, step);
  fitter_l1.sort_and_append(inv_vrefs, pedestals_l1, stds_l1, step);
  int inv_vref_l0 = fitter_l0.fit(target_adc);
  int inv_vref_l1 = fitter_l1.fit(target_adc);
  
}

void inv_vref_scan(Target* tgt) {
  int nevents = pftool::readline_int("Number of events per point: ", 1);
  std::array<int, 2> channels = {17, 51};

  auto roc = tgt->roc(pftool::state.iroc);

  if (pftool::state.daq_format_mode == Target::DaqFormat::SIMPLEROC) {
    inv_vref_scan_writer<pflib::packing::SingleROCEventPacket>(
        tgt, roc, nevents, channels);
  } else if (pftool::state.daq_format_mode ==
             Target::DaqFormat::ECOND_SW_HEADERS) {
    inv_vref_scan_writer<pflib::packing::MultiSampleECONDEventPacket>(
        tgt, roc, nevents, channels);
  }
}
