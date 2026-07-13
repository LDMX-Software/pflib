#include "tot_scan.h"

#include "../daq_run.h"
#include "../tasks/tot_scan.h"
#include "pflib/utility/median.h"
#include "pflib/utility/string_format.h"

namespace pflib::algorithm {

std::map<int, std::map<std::string, std::map<std::string, uint64_t>>> tot_scan(
    Target* tgt) {
  static auto the_log_{::pflib::logging::get("tot_scan")};

  size_t n_events =
      pftool::readline_int("How many events per time point?", 10);
  int target_calib =
      pftool::readline_int("At which calib should tot activate?", 455);

  std::map<int, std::array<int, 2>> vref_targets;
  std::map<int, std::array<int, 72>> trim_targets;
  const pflib::packing::SingleECONDRocErxMapping& mapping =
      tgt->getRocErxMapping();
  DecodeAndBuffer buffer{n_events, tgt->nrocs() * 2};
  tgt->setup_run(1, Target::DaqFormat::ECOND_SW_HEADERS, 1);

  // find target calibs
  int tot_vref_initial = 420;
  int trim_tot_initial = 31;
  std::map<int, std::vector<int>> calibs;
  std::map<std::string, std::map<std::string, uint64_t>> parameters;

  // applies test parameters
  for (int i_link{0}; i_link < 2; i_link++) {
    std::string refvol_page{
        pflib::utility::string_format("REFERENCEVOLTAGE_%d", i_link)};
    parameters[refvol_page]["TOT_VREF"] = tot_vref_initial;
    parameters[refvol_page]["INTCTEST"] = 1;
    parameters[refvol_page]["CHOICE_CINJ"] = 1;
  }

  for (int ch{0}; ch < 72; ch++) {
    auto ch_page = pflib::utility::string_format("CH_%d", ch);
    parameters[ch_page]["TRIM_TOT"] = trim_tot_initial;
    parameters[ch_page]["LOWRANGE"] = 0;
    parameters[ch_page]["HIGHRANGE"] = 0;
  }
  auto test_params = tgt->tempApplyAllROCs(parameters);
  /*
  // loops through calib, finding calib just before tot activation
  for (int i_roc : tgt->roc_ids()) {
    calibs[i_roc].resize(72);
  }
  for (int ch{0}; ch < 72; ch++) {
    std::map<int, std::map<std::string, std::map<std::string, uint64_t>>> calib_parameters;
    bool complete = false;
    auto ch_page = pflib::utility::string_format("CH_%d", ch);
    std::map<int, int> calib_decrease;
    std::map<int, bool> roc_complete;
    std::map<int, int> calib;
    for (int i_roc : tgt->roc_ids()) {
      roc_complete[i_roc] = false;
      calib_decrease[i_roc] = 50;
      calib[i_roc] = 500;
      calibs[i_roc][ch] = -1;
    }
    while (!complete) {
      for (int i_roc : tgt->roc_ids()) {
        if (ch / 36 == 0) {
          calib_parameters[i_roc]["REFERENCEVOLTAGE_0"]["CALIB"] = calib[i_roc];
          calib_parameters[i_roc][ch_page]["HIGHRANGE"] = 1;
        } else {
          calib_parameters[i_roc]["REFERENCEVOLTAGE_1"]["CALIB"] = calib[i_roc];
          calib_parameters[i_roc][ch_page]["HIGHRANGE"] = 1;
        }
      }
      auto calib_test_params = tgt->tempApplyAllROCs(calib_parameters);
      usleep(10);
      daq_run(tgt, "CHARGE", buffer, n_events, 100);
      auto data = buffer.get_buffer();

      for (int i_roc : tgt->roc_ids()) {
        auto [i_erx, i_ch] = mapping.toErxChannel(i_roc, ch);
        double sum = 0;
        for (std::size_t i{0}; i < data.size(); i++) {
          double tot = data[i].soi().channel(i_erx, i_ch).tot();
          if (tot >= 0) {
            sum += 1;
          }
        }
        double eff = sum / data.size();
        if (eff <= 0.5) {
          roc_complete[i_roc] = true;
        }
        if ((roc_complete[i_roc] == true) && (calib_decrease[i_roc] != 1)) {
          calib[i_roc] += 50;
          calib_decrease[i_roc] = 1;
          roc_complete[i_roc] = false;
        } else if ((((roc_complete[i_roc] == true) && (calib_decrease[i_roc] == 1)) ||
                   (calib[i_roc] == 0)) && calibs[i_roc][ch] == -1) {
            calibs[i_roc][ch] = calib[i_roc];
        }
        calib[i_roc] -= calib_decrease[i_roc];
      }
      complete = true;
      for (int i_roc : tgt->roc_ids()) {
        if (calibs[i_roc][ch] == -1) {
          complete = false;
        }
      }
    }
    for (int i_roc : tgt->roc_ids()) {
      pflib_log(info) << "Calib for ch " << ch << ": " << calibs[i_roc][ch];
    }
  }
  */
// binary search through calib, finding calib just before tot activation
  for (int i_roc : tgt->roc_ids()) {
    calibs[i_roc].resize(72);
  }
  for (int ch{0}; ch < 72; ch++) {
    std::map<int, std::map<std::string, std::map<std::string, uint64_t>>> calib_parameters;
    bool complete = false;
    auto ch_page = pflib::utility::string_format("CH_%d", ch);
    std::map<int, int> low;
    std::map<int, int> high;
    std::map<int, int> mid;
    for (int i_roc : tgt->roc_ids()) {
      low[i_roc] = 0;
      high[i_roc] = 1000;
      calibs[i_roc][ch] = -1;
    }
    while (!complete) {
      for (int i_roc : tgt->roc_ids()) {
        if (calibs[i_roc][ch] != -1) {
          continue;
        }
        mid[i_roc] = (low[i_roc] + high[i_roc] + 1) / 2;
        if (ch / 36 == 0) {
          calib_parameters[i_roc]["REFERENCEVOLTAGE_0"]["CALIB"] = mid[i_roc];
          calib_parameters[i_roc][ch_page]["HIGHRANGE"] = 1;
        } else {
          calib_parameters[i_roc]["REFERENCEVOLTAGE_1"]["CALIB"] = mid[i_roc];
          calib_parameters[i_roc][ch_page]["HIGHRANGE"] = 1;
        }
      }
      auto calib_test_params = tgt->tempApplyAllROCs(calib_parameters);
      usleep(10);
      daq_run(tgt, "CHARGE", buffer, n_events, pftool::state.daq_rate);
      auto data = buffer.get_buffer();

      for (int i_roc : tgt->roc_ids()) {
        if (calibs[i_roc][ch] != -1) {
          continue;
        }
        auto [i_erx, i_ch] = mapping.toErxChannel(i_roc, ch);
        double sum = 0;
        for (std::size_t i{0}; i < data.size(); i++) {
          double tot = data[i].soi().channel(i_erx, i_ch).tot();
          if (tot >= 0) {
            sum += 1;
          }
        }
        double eff = sum / data.size();
        if (eff <= 0.5) {
          low[i_roc] = mid[i_roc];
        }
        else {
          high[i_roc] = mid[i_roc] -1;
        }
        if (low[i_roc] == high[i_roc]) {
          calibs[i_roc][ch] = low[i_roc];
        }
      }
      complete = true;
      for (int i_roc : tgt->roc_ids()) {
        if (calibs[i_roc][ch] == -1) {
          complete = false;
        }
      }
    }
    for (int i_roc : tgt->roc_ids()) {
      pflib_log(info) << "Calib for ch " << ch << ": " << calibs[i_roc][ch];
    }
  }
  // finds and stores middle calib for each link
  //std::map<int, int> target_calibs_l0;
  //std::map<int, int> target_calibs_l1;
  std::map<int, std::array<int, 2>> channels;
  for (int i_roc : tgt->roc_ids()) {
    std::vector<int> l0(calibs[i_roc].begin(), calibs[i_roc].begin() + 36);
    std::vector<int> l1(calibs[i_roc].begin() + 36, calibs[i_roc].end());
    //std::vector<int> l0_sorted = l0;
    //std::vector<int> l1_sorted = l1;
    //std::sort(l0_sorted.begin(), l0_sorted.end());
   //std::sort(l1_sorted.begin(), l1_sorted.end());

    int median_calib_l0 = pflib::utility::median(l0);
    int median_calib_l1 = pflib::utility::median(l1);
    //channels[i_roc][0] = std::distance(l0.begin(), std::find(l0.begin(), l0.end(), target_calibs_l0[i_roc]));
    //channels[i_roc][1] = std::distance(l1.begin(), std::find(l1.begin(), l1.end(), target_calibs_l1[i_roc])) + 36;

    pflib_log(info) << "Roc: " << i_roc;
    pflib_log(info) << "L0 median calib: " << median_calib_l0;
    pflib_log(info) << "L1 median calib: " << median_calib_l1;

    std::vector<int> filtered_l0;
    for (int val : l0) {
        if (std::abs(val - median_calib_l0) <= 100) {
            filtered_l0.push_back(val);
        }
    }
    std::vector<int> filtered_l1;
    for (int val : l1) {
        if (std::abs(val - median_calib_l1) <= 100) {
            filtered_l1.push_back(val);
        }
    }
    int target_l0 = (*std::min_element(filtered_l0.begin(), filtered_l0.end())
                    + *std::max_element(filtered_l0.begin(), filtered_l0.end())) / 2;

    int target_l1 = (*std::min_element(filtered_l1.begin(), filtered_l1.end())
                    + *std::max_element(filtered_l1.begin(), filtered_l1.end())) / 2;

    auto it_l0 = std::min_element(l0.begin(), l0.end(),
        [target_l0](int a, int b) {
            return std::abs(a - target_l0) < std::abs(b - target_l0);});
    channels[i_roc][0] = std::distance(l0.begin(), it_l0);

    auto it_l1 = std::min_element(l1.begin(),l1.end(),
        [target_l1](int a, int b) {
            return std::abs(a - target_l1) < std::abs(b - target_l1);});
    channels[i_roc][1] = std::distance(l1.begin(), it_l1) + 36;

    pflib_log(info) << "Using channel: " << channels[i_roc][0];
    pflib_log(info) << "Using channel: " << channels[i_roc][1];
  }

  // finds tot_vref per link which causes 0.5 tot eff on target calib
  // uses the channel with median calib found before to calibrate
  for (int i_roc : tgt->roc_ids()) {
    pflib_log(info) << "Getting tot_vref for roc: " << i_roc;
    for (int i_link = 0; i_link < 2; i_link++) {
      pflib_log(info) << "link: " << i_link;
      std::map<std::string, std::map<std::string, uint64_t>> vref_parameters;
      bool complete = false;
      int tot_vref = 1000;
      auto ch_page = pflib::utility::string_format("CH_%d", channels[i_roc][i_link]);
      auto refvol_page =
          pflib::utility::string_format("REFERENCEVOLTAGE_%d", i_link);
      int tot_decrease = 50;
      while (!complete) {
        pflib_log(info) << "Testing tot_vref: " << tot_vref;
        vref_parameters[ch_page]["TRIM_TOT"] = 31;
        vref_parameters[ch_page]["HIGHRANGE"] = 1;
        vref_parameters[refvol_page]["CALIB"] = target_calib;
        vref_parameters[refvol_page]["TOT_VREF"] = tot_vref;
        auto vref_test_params = tgt->tempApplyAllROCs(vref_parameters);
        usleep(10);

        daq_run(tgt, "CHARGE", buffer, n_events, pftool::state.daq_rate);
        auto data = buffer.get_buffer();
        auto [i_erx, i_ch] = mapping.toErxChannel(i_roc, channels[i_roc][i_link]);
        double sum = 0;
        for (std::size_t i{0}; i < data.size(); i++) {
          double tot = data[i].soi().channel(i_erx, i_ch).tot();
          if (tot >= 0) {
            sum += 1;
          }
        }
        double eff = sum / data.size();
        if (eff >= 0.5) {
          complete = true;;
        }
        if ((complete == true) && (tot_decrease != 1)) {
          tot_vref += 50;
          tot_decrease = 1;
          complete = false;
        } else if (((complete == true) && (tot_decrease == 1)) ||
                   (tot_vref == 0)) {
          break;
        }
        tot_vref -= tot_decrease;
      }
      pflib_log(info) << "tot_vref for link " << i_link << ": " << tot_vref;
      vref_targets[i_roc][i_link] = tot_vref;
    }
  }
  // finds trim_tot right before tot reaches 0.5 eff for each channel
  std::map<int, std::map<std::string, std::map<std::string, uint64_t>>> pre_tot_parameters;
  for (int i_roc : tgt->roc_ids()) {
    for (int i_link{0}; i_link < 2; i_link++ ) {
      auto refvol_page =
              pflib::utility::string_format("REFERENCEVOLTAGE_%d", i_link);
      pre_tot_parameters[i_roc][refvol_page]["CALIB"] =
          target_calib;
      pre_tot_parameters[i_roc][refvol_page]["TOT_VREF"] =
          vref_targets[i_roc][i_link];
    }
  }
  auto pre_tot_test_params = tgt->tempApplyAllROCs(pre_tot_parameters);

  for (int i_roc : tgt->roc_ids()) {
    trim_targets[i_roc].fill(-1);
  }

  for (int ch{0}; ch < 72; ch++) {
    pflib_log(info) << "Getting trim values for channel " << ch;
    auto ch_page = pflib::utility::string_format("CH_%d", ch);
    bool complete = false;
    std::map<int, std::map<std::string, std::map<std::string, uint64_t>>> tot_parameters;
    for (int trim_tot = 63; trim_tot >= 0; trim_tot--) {
      for (int i_roc : tgt->roc_ids()) {
        if (trim_targets[i_roc][ch] != -1) {continue;}
        if (ch / 36 == 0) {
          tot_parameters[i_roc][ch_page]["HIGHRANGE"] = 1;
          tot_parameters[i_roc][ch_page]["TRIM_TOT"] = trim_tot;
        } else {
          tot_parameters[i_roc][ch_page]["HIGHRANGE"] = 1;
          tot_parameters[i_roc][ch_page]["TRIM_TOT"] = trim_tot;
        }
      }
      auto tot_test_params = tgt->tempApplyAllROCs(tot_parameters);
      usleep(10);

      daq_run(tgt, "CHARGE", buffer, n_events, pftool::state.daq_rate);
      auto data = buffer.get_buffer();
      for (int i_roc : tgt->roc_ids()) {
        if (trim_targets[i_roc][ch] != -1) {continue;}
        auto [i_erx, i_ch] = mapping.toErxChannel(i_roc, ch);
        double sum = 0;
        for (std::size_t i{0}; i < data.size(); i++) {
          double tot = data[i].soi().channel(i_erx, i_ch).tot();
          if (tot >= 0) {
            sum += 1;
          }
        }
        double eff = sum / data.size();
        if ((eff <= 0.5) || (trim_tot == 0)) {
          trim_targets[i_roc][ch] = trim_tot;
        }
      }
      bool complete = true;
      for (int i_roc : tgt->roc_ids()) {
        if (trim_targets[i_roc][ch] == -1) {
          complete = false;
          break;
        }
      }
      if (complete == true) {break;}
    }
  }

  // create settings page
  std::map<int, std::map<std::string, std::map<std::string, uint64_t>>>
      settings;
  for (int i_roc : tgt->roc_ids()) {
    for (int i_link{0}; i_link < 2; i_link++) {
      auto refvol_page =
          pflib::utility::string_format("REFERENCEVOLTAGE_%d", i_link);
      settings[i_roc][refvol_page]["TOT_VREF"] = vref_targets[i_roc][i_link];
    }

    for (int ch{0}; ch < 72; ch++) {
      auto ch_page = pflib::utility::string_format("CH_%d", ch);
      settings[i_roc][ch_page]["TRIM_TOT"] = trim_targets[i_roc][ch];
    }
  }
  return settings;
}

}  // namespace pflib::algorithm
