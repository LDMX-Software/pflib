#include "non-linearity_scan.h"

#include <vector>
#include <cmath>

#include "../daq_run.h"
#include "pflib/utility/efficiency.h"
#include "pflib/utility/string_format.h"
#include "pflib/utility/mean.h"

namespace pflib::algorithm {

  void linear_fit(std::vector<double> x,
                  std::vector<double> y,
                  double& m,
                  double& b) {
    int n = x.size();
    double sum_x = 0, sum_y = 0, sum_xy = 0, sum_x2 = 0;

    for (int i = 0; i < n; i++) {
      sum_x  += x[i];
      sum_y  += y[i];
      sum_xy += x[i] * y[i];
      sum_x2 += x[i] * x[i];
    }

    m = (n * sum_xy - sum_x * sum_y) /
        (n * sum_x2 - sum_x * sum_x);

    b = (sum_y - m * sum_x) / n;
  }

  double ideal_steps_inl_calculation(std::vector<double> calibs, std::vector<double> peaks) {

    int min_calib = calibs[0];
    int closest_multiple{0};
    if (min_calib % 6 >= 3) {
      closest_multiple = min_calib - min_calib % 6;
    }
    else {
      closest_multiple = min_calib - 6 + min_calib % 6;
    }

    int step = closest_multiple / 6 + 235; // 235 is the assumed pedestal - to be changed later
    std::vector<double> ideal_inl;

    for (int n{0}; n <  peaks.size(); n++) {

      double diff = std::abs(step - peaks[n]);
      ideal_inl.push_back(diff);

      int current_calib = calibs[n];
        if ((current_calib % 6 == 0) && (n != 0)) {
          step++;
        }
    }
    auto max_inl_it = std::max_element(ideal_inl.begin(), ideal_inl.end());
    double max_ideal_inl = *max_inl_it;
    return max_ideal_inl;
  }

  double linear_fit_inl_calculation(std::vector<double> calibs, std::vector<double> peaks, double& m, double& b){

    std::vector<double> y;
    std::vector<double> INL;

    for (double c : calibs){
      y.push_back(m*c+b);
    }
    for (int i = 0; i < peaks.size(); i++){
      double difference = std::abs(y[i]-peaks[i]);
      INL.push_back(difference);
    }
    auto max_value = std::max_element(INL.begin(), INL.end());
    double inl_max = *max_value;

    return inl_max;
  }

  double dnl_calculation(std::vector<double> peaks) {

    std::vector<double> DNL;
    std::vector<double> steps;

    double width = 0;
    int i = 0;

    for (int val{0}; val < peaks.size(); val++) {

      int step_start = std::floor(peaks[i]);
      double diff = peaks[val] - step_start;

      if (diff < 1) {
        width++;
      }
      else {
        if (peaks[val + 1] - step_start < 1) {
          width++; // accounts for outliers within step
        }
        else { // new step
          i = val;
          steps.push_back(width);
          width = 1;
        }
      }
    }

    double avg_step = pflib::utility::mean(steps);

    for (double step : steps) {
      DNL.push_back(step - avg_step);
    }

    auto max_value = std::max_element(DNL.begin(), DNL.end());
    double dnl_max = *max_value;

    return dnl_max;
  }

  double new_dnl_calculation(std::vector<double> calibs, std::vector<double> peaks) {

    int max_adc = *std::max_element(peaks.begin(), peaks.end());
    int min_adc = *std::min_element(peaks.begin(), peaks.end());

    double width_ideal = (calibs.size)/(max_adc-min_adc+1);
    
    std::vector<double> DNL;
    std::vector<double> steps;

    double width = 0;
    int i = 0;

    for (int val{0}; val < peaks.size(); val++) {

      int step_start = std::floor(peaks[i]);
      double diff = std::abs(peaks[val] - step_start); // should probably be absolute value here

      if (diff < 1) {
        width++;
      }
      else {
        if (std::abs(peaks[val + 1] - step_start) < 1) {
          width++; // accounts for outliers within step
        }
        else { // new step
          i = val;
          steps.push_back(width);
          width = 1;
        }
      }
    }
    
    for (double step : steps) {
      DNL.push_back(std::abs((step/width_ideal) - 1));
    }

    double max_DNL = *std::max_element(DNL.begin(), DNL.end());

    return max_DNL;
  }

  template <class EventPacket>
  std::vector<double> nl_scan(Target* tgt, ROC& roc, size_t& n_events, int& channel, int& i_link, std::array<int,4> delays, std::vector<double> CALIBs, double& optimal_bx) {

    static auto the_log_{::pflib::logging::get("NL_scan")};

    auto channel_page = pflib::utility::string_format("CH_%d", channel);
    auto refvol_page = pflib::utility::string_format("REFERENCEVOLTAGE_%d", i_link);
    auto globalanalog_page = pflib::utility::string_format("GLOBALANALOG_%d", i_link);

    DecodeAndBuffer<EventPacket> buffer{n_events, 1};

    int phase_strobe{0};
    int n_phase_strobe{16};
    std::vector<double> nl_results;
    std::vector<double> adc;
    std::vector<double> avg_adc;
    std::vector<double> peaks;
    std::vector<std::array<double,2>> adc_to_bx;

    tgt->setup_run(1 /* dummy - not stored */, pftool::state.daq_format_mode,
                  1 /* dummy */);

    auto vref_test_param = roc.testParameters()
                              .add(globalanalog_page, "DELAY40", delays[0])
                              .add(globalanalog_page, "DELAY65", delays[1])
                              .add(globalanalog_page, "DELAY87", delays[2])
                              .add(globalanalog_page, "DELAY9", delays[3])
                              .add(refvol_page, "INTCTEST", 1)
                              .add(refvol_page, "CHOICE_CINJ", 0)
                              .add(channel_page, "HIGHRANGE", 1)
                              .add(channel_page, "LOWRANGE", 0) // preCC
                              .apply();  // applying static parameters
    usleep(10);

    // INL scan

    tgt->fc().fc_setup_calib(optimal_bx);
    pflib_log(info) << "charge_to_l1a = " << tgt->fc().fc_get_setup_calib();
    usleep(10);

    for (int calib : CALIBs) {
      pflib_log(info) << "CALIB = " << calib;
      for (phase_strobe = 0; phase_strobe < n_phase_strobe; phase_strobe++) {
        auto phase_strobe_test_handle =
          roc.testParameters().add("TOP", "PHASE_STROBE", phase_strobe).add(refvol_page, "CALIB_2V5", calib).apply();
        pflib_log(info) << "TOP.PHASE_STROBE = " << phase_strobe;
        usleep(10);  // make sure parameters are applied

        daq_run(tgt, "CHARGE", buffer, n_events, 100);
        auto data = buffer.get_buffer();
        for (int i{0}; i < data.size(); i++) {
          double data_adc = 0.;

          if constexpr (std::is_same_v<EventPacket,
                        pflib::packing::MultiSampleECONDEventPacket>) {
            data_adc = data[i].samples[data[i].i_soi].channel(i_link, channel).adc();
          } else if constexpr (std::is_same_v<EventPacket,
                                pflib::packing::SingleROCEventPacket>) {
            data_adc = data[i].channel(channel).adc();
          }
          adc.push_back(data_adc);
        }
        double avg = pflib::utility::mean(adc);
        avg_adc.push_back(avg);
      }
      auto max_value = std::max_element(avg_adc.begin(), avg_adc.end());
      double max_adc = *max_value;
      peaks.push_back(max_adc);
      pflib_log(info) << "AVG Peak : " << max_adc;
    }

    double m, b;
    linear_fit(CALIBs, peaks, m, b);
    double fit_inl = linear_fit_inl_calculation(CALIBs, peaks, m, b);
    double ideal_inl = ideal_steps_inl_calculation(CALIBs, peaks);
    double max_dnl = new_dnl_calculation(CALIBs, peaks);

    nl_results = {ideal_inl, fit_inl, max_dnl};

    return nl_results;
  }

  template std::vector<double> nl_scan<pflib::packing::SingleROCEventPacket>(Target* tgt, ROC& roc, size_t& n_events, int& channel, int& i_link, std::array<int,4> delays, std::vector<double> CALIBs, double& optimal_bx);
  template std::vector<double> nl_scan<pflib::packing::MultiSampleECONDEventPacket>(Target* tgt, ROC& roc, size_t& n_events, int& channel, int& i_link, std::array<int,4> delays, std::vector<double> CALIBs, double& optimal_bx);

}
