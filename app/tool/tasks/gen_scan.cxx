#include "gen_scan.h"

#include <filesystem>

#include "load_parameter_points.h"
#include "pflib/utility/json.h"
#include "pflib/DecodeAndWrite.h"


ENABLE_LOGGING();

void gen_scan(Target* tgt) {
  static const std::vector<std::string> trigger_types = {
    "PEDESTAL", "CHARGE" //, "LED"
  };
  int nevents = pftool::readline_int("Number of events per parameter point: ", 1);
  int channel = pftool::readline_int("Channel to select: ", 42);
  std::string trigger = pftool::readline("Trigger type: ", trigger_types);

  /**
   * The user can define "scan wide" parameters which are just parameters that
   * are set for the entire scan (and then reset to their values before this command).
   *
   * These scan wide parameters are written into the JSON header of the output CSV file
   * along with the other inputs to this function.
   */
  std::map<std::string, std::map<std::string, int>> scan_wide_params;
  if (pftool::readline_bool("Are there parameters to set constant for the whole scan? ", false)) {
    do {
      std::cout << "Adding a scan-wide parameter..." << std::endl;
      try {
        auto page = pftool::readline("  Page: ", pftool::state.page_names());
        auto param = pftool::readline("  Parameter: ", pftool::state.param_names(page));
        auto val = pftool::readline_int("  Value: ");
        scan_wide_params[page][param] = val;
      } catch (const pflib::Exception& e) {
        pflib_log(error) << "[" << e.name() << "] : " << e.message();
      }
    } while (not pftool::readline_bool("done adding more scan-wide parameters? ", false));
  }

  std::filesystem::path parameter_points_file =
    pftool::readline("File of parameter points: ");

  pflib_log(info) << "loading parameter points file...";
  auto [param_names, param_values ] = load_parameter_points(parameter_points_file);
  pflib_log(info) << "successfully loaded parameter points";

  std::string output_filepath =
    pftool::readline_path(std::string(parameter_points_file.stem()), ".csv");

  auto roc{tgt->hcal().roc(pftool::state.iroc, pftool::state.type_version())};
  boost::json::object header;
  header["parameter_points_file"] = std::string(parameter_points_file);
  header["channel"] = channel;
  header["nevents_per_point"] = nevents;
  header["trigger"] = trigger;
  boost::json::object swp;
  for (const auto& [page, params]: scan_wide_params) {
    boost::json::object page_json;
    for (const auto& [name, val] : params) {
      page_json[name] = val;
    }
    swp[page] = page_json;
  }
  header["scan_wide_params"] = swp;
  auto scan_wide_param_hold = pflib::ROC::TestParameters(
    roc,
    scan_wide_params
  );

  std::size_t i_param_point{0};
  pflib::DecodeAndWriteToCSV writer{
    output_filepath,
    [&](std::ofstream& f) {
      f << std::boolalpha
        << "# " << boost::json::serialize(header) << '\n';
      for (const auto& [ page, parameter ] : param_names) {
        f << page << '.' << parameter << ',';
      }
      f << pflib::packing::Sample::to_csv_header
        << '\n';
    },
    [&](std::ofstream& f, const pflib::packing::SingleROCEventPacket& ep) {
      for (const auto& val : param_values[i_param_point]) {
        f << val << ',';
      }
      ep.channel(channel).to_csv(f);
      f << '\n';
    }
  };
  tgt->setup_run(1 /* dummy - not stored */, Target::DaqFormat::SIMPLEROC, 1 /* dummy */);
  for (; i_param_point < param_values.size(); i_param_point++) {
    auto test_param_builder = roc.testParameters();
    for (std::size_t i_param{0}; i_param < param_names.size(); i_param++) {
      test_param_builder.add(
          param_names[i_param].first,
          param_names[i_param].second,
          param_values[i_param_point][i_param]
      );
    }
    auto test_param = test_param_builder.apply();
    pflib_log(info) << "running test parameter point " << i_param_point << " / " << param_values.size();
    tgt->daq_run(trigger, writer, nevents, pftool::state.daq_rate);
  }
}
