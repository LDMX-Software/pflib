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
      pftool::readline_int("How many events per time point?", 100);
  int target_calib =
      pftool::readline_int("At which calib should tot activate?", 440);

  std::map<int, std::array<int, 2>> vref_targets;
  std::map<int, std::array<int, 72>> trim_targets;
  const pflib::packing::SingleECONDRocErxMapping& mapping =
      tgt->getRocErxMapping();
  DecodeAndBuffer buffer{n_events, tgt->nrocs() * 2};
  tgt->setup_run(1, Target::DaqFormat::ECOND_SW_HEADERS, 1);

  // find target calibs
  int tot_vref_initial = 300;
  int trim_tot_initial = 32;
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
  }
  auto test_params = tgt->tempApplyAllROCs(parameters);
  // loops through calib, finding calib just before tot activation
  for (int i_roc : tgt->roc_ids()) {
    pflib_log(info) << "Getting calibs for roc: " << i_roc;
    calibs[i_roc].resize(72);
    for (int ch{0}; ch < 72; ch++) {
      pflib_log(info) << "Getting calib for channel: " << ch;
      std::map<std::string, std::map<std::string, uint64_t>> calib_parameters;
      bool complete = false;
      int calib = 500;
      auto ch_page = pflib::utility::string_format("CH_%d", ch);
      int calib_decrease = 50;
      while (!complete) {
        if (ch / 36 == 0) {
          calib_parameters["REFERENCEVOLTAGE_0"]["CALIB"] = calib;
          calib_parameters[ch_page]["HIGHRANGE"] = 1;
        } else {
          calib_parameters["REFERENCEVOLTAGE_1"]["CALIB"] = calib;
          calib_parameters[ch_page]["HIGHRANGE"] = 1;
        }
        auto calib_test_params = tgt->tempApplyAllROCs(calib_parameters);
        usleep(10);

        daq_run(tgt, "CHARGE", buffer, n_events, 100);
        auto data = buffer.get_buffer();
        auto [i_erx, i_ch] = mapping.toErxChannel(i_roc, ch);
        for (std::size_t i{0}; i < data.size(); i++) {
          double tot = data[i].soi().channel(i_erx, i_ch).tot();
          if (tot == -1) {
            complete = true;
          }
        }
        if ((complete == true) && (calib_decrease != 1)) {
          calib += 50;
          calib_decrease = 1;
          complete = false;
        } else if (((complete == true) && (calib_decrease == 1)) ||
                   (calib == 0)) {
          break;
        }
        calib -= calib_decrease;
      }
      pflib_log(info) << "Calib: " << calib;
      calibs[i_roc][ch] = calib;
    }
  }

  // finds and stores median calib for each link
  std::map<int, int> target_calibs_l0;
  std::map<int, int> target_calibs_l1;
  for (int i_roc : tgt->roc_ids()) {
    std::vector<int> l0(calibs[i_roc].begin(), calibs[i_roc].begin() + 36);
    std::vector<int> l1(calibs[i_roc].begin() + 36, calibs[i_roc].end());
    target_calibs_l0[i_roc] = pflib::utility::median(l0);
    target_calibs_l1[i_roc] = pflib::utility::median(l1);

    pflib_log(info) << "Roc: " << i_roc;
    pflib_log(info) << "L0 median calib: " << target_calibs_l0[i_roc];
    pflib_log(info) << "L1 median calib: " << target_calibs_l1[i_roc];
  }

  // sets calib to median of each link
  // finds trim_tot right before tot activates for each channel
  for (int i_roc : tgt->roc_ids()) {
    pflib_log(info) << "Getting trim_tot for roc: " << i_roc;
    for (int ch{0}; ch < 72; ch++) {
      auto ch_page = pflib::utility::string_format("CH_%d", ch);
      bool complete = false;
      int trim_tot = 63;
      while (!complete) {
        std::map<std::string, std::map<std::string, uint64_t>> tot_parameters;
        if (ch / 36 == 0) {
          tot_parameters["REFERENCEVOLTAGE_0"]["CALIB"] =
              target_calibs_l0[i_roc];
          tot_parameters[ch_page]["HIGHRANGE"] = 1;
          tot_parameters[ch_page]["TRIM_TOT"] = trim_tot;
        } else {
          tot_parameters["REFERENCEVOLTAGE_1"]["CALIB"] =
              target_calibs_l1[i_roc];
          tot_parameters[ch_page]["HIGHRANGE"] = 1;
          tot_parameters[ch_page]["TRIM_TOT"] = trim_tot;
        }
        auto tot_test_params = tgt->tempApplyAllROCs(tot_parameters);
        usleep(10);

        daq_run(tgt, "CHARGE", buffer, n_events, 100);
        auto data = buffer.get_buffer();
        auto [i_erx, i_ch] = mapping.toErxChannel(i_roc, ch);
        for (std::size_t i{0}; i < data.size(); i++) {
          double tot = data[i].soi().channel(i_erx, i_ch).tot();
          if (tot == -1) {
            complete = true;
            break;
          }
        }

        if (!complete) {
          trim_tot -= 1;
        }

        if (trim_tot == 0) {
          complete = true;
        }
      }
      pflib_log(info) << "trim_tot for ch " << ch << ": " << trim_tot;
      trim_targets[i_roc][ch] = trim_tot;
    }
  }

  // finds tot_vref per link which causes tot activation on target calib
  std::array<int, 2> channels = {17, 51};
  for (int i_roc : tgt->roc_ids()) {
    pflib_log(info) << "Getting tot_vref for roc: " << i_roc;
    for (int i_link = 0; i_link < 2; i_link++) {
      pflib_log(info) << "link: " << i_link;
      std::map<std::string, std::map<std::string, uint64_t>> vref_parameters;
      bool complete = false;
      int tot_vref = 1000;
      auto ch_page = pflib::utility::string_format("CH_%d", channels[i_link]);
      auto refvol_page =
          pflib::utility::string_format("REFERENCEVOLTAGE_%d", i_link);
      int tot_decrease = 50;
      while (!complete) {
        pflib_log(info) << "Testing tot_vref: " << tot_vref;
        vref_parameters[ch_page]["TRIM_TOT"] =
            trim_targets[i_roc][channels[i_link]];
        vref_parameters[ch_page]["HIGHRANGE"] = 1;
        vref_parameters[refvol_page]["CALIB"] = target_calib;
        vref_parameters[refvol_page]["TOT_VREF"] = tot_vref;
        auto vref_test_params = tgt->tempApplyAllROCs(vref_parameters);
        usleep(10);

        daq_run(tgt, "CHARGE", buffer, n_events, 100);
        auto data = buffer.get_buffer();
        auto [i_erx, i_ch] = mapping.toErxChannel(i_roc, channels[i_link]);
        for (std::size_t i{0}; i < data.size(); i++) {
          double tot = data[i].soi().channel(i_erx, i_ch).tot();
          if (tot >= 0) {
            complete = true;
          }
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
      pflib_log(info) << "tot_vref: " << tot_vref;
      vref_targets[i_roc][i_link] = tot_vref;
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
