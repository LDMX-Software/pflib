#include "multi_channel_scan.h"

#include <filesystem>
#include <nlohmann/json.hpp>

#include "../daq_run.h"
#include "load_parameter_points.h"
#include "pflib/utility/string_format.h"

ENABLE_LOGGING();

void multi_channel_scan(Target* tgt) {
  int nevents = pftool::readline_int("How many events per time point? ", 1);
  int calib = pftool::readline_int("Setting for calib pulse amplitude? ",
                                   highrange ? 64 : 1024);
  int start_bx = pftool::readline_int("Starting BX? ", -1);
  int n_bx = pftool::readline_int("Number of BX? ", 3);
  int link{0};
  pftool::readline_bool("Link 0 [Y] or link 1 [N]", "true") ? link = 0 : link = 1;
  std::string fname = pftool::readline_path("multi-channel-scan", ".csv");

  int ch0{0};
  link == 0 ? ch = 16 : ch = 52;

  auto roc{tgt->roc(pftool::state.iroc)};
  auto test_param_handle = roc.testParameters();
  auto refvol_page = pflib::utility::string_format("REFERENCEVOLTAGE_%d", link);
  test_param_handle
        .add(refvol_page, "CALIB_2V5", calib)
        .add(refvol_page, "INTCTEST", 1)
        .apply();
  }

  auto test_param = test_param_handle.apply();


  std::filesystem::path parameter_points_file =
      pftool::readline("File of parameter points: ");

  pflib_log(info) << "loading parameter points file...";
  auto [param_names, param_values] =
      load_parameter_points(parameter_points_file);
  pflib_log(info) << "successfully loaded parameter points";

  int phase_strobe{0};
  int charge_to_l1a{0};
  double time{0};
  double clock_cycle{25.0};
  int n_phase_strobe{16};
  int offset{1};
  std::size_t i_param_point{0};
  DecodeAndWriteToCSV writer{
      fname,
      [&](std::ofstream& f) {
        nlohmann::json header;
        header["preCC"] = "True";
        f << std::boolalpha << "# " << header << '\n' << "time,";
        for (const auto& [page, parameter] : param_names) {
          f << page << '.' << parameter << ',';
        }
        f << pflib::packing::Sample::to_csv_header << '\n';
      },
      [&](std::ofstream& f, const pflib::packing::SingleROCEventPacket& ep) {
        for (int ch = ch0 - 16; ch < ch0 + 16; ch++) {
          f << time << ',';

          for (const auto& val : param_values[i_param_point]) {
            f << val << ',';
          }
          f << ch << ',';

          ep.channel(ch).to_csv(f);
          f << '\n';
        }
      }};
  tgt->setup_run(1 /* dummy - not stored */, Target::DaqFormat::SIMPLEROC,
                 1 /* dummy */);
  // Do the scan for increasing amount of channels
  for (int j = 0; j < 16; j++) {
    int nr_channels = 2*j;
    for (int ch = ch0-nr_channels; ch <= ch0+nr_channels; ch++) {
      auto channel_page = pflib::utility::string_format("CH_%d", ch);
      test_param_handle
          .add(channel_page, "HIGHRANGE", 1)
    }
    auto test_param_two = test_param_handle.apply(); 
    for (; i_param_point < param_values.size(); i_param_point++) {
      // set parameters
      auto test_param_builder = roc.testParameters();
      for (std::size_t i_param{0}; i_param < param_names.size(); i_param++) {
        const auto& full_page = param_names[i_param].first;
        std::string str_page = full_page.substr(0, full_page.find("_"));
        const auto& param = param_names[i_param].second;
        int value = param_values[i_param_point][i_param];

        if (str_page == "CH" || str_page == "TOP") {
          test_param_builder.add(full_page, param, value);
        } else {
          test_param_builder.add(str_page + "_" + std::to_string(link), param,
                                     value);
          }
        }
        pflib_log(info) << param << " = " << value;
      }
      auto test_param_three = test_param_builder.apply();
      // timescan
      auto central_charge_to_l1a = tgt->fc().fc_get_setup_calib();
      for (charge_to_l1a = central_charge_to_l1a + start_bx;
           charge_to_l1a < central_charge_to_l1a + start_bx + n_bx;
           charge_to_l1a++) {
        tgt->fc().fc_setup_calib(charge_to_l1a);
        pflib_log(info) << "charge_to_l1a = " << tgt->fc().fc_get_setup_calib();
        for (phase_strobe = 0; phase_strobe < n_phase_strobe; phase_strobe++) {
          auto phase_strobe_test_handle =
              roc.testParameters()
                  .add("TOP", "PHASE_STROBE", phase_strobe)
                  .apply();
          pflib_log(info) << "TOP.PHASE_STROBE = " << phase_strobe;
          usleep(10);  // make sure parameters are applied
          time =
              (charge_to_l1a - central_charge_to_l1a + offset) * clock_cycle -
              phase_strobe * clock_cycle / n_phase_strobe;
          daq_run(tgt, "CHARGE", writer, nevents, pftool::state.daq_rate);
        }
      }
      // reset charge_to_l1a to central value
      tgt->fc().fc_setup_calib(central_charge_to_l1a);
    }
  }
}
