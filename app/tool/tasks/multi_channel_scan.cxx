#include "multi_channel_scan.h"

#include <filesystem>
#include <nlohmann/json.hpp>

#include "../daq_run.h"
#include "../econ_links.h"
#include "load_parameter_points.h"
#include "pflib/utility/string_format.h"

ENABLE_LOGGING();

template <class EventPacket>
static void multi_channel_scan_writer(Target* tgt, pflib::ROC& roc,
                                      size_t n_events, int calib, bool isLED,
                                      int highrange, int link,
                                      std::string fname, int start_bx,
                                      int n_bx) {
  int ch0{0};
  link == 0 ? ch0 = 18 : ch0 = 54;
  int n_links = determine_n_links(tgt);

  if (isLED) {
    auto refvol_page =
        pflib::utility::string_format("REFERENCEVOLTAGE_%d", link);
    auto test_param_handle = roc.testParameters();
    test_param_handle.add(refvol_page, "INTCTEST", 1);
    auto test_param = test_param_handle.apply();

    int phase_strobe{0};
    int charge_to_l1a{0};
    double time{0};
    double clock_cycle{25.0};
    int n_phase_strobe{16};
    int offset{1};
    int nr_channels{-1};
    DecodeAndWriteToCSV<EventPacket> writer{
        fname,
        [&](std::ofstream& f) {
          nlohmann::json header;
          header["highrange"] = highrange;
          header["preCC"] = !highrange;
          header["ledflash"] = isLED;
          f << std::boolalpha << "# " << header << '\n'
            << "nr channels," << "time,";
          f << "channel,";
          f << pflib::packing::Sample::to_csv_header << '\n';
        },
        [&](std::ofstream& f, const EventPacket& ep) {
          for (int ch{0}; ch < 72; ch++) {
            f << nr_channels + 1 << ',';
            f << time << ',';
            f << ch << ',';
            if constexpr (std::is_same_v<
                              EventPacket,
                              pflib::packing::MultiSampleECONDEventPacket>) {
              ep.samples[ep.i_soi].channel(link, ch).to_csv(f);
            } else if constexpr (std::is_same_v<
                                     EventPacket,
                                     pflib::packing::SingleROCEventPacket>) {
              ep.channel(ch).to_csv(f);
            } else {
              PFEXCEPTION_RAISE("BadConf",
                                "Unable to do all_channels_to_csv for the "
                                "currently configured format.");
            }
            f << '\n';
          }},
          n_links // number of links
        };
    tgt->setup_run(1 /* dummy - not stored */, pftool::state.daq_format_mode,
                   1 /* dummy */);
    // Do the scan for increasing amount of channels
    for (nr_channels; nr_channels < 1; nr_channels++) {
      auto test_param_handle = roc.testParameters();
      if (nr_channels == -1) {  // In case of -1, just take pedestal data
        daq_run(tgt, "PEDESTAL", writer, 1, pftool::state.daq_rate);
        continue;
      }
      int central_charge_to_l1a = tgt->fc().fc_get_setup_led();
      for (charge_to_l1a = central_charge_to_l1a + start_bx;
           charge_to_l1a < central_charge_to_l1a + start_bx + n_bx;
           charge_to_l1a++) {
        tgt->fc().fc_setup_led(charge_to_l1a);
        pflib_log(info) << "led_to_l1a = " << tgt->fc().fc_get_setup_led();
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
          daq_run(tgt, "LED", writer, 1, pftool::state.daq_rate);
        }
      }
      // reset charge_to_l1a to central value
      tgt->fc().fc_setup_led(central_charge_to_l1a);
    }
  } else {
    auto refvol_page =
        pflib::utility::string_format("REFERENCEVOLTAGE_%d", link);
    auto test_param_handle = roc.testParameters();
    test_param_handle.add(refvol_page, "CALIB", highrange ? calib : 0)
        .add(refvol_page, "CALIB_2V5", highrange ? 0 : calib)
        .add(refvol_page, "INTCTEST", 1)
        .add(refvol_page, "CHOICE_CINJ", highrange ? 1 : 0);
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
    int nr_channels{-1};
    DecodeAndWriteToCSV<EventPacket> writer{
        fname,
        [&](std::ofstream& f) {
          nlohmann::json header;
          header["highrange"] = highrange;
          header["precc"] = !highrange;
          header["ledflash"] = isLED;
          f << std::boolalpha << "# " << header << '\n'
            << "nr channels," << "time,";
          for (const auto& [page, parameter] : param_names) {
            f << page << '.' << parameter << ',';
          }
          f << "channel,";
          f << pflib::packing::Sample::to_csv_header << '\n';
        },
        [&](std::ofstream& f, const EventPacket& ep) {
          for (int ch{0}; ch < 72; ch++) {
            f << nr_channels + 1 << ',';
            f << time << ',';

            for (const auto& val : param_values[i_param_point]) {
              f << val << ',';
            }
            f << ch << ',';

            if constexpr (std::is_same_v<
                              EventPacket,
                              pflib::packing::MultiSampleECONDEventPacket>) {
              ep.samples[ep.i_soi].channel(link, ch).to_csv(f);
            } else if constexpr (std::is_same_v<
                                     EventPacket,
                                     pflib::packing::SingleROCEventPacket>) {
              ep.channel(ch).to_csv(f);
            } else {
              PFEXCEPTION_RAISE("BadConf",
                                "Unable to do all_channels_to_csv for the "
                                "currently configured format.");
            }
            f << '\n';
          }},
          n_links // number of links
        };
    tgt->setup_run(1 /* dummy - not stored */, pftool::state.daq_format_mode,
                   1 /* dummy */);
    // Do the scan for increasing amount of channels
    for (nr_channels; nr_channels < 18; nr_channels++) {
      pflib_log(info) << "running scan " << nr_channels + 1 << " out of 19";
      auto test_param_handle = roc.testParameters();
      if (nr_channels == -1) {  // In case of -1, just take pedestal data
        daq_run(tgt, "PEDESTAL", writer, n_events, pftool::state.daq_rate);
        continue;
      }
      for (int ch = ch0 - nr_channels; ch <= ch0 + nr_channels; ch++) {
        auto channel_page = pflib::utility::string_format("CH_%d", ch);
        test_param_handle.add(channel_page, "HIGHRANGE", 1);
      }
      auto test_param_two = test_param_handle.apply();
      i_param_point = 0;
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
          pflib_log(info) << param << " = " << value;
        }
        auto test_param_three = test_param_builder.apply();
        // timescan
        int central_charge_to_l1a;
        central_charge_to_l1a = tgt->fc().fc_get_setup_calib();
        for (charge_to_l1a = central_charge_to_l1a + start_bx;
             charge_to_l1a < central_charge_to_l1a + start_bx + n_bx;
             charge_to_l1a++) {
          tgt->fc().fc_setup_calib(charge_to_l1a);
          pflib_log(info) << "charge_to_l1a = "
                          << tgt->fc().fc_get_setup_calib();
          for (phase_strobe = 0; phase_strobe < n_phase_strobe;
               phase_strobe++) {
            auto phase_strobe_test_handle =
                roc.testParameters()
                    .add("TOP", "PHASE_STROBE", phase_strobe)
                    .apply();
            pflib_log(info) << "TOP.PHASE_STROBE = " << phase_strobe;
            usleep(10);  // make sure parameters are applied
            time =
                (charge_to_l1a - central_charge_to_l1a + offset) * clock_cycle -
                phase_strobe * clock_cycle / n_phase_strobe;
            daq_run(tgt, "CHARGE", writer, n_events, pftool::state.daq_rate);
          }
        }
        // reset charge_to_l1a to central value
        tgt->fc().fc_setup_calib(central_charge_to_l1a);
      }
    }
  }
}

void multi_channel_scan(Target * tgt) {
  size_t n_events =
      pftool::readline_int("How many events per time point? ", 1);
  int calib =
      pftool::readline_int("Setting for calib pulse amplitude? ", 256);
  bool isLED = pftool::readline_bool("Use external LED flashes?", false);
  bool highrange =
      pftool::readline_bool("Use highrange (Y) or preCC (N)? ", false);
  int start_bx = pftool::readline_int("Starting BX? ", -1);
  int n_bx = pftool::readline_int("Number of BX? ", 3);
  int link{0};
  pftool::readline_bool("Link 0 [Y] or link 1 [N]", "true") ? link = 0
                                                            : link = 1;
  std::string fname = pftool::readline_path("multi-channel-scan", ".csv");

  pflib::ROC roc{tgt->roc(pftool::state.iroc)};

  if (pftool::state.daq_format_mode == Target::DaqFormat::SIMPLEROC) {
    multi_channel_scan_writer<pflib::packing::SingleROCEventPacket>(
        tgt, roc, n_events, calib, isLED, highrange, link, fname, start_bx,
        n_bx);
  } else if (pftool::state.daq_format_mode ==
             Target::DaqFormat::ECOND_SW_HEADERS) {
    multi_channel_scan_writer<pflib::packing::MultiSampleECONDEventPacket>(
        tgt, roc, n_events, calib, isLED, highrange, link, fname, start_bx,
        n_bx);
  }
}
