#include "parameter_timescan.h"

#include <filesystem>
#include <nlohmann/json.hpp>

#include "../daq_run.h"
#include "load_parameter_points.h"
#include "pflib/utility/string_format.h"

ENABLE_LOGGING();

void parameter_timescan_writer(Target* tgt, pflib::ROC& roc, std::string& fname,
                               size_t nevents, bool highrange, bool preCC,
                               std::filesystem::path& parameter_points_file,
                               std::array<int, 2>& channels) {
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
        header["highrange"] = highrange;
        header["preCC"] = preCC;
        f << std::boolalpha << "# " << header << '\n' << "time,";
        for (const auto& [page, parameter] : param_names) {
          f << page << '.' << parameter << ',';
        }
        f << "channel," << pflib::packing::Sample::to_csv_header << '\n';
      },
      [&](std::ofstream& f, const EventPacket& ep) {
        for (int ch : channels) {
          f << time << ',';

          for (const auto& val : param_values[i_param_point]) {
            f << val << ',';
          }
          f << ch << ',';

          if constexpr (std::is_same_v<
                            EventPacket,
                            pflib::packing::MultiSampleECONDEventPacket>) {
            ep.samples[ep.i_soi].channel(link, i_ch).to_csv(f);
          } else if constexpr (std::is_same_v<
                                   EventPacket,
                                   pflib::packing::SingleROCEventPacket>) {
            ep.channel(channel).to_csv(f);
          } else {
            PFEXCEPTION_RAISE("BadConf",
                              "Unable to do all_channels_to_csv for the "
                              "currently configured format.");
          }
          f << '\n';
        }
      }};
  tgt->setup_run(1 /* dummy - not stored */, pftool::state.daq_format_mode,
                 1 /* dummy */);
  for (; i_param_point < param_values.size(); i_param_point++) {
    // set parameters
    auto test_param_builder = roc.testParameters();
    // Add implementation for other pages as well
    for (std::size_t i_param{0}; i_param < param_names.size(); i_param++) {
      const auto& full_page = param_names[i_param].first;
      std::string str_page = full_page.substr(0, full_page.find("_"));
      const auto& param = param_names[i_param].second;
      int value = param_values[i_param_point][i_param];

      if (str_page == "CH") {
        test_param_builder.add(full_page, param, value);
      } else if (str_page == "TOP") {
        test_param_builder.add(full_page, param, value);
      } else {
        for (int l = 0; l < 2; l++) {
          if (link[l]) {
            test_param_builder.add(str_page + "_" + std::to_string(l), param,
                                   value);
          }
        }
      }
      pflib_log(info) << param << " = " << value;
    }
    auto test_param = test_param_builder.apply();
    // timescan
    auto central_charge_to_l1a = tgt->fc().fc_get_setup_calib();
    for (charge_to_l1a = central_charge_to_l1a + start_bx;
         charge_to_l1a < central_charge_to_l1a + start_bx + n_bx;
         charge_to_l1a++) {
      tgt->fc().fc_setup_calib(charge_to_l1a);
      pflib_log(info) << "charge_to_l1a = " << tgt->fc().fc_get_setup_calib();
      if (!totscan) {
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
      } else {
        time = (charge_to_l1a - central_charge_to_l1a + offset) * clock_cycle;
        daq_run(tgt, "CHARGE", writer, nevents, pftool::state.daq_rate);
      }
    }
    // reset charge_to_l1a to central value
    tgt->fc().fc_setup_calib(central_charge_to_l1a);
  }
}

void parameter_timescan(Target* tgt) {
  int nevents = pftool::readline_int("How many events per time point? ", 1);
  bool preCC = pftool::readline_bool("Use pre-CC charge injection? ", false);
  bool highrange = false;
  if (!preCC)
    highrange =
        pftool::readline_bool("Use highrange (Y) or lowrange (N)? ", false);
  bool totscan = pftool::readline_bool(
      "Scan TOT? This setting reduced the samples per BX to 1 ", false);
  int toa_threshold = 0;
  int tot_threshold = 0;
  if (totscan)
    toa_threshold = pftool::readline_int("Value for TOA threshold: ", 250);
  if (totscan)
    tot_threshold = pftool::readline_int("Value for TOT threshold: ", 500);
  int calib = pftool::readline_int("Setting for calib pulse amplitude? ",
                                   highrange ? 64 : 1024);
  std::vector<int> channels;
  if (pftool::readline_bool("Pulse into one channel (N) or all channels (Y)?",
                            false)) {
    for (int ch{0}; ch < 72; ch++) channels.push_back(ch);
  } else {
    int channel = pftool::readline_int("Channel to pulse into?", 61);
    channels.push_back(channel);
  }
  int start_bx = pftool::readline_int("Starting BX? ", -1);
  int n_bx = pftool::readline_int("Number of BX? ", 3);
  std::string fname = pftool::readline_path("param-time-scan", ".csv");
  std::array<bool, 2> link{false, false};
  for (int ch : channels)
    link[ch / 36] = true;  // link0 if ch0-35, and 1 if 36-71

  auto roc{tgt->roc(pftool::state.iroc)};
  auto test_param_handle = roc.testParameters();
  auto add_rv = [&](int l) {
    auto refvol_page = pflib::utility::string_format("REFERENCEVOLTAGE_%d", l);
    test_param_handle.add(refvol_page, "CALIB", preCC ? 0 : calib)
        .add(refvol_page, "CALIB_2V5", preCC ? calib : 0)
        .add(refvol_page, "INTCTEST", 1)
        .add(refvol_page, "CHOICE_CINJ", (highrange && !preCC) ? 1 : 0);
    if (totscan) {
      test_param_handle.add(refvol_page, "TOA_VREF", toa_threshold)
          .add(refvol_page, "TOT_VREF", tot_threshold);
    }
  };

  if (link[0]) add_rv(0);
  if (link[1]) add_rv(1);

  for (int ch : channels) {
    auto channel_page = pflib::utility::string_format("CH_%d", ch);
    test_param_handle
        .add(channel_page, "HIGHRANGE", (highrange || preCC) ? 1 : 0)
        .add(channel_page, "LOWRANGE",
             preCC       ? 0
             : highrange ? 0
                         : 1);
  }
  auto test_param = test_param_handle.apply();

  std::filesystem::path parameter_points_file =
      pftool::readline("File of parameter points: ");

  pflib_log(info) << "loading parameter points file...";
  auto [param_names, param_values] =
      load_parameter_points(parameter_points_file);
  pflib_log(info) << "successfully loaded parameter points";

  if (pftool::state.daq_format_mode == Target::DaqFormat::SIMPLEROC) {
    parameter_timescan_writer<pflib::packing::SingleROCEventPacket>(
        tgt, roc, fname, nevents, highrange, preCC, parameter_points_file,
        channels);
  } else if (pftool::state.daq_format_mode ==
             Target::DaqFormat::ECOND_SW_HEADERS) {
    parameter_timescan_writer<pflib::packing::MultiSampleECONDEventPacket>(
        tgt, roc, fname, nevents, highrange, preCC, parameter_points_file,
        channels);
  }

  // DecodeAndWriteToCSV writer{
  //     fname,
  //     [&](std::ofstream& f) {
  //       nlohmann::json header;
  //       header["highrange"] = highrange;
  //       header["preCC"] = preCC;
  //       f << std::boolalpha << "# " << header << '\n' << "time,";
  //       for (const auto& [page, parameter] : param_names) {
  //         f << page << '.' << parameter << ',';
  //       }
  //       f << "channel," << pflib::packing::Sample::to_csv_header << '\n';
  //     },
  //     [&](std::ofstream& f, const pflib::packing::SingleROCEventPacket& ep) {
  //       for (int ch : channels) {
  //         f << time << ',';

  //         for (const auto& val : param_values[i_param_point]) {
  //           f << val << ',';
  //         }
  //         f << ch << ',';

  //         ep.channel(ch).to_csv(f);
  //         f << '\n';
  //       }
  //     }};
  // tgt->setup_run(1 /* dummy - not stored */, Target::DaqFormat::SIMPLEROC,
  //                1 /* dummy */);
  // for (; i_param_point < param_values.size(); i_param_point++) {
  //   // set parameters
  //   auto test_param_builder = roc.testParameters();
  //   // Add implementation for other pages as well
  //   for (std::size_t i_param{0}; i_param < param_names.size(); i_param++) {
  //     const auto& full_page = param_names[i_param].first;
  //     std::string str_page = full_page.substr(0, full_page.find("_"));
  //     const auto& param = param_names[i_param].second;
  //     int value = param_values[i_param_point][i_param];

  //     if (str_page == "CH") {
  //       test_param_builder.add(full_page, param, value);
  //     } else if (str_page == "TOP") {
  //       test_param_builder.add(full_page, param, value);
  //     } else {
  //       for (int l = 0; l < 2; l++) {
  //         if (link[l]) {
  //           test_param_builder.add(str_page + "_" + std::to_string(l), param,
  //                                  value);
  //         }
  //       }
  //     }
  //     pflib_log(info) << param << " = " << value;
  //   }
  //   auto test_param = test_param_builder.apply();
  //   // timescan
  //   auto central_charge_to_l1a = tgt->fc().fc_get_setup_calib();
  //   for (charge_to_l1a = central_charge_to_l1a + start_bx;
  //        charge_to_l1a < central_charge_to_l1a + start_bx + n_bx;
  //        charge_to_l1a++) {
  //     tgt->fc().fc_setup_calib(charge_to_l1a);
  //     pflib_log(info) << "charge_to_l1a = " <<
  //     tgt->fc().fc_get_setup_calib(); if (!totscan) {
  //       for (phase_strobe = 0; phase_strobe < n_phase_strobe; phase_strobe++)
  //       {
  //         auto phase_strobe_test_handle =
  //             roc.testParameters()
  //                 .add("TOP", "PHASE_STROBE", phase_strobe)
  //                 .apply();
  //         pflib_log(info) << "TOP.PHASE_STROBE = " << phase_strobe;
  //         usleep(10);  // make sure parameters are applied
  //         time =
  //             (charge_to_l1a - central_charge_to_l1a + offset) * clock_cycle
  //             - phase_strobe * clock_cycle / n_phase_strobe;
  //         daq_run(tgt, "CHARGE", writer, nevents, pftool::state.daq_rate);
  //       }
  //     } else {
  //       time = (charge_to_l1a - central_charge_to_l1a + offset) *
  //       clock_cycle; daq_run(tgt, "CHARGE", writer, nevents,
  //       pftool::state.daq_rate);
  //     }
  //   }
  //   // reset charge_to_l1a to central value
  //   tgt->fc().fc_setup_calib(central_charge_to_l1a);
  // }
}
