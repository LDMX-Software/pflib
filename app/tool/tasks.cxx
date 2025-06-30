/**
 * @file tasks.cxx
 *
 * Definition of TASKS menu commands
 */
#include "pftool.h"

ENABLE_LOGGING();

#include <filesystem>

#include "pflib/utility/string_format.h"
#include "pflib/utility/load_integer_csv.h"
#include "pflib/utility/json.h"
#include "pflib/DecodeAndWrite.h"

/**
 * load a CSV of parameter points into member
 *
 * @param[in] filepath path to CSV containing parameter points
 * @return 2-tuple with parameter names and values
 */
static
std::tuple<std::vector<std::pair<std::string,std::string>>, std::vector<std::vector<int>>>
load_parameter_points(const std::string& filepath) {
  /**
   * The input parameter points file is just a CSV where the header
   * is used to define the parameters that will be set and the rows
   * are the values of those parameters.
   *
   * For example, the CSV
   * ```csv
   * page.param1,page.param2
   * 1,2
   * 3,4
   * ```
   * would produce two runs with this command where the parameter settings are
   * 1. page.param1 = 1 and page.param2 = 2
   * 2. page.param1 = 3 and page.param2 = 4
   */
  std::vector<std::pair<std::string,std::string>> param_names;
  std::vector<std::vector<int>> param_values;
  pflib::utility::load_integer_csv(
    filepath,
    [&param_names,&param_values,&filepath]
    (const std::vector<int>& row) {
      if (row.size() != param_names.size()) {
        PFEXCEPTION_RAISE("BadRow",
            "A row in "+std::string(filepath)
            +" contains "+std::to_string(row.size())
            +" cells which is not "+std::to_string(param_names.size())
            +" the number of parameters defined in the header.");
      }
      param_values.push_back(row);
    },
    [&param_names](const std::vector<std::string>& header) {
      param_names.resize(header.size());
      for (std::size_t i{0}; i < header.size(); i++) {
        const auto& param_fullname = header[i];
        auto dot = param_fullname.find(".");
        if (dot == std::string::npos) {
          PFEXCEPTION_RAISE("BadParam",
              "Header cell "+param_fullname+" does not contain a '.' "
              "separating the page and parameter names.");
        }
        param_names[i] = {
          param_fullname.substr(0, dot),
          param_fullname.substr(dot+1)
        };
        pflib_log(debug) << "parameter " << i << " is "
          << param_names[i].first << "." << param_names[i].second;
      }
    }
  ); 
  return std::make_tuple(param_names, param_values);
}

/**
 * TASKS.CHARGE_TIMESCAN
 *
 * Scan a internal calibration pulse in time by varying the charge_to_l1a
 * and top.phase_strobe parameters
 */
static void charge_timescan(Target* tgt) {
  int nevents = pftool::readline_int("How many events per time point? ", 1);
  bool preCC = pftool::readline_bool("Use pre-CC charge injection? ", false);
  bool highrange = false;
  if (!preCC) highrange = pftool::readline_bool("Use highrange (Y) or lowrange (N)? ", false);
  int calib = pftool::readline_int("Setting for calib pulse amplitude? ", highrange ? 64 : 1024);
  int channel = pftool::readline_int("Channel to pulse into? ", 61);
  int start_bx = pftool::readline_int("Starting BX? ", -1);
  int n_bx = pftool::readline_int("Number of BX? ", 3);
  std::string fname = pftool::readline_path("charge-time-scan", ".csv");
  pflib::ROC roc{tgt->hcal().roc(pftool::state.iroc, pftool::state.type_version())};
  auto channel_page = pflib::utility::string_format("CH_%d", channel);
  int link = (channel / 36);
  auto refvol_page = pflib::utility::string_format("REFERENCEVOLTAGE_%d", link);
  auto test_param_handle = roc.testParameters()
    .add(refvol_page, "CALIB", preCC ? 0 : calib)
    .add(refvol_page, "CALIB_2V5", preCC ? calib : 0)
    .add(refvol_page, "INTCTEST", 1)
    .add(refvol_page, "CHOICE_CINJ", (highrange && !preCC) ? 1 : 0)
    .add(channel_page, "HIGHRANGE", (highrange || preCC) ? 1 : 0)
    .add(channel_page, "LOWRANGE", preCC ? 0 : highrange ? 0 : 1)
    .apply();
  int phase_strobe{0};
  int charge_to_l1a{0};
  double time{0};
  double clock_cycle{25.0};
  int n_phase_strobe{16};
  int offset{1};
  pflib::DecodeAndWriteToCSV writer{
    fname,
    [&](std::ofstream& f) {
      boost::json::object header;
      header["channel"] = channel;
      header["calib"] = calib;
      header["highrange"] = highrange;
      f << std::boolalpha
        << "# " << boost::json::serialize(header) << '\n'
        << "time,"
        << pflib::packing::Sample::to_csv_header
        << '\n';
    },
    [&](std::ofstream& f, const pflib::packing::SingleROCEventPacket& ep) {
      f << time << ',';
      ep.channel(channel).to_csv(f);
      f << '\n';
    } 
  };
  tgt->setup_run(1 /* dummy - not stored */, DAQ_FORMAT_SIMPLEROC, 1 /* dummy */);
  auto central_charge_to_l1a = tgt->fc().fc_get_setup_calib();
  for (charge_to_l1a = central_charge_to_l1a+start_bx;
       charge_to_l1a < central_charge_to_l1a+start_bx+n_bx; charge_to_l1a++) {
    tgt->fc().fc_setup_calib(charge_to_l1a);
    pflib_log(info) << "charge_to_l1a = " << tgt->fc().fc_get_setup_calib();
    for (phase_strobe = 0; phase_strobe < n_phase_strobe; phase_strobe++) {
      auto phase_strobe_test_handle = roc.testParameters()
        .add("TOP", "PHASE_STROBE", phase_strobe)
        .apply();
      pflib_log(info) << "TOP.PHASE_STROBE = " << phase_strobe;
      usleep(10); // make sure parameters are applied
      time = 
        (charge_to_l1a - central_charge_to_l1a + offset) * clock_cycle
        - phase_strobe * clock_cycle/n_phase_strobe;
      tgt->daq_run("CHARGE", writer, nevents, pftool::state.daq_rate);
    }
  }
  // reset charge_to_l1a to central value
  tgt->fc().fc_setup_calib(central_charge_to_l1a);
}

/**
 * TASKS.GEN_SCAN
 *
 * Generalized scan where the parameter points to test are input by file
 */
static void gen_scan(Target* tgt) {
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
  tgt->setup_run(1 /* dummy - not stored */, DAQ_FORMAT_SIMPLEROC, 1 /* dummy */);
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

/**
 * TASKS.TRIM_INV_SCAN
 * 
 * Scan TRIM_INV Parameter across all 63 parameter points for each channel
 * Check ADC Pedstals for each parameter point
 * Used to trim pedestals within each link
 */
static void trim_inv_scan(Target* tgt) {
  int nevents = pftool::readline_int("Number of events per point: ", 1);

  std::string output_filepath = pftool::readline_path("trim_inv_scan", ".csv");

  auto roc = tgt->hcal().roc(pftool::state.iroc, pftool::state.type_version());

  boost::json::object header;
  header["scan_type"] = "CH_#.TRIM_INV sweep";
  header["trigger"] = "PEDESTAL";
  header["nevents_per_point"] = nevents;

  pflib::DecodeAndWriteToCSV writer{
    output_filepath,
    [&](std::ofstream& f) {
      f << std::boolalpha
        << "# " << boost::json::serialize(header) << '\n';
      f << "channel,TRIM_INV," << pflib::packing::Sample::to_csv_header << '\n';
    },
    [&](std::ofstream& f, const pflib::packing::SingleROCEventPacket &ep) {
      for (int ch{0}; ch < 72; ch++) {
        f << ch << ',' << header["trim_inv"].as_int64() << ',';
        ep.channel(ch).to_csv(f);
        f << '\n';
      }
    }
  };

  tgt->setup_run(1 /* dummy */, DAQ_FORMAT_SIMPLEROC, 1 /* dummy */);

  //take pedestal run on each parameter point
  for (int trim = 0; trim <= 63; trim+=4) {
    pflib_log(info) << "Running TRIM_INV = " << trim;
    auto trim_inv_test_builder = roc.testParameters();
    for (int ch{0}; ch < 72; ch++) {
      trim_inv_test_builder.add("CH_"+std::to_string(ch), "TRIM_INV", trim);
    }
    auto trim_inv_test = trim_inv_test_builder.apply();
    header["trim_inv"] = trim;
    tgt->daq_run("PEDESTAL", writer, nevents, pftool::state.daq_rate);
    
  }
}

/**
 * TASKS.INV_VREF_SCAN
 * 
 * Perform INV_VREF scan for each link 
 * used to trim adc pedestals between two links
 */
static void inv_vref_scan(Target* tgt) {
  int nevents = pftool::readline_int("Number of events per point: ", 1);

  std::string output_filepath = pftool::readline_path("inv_vref_scan", ".csv");

  auto roc = tgt->hcal().roc(pftool::state.iroc, pftool::state.type_version());

  boost::json::object header;
  header["scan_type"] = "CH_#.inv_vref_scan";
  header["trigger"] = "PEDESTAL";
  header["nevents_per_point"] = nevents;
  header["channel"] = 0;

  pflib::DecodeAndWriteToCSV writer{
    output_filepath,
    [&](std::ofstream& f) {
      f << std::boolalpha
        << "# " << boost::json::serialize(header) << '\n';
      f << "channel,INV_VREF," << pflib::packing::Sample::to_csv_header << '\n';
    },
    [&](std::ofstream& f, const pflib::packing::SingleROCEventPacket &ep) {
    for (int ch{0}; ch < 72; ch++) {
      f << ch << ',' << header["inv_vref"].as_int64() << ',';
      ep.channel(ch).to_csv(f);
      f << '\n';
    }
    }
  };

  tgt->setup_run(1 /* dummy */, DAQ_FORMAT_SIMPLEROC, 1 /* dummy */);

  //set global params HZ_noinv on each link (arbitrary channel on each)
    auto test_param = roc.testParameters()
      .add("CH_17", "HZ_NOINV", 1)
      .add("CH_53", "HZ_NOINV", 1)
      .apply();

  //increment inv_vref in increments of 20. 10 bit value but only scanning to 600
  for (int inv_vref = 0; inv_vref <= 600; inv_vref += 20) {
    pflib_log(info) << "Running INV_VREF = " << inv_vref;
    //set inv_vref simultaneously for both links
    auto test_param = roc.testParameters()
      .add("REFERENCEVOLTAGE_0", "INV_VREF", inv_vref)
      .add("REFERENCEVOLTAGE_1", "INV_VREF", inv_vref)
      .apply();
      //store current scan state in header for writer access
      header["inv_vref"] = inv_vref;
      tgt->daq_run("PEDESTAL", writer, nevents, pftool::state.daq_rate);
  }
}


/**
   * TASKS.NOINV_VREF_SCAN
   * 
   * Perform NOINV_VREF scan for each link 
   * used to trim adc pedestals between two links
   * same as inv_vref_scan with noinv_vref parameter instead
   */
static void noinv_vref_scan(Target* tgt) {
  int nevents = pftool::readline_int("Number of events per point: ", 1);

  std::string output_filepath = pftool::readline_path("noinv_vref_scan", ".csv");

  auto roc = tgt->hcal().roc(pftool::state.iroc, pftool::state.type_version());

  boost::json::object header;
  header["scan_type"] = "CH_#.noinv_vref_scan";
  header["trigger"] = "PEDESTAL";
  header["nevents_per_point"] = nevents;
  header["channel"] = 0;

  pflib::DecodeAndWriteToCSV writer{
    output_filepath,
    [&](std::ofstream& f) {
      f << std::boolalpha
        << "# " << boost::json::serialize(header) << '\n';
      f << "channel,NOINV_VREF," << pflib::packing::Sample::to_csv_header << '\n';
    },
    [&](std::ofstream& f, const pflib::packing::SingleROCEventPacket &ep) {
    for (int ch{0}; ch < 72; ch++) {
      f << ch << ',' << header["noinv_vref"].as_int64() << ',';
      ep.channel(ch).to_csv(f);
      f << '\n';
    }
    }
  };


  tgt->setup_run(1 /* dummy */, DAQ_FORMAT_SIMPLEROC, 1 /* dummy */);

  //set global params HZ_INV on each link (arbitrary channel on each)
    auto test_param = roc.testParameters()
      .add("CH_17", "HZ_INV", 1)
      .add("CH_53", "HZ_INV", 1)
      .apply();

  //increment noinv_vref in increments of 20. 10 bit value but only scanning to 600
  for (int noinv_vref = 0; noinv_vref <= 600; noinv_vref += 20) {
    pflib_log(info) << "Running NOINV_VREF = " << noinv_vref;
    //set noinv_vref simultaneously for both links
    auto test_param = roc.testParameters()
      .add("REFERENCEVOLTAGE_0", "NOINV_VREF", noinv_vref)
      .add("REFERENCEVOLTAGE_1", "NOINV_VREF", noinv_vref)
      .apply();
      header["noinv_vref"] = noinv_vref;
      
      tgt->daq_run("PEDESTAL", writer, nevents, pftool::state.daq_rate);
  }
}



/*
 * TASKS.PARAMETER_TIMESCAN
 *
 * Scan a internal calibration pulse in time by varying the charge_to_l1a
 * and top.phase_strobe parameters, and then change given parameters pulse-wise.
 * In essence, an implementation of gen_scan into charge_timescan, to see how pulse shapes
 * transform with varying parameters.
 *
 */
static void parameter_timescan(Target* tgt) {
  int nevents = pftool::readline_int("How many events per time point? ", 1);
  bool preCC = pftool::readline_bool("Use pre-CC charge injection? ", false);
  bool highrange = false;
  if (!preCC) highrange = pftool::readline_bool("Use highrange (Y) or lowrange (N)? ", false);
  int calib = pftool::readline_int("Setting for calib pulse amplitude? ", highrange ? 64 : 1024);
  int channel = pftool::readline_int("Channel to pulse into? ", 61);
  int start_bx = pftool::readline_int("Starting BX? ", -1);
  int n_bx = pftool::readline_int("Number of BX? ", 3);
  std::string fname = pftool::readline_path("param-time-scan", ".csv");
  auto roc{tgt->hcal().roc(pftool::state.iroc, pftool::state.type_version())};
  auto channel_page = pflib::utility::string_format("CH_%d", channel);
  int link = (channel / 36);
  auto refvol_page = pflib::utility::string_format("REFERENCEVOLTAGE_%d", link);
  auto test_param_handle = roc.testParameters()
    .add(refvol_page, "CALIB", preCC ? 0 : calib)
    .add(refvol_page, "CALIB_2V5", preCC ? calib : 0)
    .add(refvol_page, "INTCTEST", 1)
    .add(refvol_page, "CHOICE_CINJ", (highrange && !preCC) ? 1 : 0)
    .add(channel_page, "HIGHRANGE", (highrange || preCC) ? 1 : 0)
    .add(channel_page, "LOWRANGE", preCC ? 0 : highrange ? 0 : 1)
    .apply();

  std::filesystem::path parameter_points_file =
    pftool::readline("File of parameter points: ");

  pflib_log(info) << "loading parameter points file...";
  auto [param_names, param_values ] = load_parameter_points(parameter_points_file);
  pflib_log(info) << "successfully loaded parameter points";

  int phase_strobe{0};
  int charge_to_l1a{0};
  double time{0};
  double clock_cycle{25.0};
  int n_phase_strobe{16};
  int offset{1};
  std::size_t i_param_point{0};
  pflib::DecodeAndWriteToCSV writer{
    fname,
    [&](std::ofstream& f) {
      boost::json::object header;
      header["channel"] = channel;
      header["highrange"] = highrange;
      header["preCC"] = preCC;
      f << std::boolalpha
        << "# " << boost::json::serialize(header) << '\n'
        << "time,";
      for (const auto& [ page, parameter ] : param_names) {
        f << page << '.' << parameter << ',';
      }
      f << pflib::packing::Sample::to_csv_header
        << '\n';
    },
    [&](std::ofstream& f, const pflib::packing::SingleROCEventPacket& ep) {
      f << time << ',';
      for (const auto& val : param_values[i_param_point]) {
        f << val << ',';
      }
      ep.channel(channel).to_csv(f);
      f << '\n';
    } 
  };
  tgt->setup_run(1 /* dummy - not stored */, DAQ_FORMAT_SIMPLEROC, 1 /* dummy */);
  for (; i_param_point < param_values.size(); i_param_point++) {
    // set parameters
    auto test_param_builder = roc.testParameters();
    // Add implementation for other pages as well
    for (std::size_t i_param{0}; i_param < param_names.size(); i_param++) {
      test_param_builder.add(
      (param_names[i_param].first == "REFERENCEVOLTAGE_0" ? refvol_page : 
       param_names[i_param].first == "REFERENCEVOLTAGE_1" ? refvol_page :
       param_names[i_param].first),
      param_names[i_param].second,
      param_values[i_param_point][i_param]
      );
      pflib_log(info) << param_names[i_param].second << " = " << param_values[i_param_point][i_param];
    }
    auto test_param = test_param_builder.apply();
    // timescan
    auto central_charge_to_l1a = tgt->fc().fc_get_setup_calib();
    for (charge_to_l1a = central_charge_to_l1a+start_bx;
      charge_to_l1a < central_charge_to_l1a+start_bx+n_bx; charge_to_l1a++) {
      tgt->fc().fc_setup_calib(charge_to_l1a);
      pflib_log(info) << "charge_to_l1a = " << tgt->fc().fc_get_setup_calib();
      for (phase_strobe = 0; phase_strobe < n_phase_strobe; phase_strobe++) {
        auto phase_strobe_test_handle = roc.testParameters()
            .add("TOP", "PHASE_STROBE", phase_strobe)
            .apply();
        pflib_log(info) << "TOP.PHASE_STROBE = " << phase_strobe;
        usleep(10); // make sure parameters are applied
        time = 
          (charge_to_l1a - central_charge_to_l1a + offset) * clock_cycle
          - phase_strobe * clock_cycle/n_phase_strobe;
        tgt->daq_run("CHARGE", writer, nevents, pftool::state.daq_rate);
      }
    }
    // reset charge_to_l1a to central value
    tgt->fc().fc_setup_calib(central_charge_to_l1a);
  }
}

namespace {
auto menu_tasks =
    pftool::menu("TASKS", "tasks for studying the chip and tuning its parameters")
        ->line("CHARGE_TIMESCAN", "scan charge/calib pulse over time", charge_timescan)
        ->line("GEN_SCAN", "scan over file of input parameter points", gen_scan)
        ->line("PARAMETER_TIMESCAN", "scan charge/calib pulse over time for varying parameters", parameter_timescan)
        ->line("TRIM_INV_SCAN", "scan trim_inv parameter", trim_inv_scan)
        ->line("INV_VREF_SCAN", "scan over INV_VREF parameter", inv_vref_scan)
        ->line("NOINV_VREF_SCAN", "scan over NOINV_VREF parameter", noinv_vref_scan)
;
}
