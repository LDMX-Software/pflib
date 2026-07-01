#include "trim_toa_uva.h"
#include "../daq_run.h"
#include "../tasks/trim_toa_uva.h"
#include "pflib/utility/string_format.h"
#include "pflib/utility/median.h"

namespace pflib::algorithm {

std::map<int, std::map<std::string, std::map<std::string, uint64_t>>> trim_toa_uva(
  Target* tgt) {
 static auto the_log_{::pflib::logging::get("trim_toa_uva")};

  size_t n_events = pftool::readline_int(
    "How many events per time point?", 100);

  std::map<int, std::array<int, 72>> trim_targets;
  const pflib::packing::SingleECONDRocErxMapping& mapping =
        tgt->getRocErxMapping();
  DecodeAndBuffer buffer{n_events, tgt->nrocs() * 2};
  tgt->setup_run(1, Target::DaqFormat::ECOND_SW_HEADERS, 1);

  //find target calibs
  int trim_toa_initial = 32;
  std::map<int, std::vector<int>> calibs;
  std::map<std::string, std::map<std::string, uint64_t>>
         parameters;

  //applies test parameters
  for (int i_link{0}; i_link < 2; i_link++) {
    std::string refvol_page{pflib::utility::string_format("REFERENCEVOLTAGE_%d", i_link)};
    parameters[refvol_page]["INTCTEST"] = 1;
    parameters[refvol_page]["CHOICE_CINJ"] = 1;
  }

  for (int ch{0}; ch < 72; ch++) {
    auto ch_page = pflib::utility::string_format("CH_%d", ch);
    parameters[ch_page]["TRIM_TOA"] = trim_toa_initial;
  }
  auto test_params = tgt->tempApplyAllROCs(parameters);
  //loops through calib, finding calib just before toa activation
  for (int i_roc : tgt->roc_ids()) {
    pflib_log(info) << "Getting calibs for roc: " << i_roc;
    calibs[i_roc].resize(72);
    for(int ch{0}; ch < 72; ch++) {
      pflib_log(info) << "Getting calib for channel: " << ch;
      std::map<std::string, std::map<std::string, uint64_t>>
           calib_parameters;
      bool complete = false;
      int calib = 500;
      auto ch_page = pflib::utility::string_format("CH_%d", ch);
      int calib_decrease = 50;
      while(!complete) {
        if(ch/36 == 0){
          calib_parameters["REFERENCEVOLTAGE_0"]["CALIB"] = calib;
          calib_parameters[ch_page]["HIGHRANGE"] = 1;
          }
        else {
          calib_parameters["REFERENCEVOLTAGE_1"]["CALIB"] = calib;
          calib_parameters[ch_page]["HIGHRANGE"] = 1;
        }
        auto calib_test_params = tgt->tempApplyAllROCs(calib_parameters);
        usleep(10);

        daq_run(tgt, "CHARGE", buffer, n_events, 100);
        auto data = buffer.get_buffer();
        auto [i_erx, i_ch] = mapping.toErxChannel(i_roc, ch);
        for (std::size_t i{0}; i < data.size(); i++) {
          double toa = data[i].soi().channel(i_erx, i_ch).toa();
          pflib_log(info) << toa;
          if (toa == 0) {
            complete = true;
          }
        }
        if ((complete == true) && (calib_decrease != 1)) {
          calib += 50;
          calib_decrease = 1;
          complete = false;
        }
        else if (((complete == true) && (calib_decrease == 1)) || (calib == 0)) {
          break;
        }
        calib -= calib_decrease;
      }
      pflib_log(info) << "Calib: " << calib;
      calibs[i_roc][ch] = calib;
    }
  }

  //finds and stores median calib
  std::map<int, int> target_calibs;
  for (int i_roc : tgt->roc_ids()) {
    target_calibs[i_roc] = pflib::utility::median(calibs[i_roc]);

    pflib_log(info) << "Roc: " << i_roc;
    pflib_log(info) << "median calib: " << target_calibs[i_roc];
  }

  //sets calib to median of each link
  //finds trim_toa right before toa activates for each channel
  for (int i_roc : tgt->roc_ids()) {
    pflib_log(info) << "Getting trim_toa for roc: " << i_roc;
    for (int ch{0}; ch < 72; ch++) {
      auto ch_page = pflib::utility::string_format("CH_%d", ch);
      bool complete = false;
      int trim_toa = 63;
      while(!complete) {
          std::map<std::string, std::map<std::string, uint64_t>>
                      toa_parameters;
        if (ch/36 == 0) {
          toa_parameters["REFERENCEVOLTAGE_0"]["CALIB"] = target_calibs[i_roc];
          toa_parameters[ch_page]["HIGHRANGE"] = 1;
          toa_parameters[ch_page]["TRIM_TOA"] = trim_toa;
        }
        else {
          toa_parameters["REFERENCEVOLTAGE_1"]["CALIB"] = target_calibs[i_roc];
          toa_parameters[ch_page]["HIGHRANGE"] = 1;
          toa_parameters[ch_page]["TRIM_TOA"] = trim_toa;
        }
        auto toa_test_params = tgt->tempApplyAllROCs(toa_parameters);
        usleep(10);

        daq_run(tgt, "CHARGE", buffer, n_events, 100);
        auto data = buffer.get_buffer();
        auto [i_erx, i_ch] = mapping.toErxChannel(i_roc, ch);
        for (std::size_t i{0}; i < data.size(); i++) {
          double toa = data[i].soi().channel(i_erx, i_ch).toa();
          if (toa == 0) {
            complete = true;
            break;
          }
        }

        if (!complete) {
          trim_toa -= 1;
        }

        if (trim_toa == 0) {
          complete = true;
        }
      }
      pflib_log(info) << "trim_toa for ch " << ch << ": "  << trim_toa;
      trim_targets[i_roc][ch] = trim_toa;
    }
  }
  //create settings page
  std::map<int, std::map<std::string, std::map<std::string, uint64_t>>> settings;
  for (int i_roc : tgt->roc_ids()) {
    for (int ch{0}; ch < 72; ch++) {
      auto ch_page = pflib::utility::string_format("CH_%d", ch);
      settings[i_roc][ch_page]["TRIM_TOA"] = trim_targets[i_roc][ch];
    }
  }
  return settings;
}

}  // namespace pflib::algorithm
