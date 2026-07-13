
#include "toa_scan.h"
#include "../daq_run.h"
#include "../tasks/toa_scan.h"
#include "pflib/utility/string_format.h"
#include "pflib/utility/median.h"

namespace pflib::algorithm {

std::map<int, std::map<std::string, std::map<std::string, uint64_t>>> toa_scan(
  Target* tgt) {
 static auto the_log_{::pflib::logging::get("toa_scan")};

  //define parameters
  size_t n_events = pftool::readline_int(
    "How many events per time point?", 10);
  int target_calib = pftool::readline_int(
    "Target calib?", 1200);
  int vref_buffer = pftool::readline_int(
    "Vref buffer?", 5);
  std::map<int, std::array<int, 72>> trim_targets;
  const pflib::packing::SingleECONDRocErxMapping& mapping =
        tgt->getRocErxMapping();
  DecodeAndBuffer buffer{n_events, tgt->nrocs() * 2};
  tgt->setup_run(1, Target::DaqFormat::ECOND_SW_HEADERS, 1);

  //set parameters to find target vrefs
  int trim_toa_initial = 63; //gives us space to change trim_toa
  std::map<int, std::vector<int>> calibs;
  std::map<std::string, std::map<std::string, uint64_t>>
         parameters;

  for (int ch{0}; ch < 72; ch++) {
    auto ch_page = pflib::utility::string_format("CH_%d", ch);
    parameters[ch_page]["TRIM_TOA"] = trim_toa_initial;
  }
  auto test_params = tgt->tempApplyAllROCs(parameters);

  std::map<int, std::array<int, 2>> vrefs;
  for (int i_roc : tgt->roc_ids()) {
    vrefs[i_roc] = {-1, -1};
  }

  // loop over runs, from toa_vref = 0 to = 255
  for (int toa_vref{255}; toa_vref >= 0; toa_vref--) {
    //looks for highest toa_vref where pedestals activate toa
    std::map<std::string, std::map<std::string, uint64_t>>
                      vref_parameters;
    vref_parameters["REFERENCEVOLTAGE_0"]["TOA_VREF"] = toa_vref;
    vref_parameters["REFERENCEVOLTAGE_1"]["TOA_VREF"] = toa_vref;
    auto vref_test_params = tgt->tempApplyAllROCs(vref_parameters);
    usleep(10);

    daq_run(tgt, "PEDESTAL", buffer, n_events, pftool::state.daq_rate);
    auto data = buffer.get_buffer();

    for (int i_roc : tgt->roc_ids()) {
      for (int ch{0}; ch < 72; ch++) {
        auto [i_erx, i_ch] = mapping.toErxChannel(i_roc, ch);
        for (std::size_t i{0}; i < data.size(); i++) {
          double toa = data[i].soi().channel(i_erx, i_ch).toa();
          if ((toa > 0) && (vrefs[i_roc][ch/36] == -1)) {
            vrefs[i_roc][ch/36] = toa_vref + vref_buffer;
            pflib_log(info) << "Roc: " << i_roc << " link: " << ch/36 << " vref: " << vrefs[i_roc][ch/36];
          }
        }
      }
    }
    bool done = true;
    for (int i_roc : tgt->roc_ids()) {
      for (int i_link{0}; i_link < 2; i_link++) {
        if (vrefs[i_roc][i_link] == -1) {
          done = false;
        }
      }
    }
    if (done) {
      pflib_log(info) << "First sweep done";
      break;
    }
  }

  //set parameters to center target vref around target calib
  std::map<int, std::array<int, 2>> vref_targets;
  for (int i_roc : tgt->roc_ids()) {
    vref_targets[i_roc] = {-1, -1};
  }

  std::map<int, std::map<std::string, std::map<std::string, uint64_t>>>
          buffer_parameters;

  for (int i_roc : tgt->roc_ids()) {
    for (int i_link{0}; i_link < 2; i_link++) {
      std::string refvol_page{pflib::utility::string_format("REFERENCEVOLTAGE_%d", i_link)};
      buffer_parameters[i_roc][refvol_page]["INTCTEST"] = 1;
      buffer_parameters[i_roc][refvol_page]["CHOICE_CINJ"] = 1;
    }
    for (int ch{0}; ch < 72; ch++) {
      auto ch_page = pflib::utility::string_format("CH_%d", ch);
      buffer_parameters[i_roc][ch_page]["TRIM_TOA"] = 31;
    }
  }
  auto buffer_test_params = tgt->tempApplyAllROCs(buffer_parameters);

  bool complete = false;

  //increases target vref until half of channels are below 0.5 eff at trim_toa = 32
  while(!complete) {
    std::map<int, std::map<std::string, std::map<std::string, uint64_t>>>
          buffer_vrefs;
    for (int i_roc : tgt->roc_ids()) {
      for (int i_link{0}; i_link < 2; i_link++) {
        std::string refvol_page{pflib::utility::string_format("REFERENCEVOLTAGE_%d", i_link)};
        buffer_vrefs[i_roc][refvol_page]["TOA_VREF"] = vrefs[i_roc][i_link];
        buffer_vrefs[i_roc][refvol_page]["CALIB"] = target_calib;
      }
      for (int ch{0}; ch < 72; ch++) {
        auto ch_page = pflib::utility::string_format("CH_%d", ch);
        buffer_vrefs[i_roc][ch_page]["HIGHRANGE"] = 0;
        buffer_vrefs[i_roc][ch_page]["LOWRANGE"] = 1;
      }
    }
    auto buffer_test_vrefs = tgt->tempApplyAllROCs(buffer_vrefs);
    usleep(10);

    daq_run(tgt, "CHARGE", buffer, n_events, pftool::state.daq_rate);
    auto data = buffer.get_buffer();
    for (int i_roc : tgt->roc_ids()) {
      std::array<int, 2> low_ch = {0, 0};
      for (int ch{0}; ch < 72; ch++) {
        auto [i_erx, i_ch] = mapping.toErxChannel(i_roc, ch);
        double sum = 0;
        for (std::size_t i{0}; i < data.size(); i++) {
          double toa = data[i].soi().channel(i_erx, i_ch).toa();
          if (toa > 0) {
            sum += 1;
          }
        }
        double eff = sum / data.size();
        if (eff <= 0.5) {
          low_ch[ch/36] += 1;
        }
      }
      for (int i_link{0}; i_link < 2; i_link++) {
        //pflib_log(info) << "link: " << i_link << " vref: " << vrefs[i_roc][i_link] << " low: " << low_ch[i_link];
        if ((low_ch[i_link] >= 18) && (vref_targets[i_roc][i_link] == -1)) {
          vref_targets[i_roc][i_link] = vrefs[i_roc][i_link];
          pflib_log(info) << "Roc: " << i_roc << " link: " << i_link << " vref: " << vref_targets[i_roc][i_link];
        }
        else if (vref_targets[i_roc][i_link] == -1) {
          vrefs[i_roc][i_link] += 1;
        }
      }
    }
    complete = true;
    for (int i_roc : tgt->roc_ids()) {
      for (int i_link{0}; i_link < 2; i_link++) {
        if(vref_targets[i_roc][i_link] == -1) {
          complete = false;
        }
      }
    }
  }
  pflib_log(info) << "Second sweep done";

  //set parameters to find trim_toa
  std::map<int, std::map<std::string, std::map<std::string, uint64_t>>>
          link_parameters;

  for (int i_roc : tgt->roc_ids()) {
    for (int i_link{0}; i_link < 2; i_link++) {
      std::string refvol_page{pflib::utility::string_format("REFERENCEVOLTAGE_%d", i_link)};
      link_parameters[i_roc][refvol_page]["INTCTEST"] = 1;
      link_parameters[i_roc][refvol_page]["CHOICE_CINJ"] = 1;
      link_parameters[i_roc][refvol_page]["TOA_VREF"] = vref_targets[i_roc][i_link];
      link_parameters[i_roc][refvol_page]["CALIB"] = target_calib;
    }
    for (int ch{0}; ch < 72; ch++) {
      auto ch_page = pflib::utility::string_format("CH_%d", ch);
      link_parameters[i_roc][ch_page]["HIGHRANGE"] = 0;
      link_parameters[i_roc][ch_page]["LOWRANGE"] = 1;
    }
  }
  auto link_test_params = tgt->tempApplyAllROCs(link_parameters);

/*
  //finds trim_toa right before toa reaches 0.5 eff at target calib
  for (int i_roc : tgt->roc_ids()) {
    pflib_log(info) << "Getting trim_toa for roc: " << i_roc;
    for (int ch{0}; ch < 72; ch++) {
      auto ch_page = pflib::utility::string_format("CH_%d", ch);
      bool complete = false;
      int trim_toa = 63;
      while(!complete) {
        std::map<std::string, std::map<std::string, uint64_t>>
                      toa_parameters;
        toa_parameters[ch_page]["TRIM_TOA"] = trim_toa;
        auto toa_test_params = tgt->tempApplyAllROCs(toa_parameters);
        usleep(10);

        daq_run(tgt, "CHARGE", buffer, n_events, pftool::state.daq_rate);
        auto data = buffer.get_buffer();
        auto [i_erx, i_ch] = mapping.toErxChannel(i_roc, ch);
        double sum = 0;
        for (std::size_t i{0}; i < data.size(); i++) {
          double toa = data[i].soi().channel(i_erx, i_ch).toa();
          if (toa > 0) {
            sum += 1;
          }
        }
        double eff = sum / data.size();
        if (eff <= 0.5) {
          complete = true;
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
*/
  //finds eff by trim_toa
  std::map<int, std::array<std::array<double, 64>, 72>> effs;
  for (int trim_toa{0}; trim_toa < 64; trim_toa++) {
    pflib_log(info) << "Finding effs for trim_toa: " << trim_toa;
    std::map<std::string, std::map<std::string, uint64_t>>
                  toa_parameters;
    for (int ch{0}; ch < 72; ch++) {
      auto ch_page = pflib::utility::string_format("CH_%d", ch);
      toa_parameters[ch_page]["TRIM_TOA"] = trim_toa;
    }
    auto toa_test_params = tgt->tempApplyAllROCs(toa_parameters);
    usleep(10);

    daq_run(tgt, "CHARGE", buffer, n_events, pftool::state.daq_rate);
    auto data = buffer.get_buffer();

    for (int i_roc : tgt->roc_ids()) {
      for (int ch{0}; ch < 72; ch++) {
        auto [i_erx, i_ch] = mapping.toErxChannel(i_roc, ch);
        double sum = 0;
        for (std::size_t i{0}; i < data.size(); i++) {
          double toa = data[i].soi().channel(i_erx, i_ch).toa();
          if (toa > 0) {
            sum += 1;
          }
        }
        effs[i_roc][ch][trim_toa] = sum / data.size();
      }
    }
  }

  //finds trim_toa right before toa reaches 0.5 eff
  for (int i_roc : tgt->roc_ids()) {
    pflib_log(info) << "Finding trim_toa for roc " << i_roc;
    for (int ch{0}; ch < 72; ch++) {
      for (int trim_toa{63}; trim_toa >= 0; trim_toa--) {
        if ((effs[i_roc][ch][trim_toa] <= 0.5) || (trim_toa == 0)) {
          pflib_log(info) << "trim_toa for ch " << ch << ": "  << trim_toa;
          trim_targets[i_roc][ch] = trim_toa;
          break;
        }
      }
    }
  }


  //create settings page
  std::map<int, std::map<std::string, std::map<std::string, uint64_t>>> settings;
  for (int i_roc : tgt->roc_ids()) {
    for (int ch{0}; ch < 72; ch++) {
      auto ch_page = pflib::utility::string_format("CH_%d", ch);
      settings[i_roc][ch_page]["TRIM_TOA"] = trim_targets[i_roc][ch];
    }
    for (int i_link = 0; i_link < 2; i_link++) {
      auto refvol_page = pflib::utility::string_format("REFERENCEVOLTAGE_%d", i_link);
      settings[i_roc][refvol_page]["TOA_VREF"] = vref_targets[i_roc][i_link];
    }
  }
  return settings;
}

}  // namespace pflib::algorithm
