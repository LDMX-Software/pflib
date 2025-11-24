#include "gen_scan.h"

#include <filesystem>
#include <nlohmann/json.hpp>

#include "../daq_run.h"
#include "load_parameter_points.h"

ENABLE_LOGGING();

// helper function to facilitate EventPacket dependent behaviour
template <class EventPacket>
static void charge_timescan_writer(Target* tgt, pflib::ROC& roc, size_t nevents, std::string& output_filepath, int channel, nlohmann::json& header){
  std::size_t i_param_point{0};

  pflib_log(info) << "loading parameter points file...";
  auto [param_names, param_values] =
      load_parameter_points(parameter_points_file);
  pflib_log(info) << "successfully loaded parameter points";

  DecodeAndWriteToCSV<EventPacket> writer{
  output_filepath,
      [&](std::ofstream& f) {
        f << std::boolalpha << "# " << header << '\n';
        for (const auto& [page, parameter] : param_names) {
          f << page << '.' << parameter << ',';
        }
        f << pflib::packing::Sample::to_csv_header << '\n';
      },
      [&](std::ofstream& f, const EventPacket& ep) {
        for (const auto& val : param_values[i_param_point]) {
          f << val << ',';
        }
        if constexpr (std::is_same_v<EventPacket,pflib::packing::MultiSampleECONDEventPacket>){
          ep.samples[ep.i_soi].channel(link, i_ch).to_csv(f);
        } else if constexpr (std::is_same_v<EventPacket,pflib::packing::SingleROCEventPacket>){
          ep.channel(channel).to_csv(f);
        } else {
          PFEXCEPTION_RAISE("BadConf", "Unable to do all_channels_to_csv for the currently configured format.");
        }      
        f << '\n';
      }
    }

  tgt->setup_run(1 /* dummy - not stored */, pftool::state.daq_format_mode, 1 /* dummy */);
  for (; i_param_point < param_values.size(); i_param_point++) {
    auto test_param_builder = roc.testParameters();
    for (std::size_t i_param{0}; i_param < param_names.size(); i_param++) {
      test_param_builder.add(param_names[i_param].first,
                             param_names[i_param].second,
                             param_values[i_param_point][i_param]);
    }
    auto test_param = test_param_builder.apply();
    pflib_log(info) << "running test parameter point " << i_param_point << " / "
                    << param_values.size();
    daq_run(tgt, trigger, writer, nevents, pftool::state.daq_rate);
  }
}

void gen_scan(Target* tgt) {
  static const std::vector<std::string> trigger_types = {
      "PEDESTAL", "CHARGE"  //, "LED"
  };
  int nevents =
      pftool::readline_int("Number of events per parameter point: ", 1);
  int channel = pftool::readline_int("Channel to select: ", 42);
  std::string trigger = pftool::readline("Trigger type: ", trigger_types);

  /**
   * The user can define "scan wide" parameters which are just parameters that
   * are set for the entire scan (and then reset to their values before this
   * command).
   *
   * These scan wide parameters are written into the JSON header of the output
   * CSV file along with the other inputs to this function.
   */
  std::map<std::string, std::map<std::string, uint64_t>> scan_wide_params;
  if (pftool::readline_bool(
          "Are there parameters to set constant for the whole scan? ", false)) {
    do {
      std::cout << "Adding a scan-wide parameter..." << std::endl;
      try {
        auto page =
            pftool::readline("  Page: ", pftool::state.roc_page_names());
        auto param = pftool::readline("  Parameter: ",
                                      pftool::state.roc_param_names(page));
        auto val = pftool::readline_int("  Value: ");
        scan_wide_params[page][param] = val;
      } catch (const pflib::Exception& e) {
        pflib_log(error) << "[" << e.name() << "] : " << e.message();
      }
    } while (not pftool::readline_bool(
        "done adding more scan-wide parameters? ", false));
  }

  std::filesystem::path parameter_points_file =
      pftool::readline("File of parameter points: ");

  std::string output_filepath =
      pftool::readline_path(std::string(parameter_points_file.stem()), ".csv");

  auto roc{tgt->roc(pftool::state.iroc)};
  nlohmann::json header;
  header["parameter_points_file"] = std::string(parameter_points_file);
  header["channel"] = channel;
  header["nevents_per_point"] = nevents;
  header["trigger"] = trigger;
  for (const auto& [page, params] : scan_wide_params) {
    for (const auto& [name, val] : params) {
      header["scan_wide_params"][page][name] = val;
    }
  }
  auto scan_wide_param_hold = pflib::ROC::TestParameters(roc, scan_wide_params);

  // call helper function to conuduct the scan
  if (pftool::state.daq_format_mode == Target::DaqFormat::SIMPLEROC) {
    gen_scan_writer<pflib::packing::SingleROCEventPacket>(  );
  } else if (pftool::state.daq_format_mode ==
             Target::DaqFormat::ECOND_SW_HEADERS) {
    gen_scan_writer<pflib::packing::MultiSampleECONDEventPacket>(
        tgt, roc, nevents, output_filepath, channel);
  }

  // DecodeAndWriteToCSV writer{
  //     output_filepath,
  //     [&](std::ofstream& f) {
  //       f << std::boolalpha << "# " << header << '\n';
  //       for (const auto& [page, parameter] : param_names) {
  //         f << page << '.' << parameter << ',';
  //       }
  //       f << pflib::packing::Sample::to_csv_header << '\n';
  //     },
  //     [&](std::ofstream& f, const pflib::packing::SingleROCEventPacket& ep) {
  //       for (const auto& val : param_values[i_param_point]) {
  //         f << val << ',';
  //       }
  //       ep.channel(channel).to_csv(f);
  //       f << '\n';
  //     }};
  // tgt->setup_run(1 /* dummy - not stored */, Target::DaqFormat::SIMPLEROC,
  //                1 /* dummy */);
  // for (; i_param_point < param_values.size(); i_param_point++) {
  //   auto test_param_builder = roc.testParameters();
  //   for (std::size_t i_param{0}; i_param < param_names.size(); i_param++) {
  //     test_param_builder.add(param_names[i_param].first,
  //                            param_names[i_param].second,
  //                            param_values[i_param_point][i_param]);
  //   }
  //   auto test_param = test_param_builder.apply();
  //   pflib_log(info) << "running test parameter point " << i_param_point << " / "
  //                   << param_values.size();
  //   daq_run(tgt, trigger, writer, nevents, pftool::state.daq_rate);
  // }

}
