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
 * TASKS.CHARGE_TIMESCAN
 *
 * Scan a internal calibration pulse in time by varying the charge_to_l1a
 * and top.phase_strobe parameters
 */
static void charge_timescan(Target* tgt) {
  int nevents = pftool::readline_int("How many events per time point? ", 1);
  bool highrange = pftool::readline_bool("Use highrange (Y) or lowrange (N)? ", false);
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
    .add(refvol_page, "CALIB", calib)
    .add(refvol_page, "INTCTEST", 1)
    .add(refvol_page, "CHOICE_CINJ", highrange ? 1 : 0)
    .add(channel_page, "HIGHRANGE", highrange ? 1 : 0)
    .add(channel_page, "LOWRANGE", highrange ? 0 : 1)
    .apply();
  int phase_strobe{0};
  int charge_to_l1a{0};
  pflib::DecodeAndWriteToCSV writer{
    fname,
    [&](std::ofstream& f) {
      boost::json::object header;
      header["channel"] = channel;
      header["calib"] = calib;
      header["highrange"] = highrange;
      f << std::boolalpha
        << "# " << boost::json::serialize(header) << '\n'
        << "charge_to_l1a,phase_strobe,"
        << pflib::packing::Sample::to_csv_header
        << '\n';
    },
    [&](std::ofstream& f, const pflib::packing::SingleROCEventPacket& ep) {
      f << charge_to_l1a << ','
        << phase_strobe << ',';
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
    for (phase_strobe = 0; phase_strobe < 16; phase_strobe++) {
      auto phase_strobe_test_handle = roc.testParameters()
        .add("TOP", "PHASE_STROBE", phase_strobe)
        .apply();
      pflib_log(info) << "TOP.PHASE_STROBE = " << phase_strobe;
      usleep(10); // make sure parameters are applied
      tgt->daq_run("CHARGE", writer, nevents, pftool::state.daq_rate);
    }
  }
  // reset charge_to_l1a to central value
  tgt->fc().fc_setup_calib(central_charge_to_l1a);
}

/**
 * TASKS.SIX_CHARGE_TIMESCAN
 * 
 * Scan an internal calibration pulse, just like CHARGE_TIMESCAN, but with multiple channels instead.
 * Currently does 6 channels in link 1.
 */
static void six_charge_timescan(Target* tgt) {
  int nevents = pftool::readline_int("How many events per time point? ", 1);
  bool highrange = pftool::readline_bool("Use highrange (Y) or lowrange (N)? ", false);
  int calib = pftool::readline_int("Setting for calib pulse amplitude? ", highrange ? 64 : 1024);
  int channel = pftool::readline_int("Channel to pulse into? ", 61);
  // int n_channel = pftool::readline_int("Number of channels to charge? ", 5);
  int start_bx = pftool::readline_int("Starting BX? ", -1);
  int n_bx = pftool::readline_int("Number of BX? ", 4);
  std::string fname = pftool::readline_path("multi-charge-time-scan", ".csv");
  pflib::ROC roc{tgt->hcal().roc(pftool::state.iroc, pftool::state.type_version())};
  auto channel_page = pflib::utility::string_format("CH_%d", channel);
  int link = (channel / 36);
  auto refvol_page = pflib::utility::string_format("REFERENCEVOLTAGE_%d", link);
  auto test_param_handle = roc.testParameters()
    .add(refvol_page, "CALIB", calib)
    .add(refvol_page, "INTCTEST", 1)
    .add(refvol_page, "CHOICE_CINJ", highrange ? 1 : 0)
    .add(channel_page, "HIGHRANGE", highrange ? 1 : 0)
    .add(channel_page, "LOWRANGE", highrange ? 0 : 1)
    // for (int val=62; val <=66; ++val) {
    //   .add(pflib::utility::string_format("CH_%d", val), "LOWRANGE", highrange ? 0 : 1)
    //   .add(pflib::utility::string_format("CH_%d", val), "HIGHRANGE", highrange ? 1 : 0)
    // })
    .add(pflib::utility::string_format("CH_%d", 62), "LOWRANGE", highrange ? 0 : 1)
    .add(pflib::utility::string_format("CH_%d", 62), "HIGHRANGE", highrange ? 1 : 0)
    .add(pflib::utility::string_format("CH_%d", 63), "LOWRANGE", highrange ? 0 : 1)
    .add(pflib::utility::string_format("CH_%d", 63), "HIGHRANGE", highrange ? 1 : 0)
    .add(pflib::utility::string_format("CH_%d", 64), "LOWRANGE", highrange ? 0 : 1)
    .add(pflib::utility::string_format("CH_%d", 64), "HIGHRANGE", highrange ? 1 : 0)
    .add(pflib::utility::string_format("CH_%d", 65), "LOWRANGE", highrange ? 0 : 1)
    .add(pflib::utility::string_format("CH_%d", 65), "HIGHRANGE", highrange ? 1 : 0)
    .add(pflib::utility::string_format("CH_%d", 66), "LOWRANGE", highrange ? 0 : 1)
    .add(pflib::utility::string_format("CH_%d", 66), "HIGHRANGE", highrange ? 1 : 0)
    .apply();
  int phase_strobe{0};
  int charge_to_l1a{0};
  pflib::DecodeAndWriteToCSV writer{
    fname,
    [&](std::ofstream& f) {
      boost::json::object header;
      header["channel"] = channel;
      header["calib"] = calib;
      header["highrange"] = highrange;
      f << std::boolalpha
        << "# " << boost::json::serialize(header) << '\n'
        << "charge_to_l1a,phase_strobe,"
        << pflib::packing::Sample::to_csv_header
        << '\n';
    },
    [&](std::ofstream& f, const pflib::packing::SingleROCEventPacket& ep) {
      f << charge_to_l1a << ','
        << phase_strobe << ',';
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
    for (phase_strobe = 0; phase_strobe < 16; phase_strobe++) {
      auto phase_strobe_test_handle = roc.testParameters()
        .add("TOP", "PHASE_STROBE", phase_strobe)
        .apply();
      pflib_log(info) << "TOP.PHASE_STROBE = " << phase_strobe;
      usleep(10); // make sure parameters are applied
      tgt->daq_run("CHARGE", writer, nevents, pftool::state.daq_rate);
    }
  }
  // reset charge_to_l1a to central value
  tgt->fc().fc_setup_calib(central_charge_to_l1a);
}

/**
 * TASKS.TWENTY_CHARGE_TIMESCAN
 * 
 * Scan an internal calibration pulse, just like CHARGE_TIMESCAN, but with multiple channels instead.
 * Currently does 20 channels in link 1.
 */
static void twenty_charge_timescan(Target* tgt) {
  int nevents = pftool::readline_int("How many events per time point? ", 1);
  bool highrange = pftool::readline_bool("Use highrange (Y) or lowrange (N)? ", false);
  int calib = pftool::readline_int("Setting for calib pulse amplitude? ", highrange ? 64 : 1024);
  int channel = pftool::readline_int("Channel to pulse into? ", 61);
  int start_bx = pftool::readline_int("Starting BX? ", -1);
  int n_bx = pftool::readline_int("Number of BX? ", 4);
  std::string fname = pftool::readline_path("twenty-charge-time-scan", ".csv");
  pflib::ROC roc{tgt->hcal().roc(pftool::state.iroc, pftool::state.type_version())};
  auto channel_page = pflib::utility::string_format("CH_%d", channel);
  int link = (channel / 36);
  auto refvol_page = pflib::utility::string_format("REFERENCEVOLTAGE_%d", link);
  auto test_param_builder = roc.testParameters()
    .add(refvol_page, "CALIB", calib)
    .add(refvol_page, "INTCTEST", 1)
    .add(refvol_page, "CHOICE_CINJ", highrange ? 1 : 0)
    .add(channel_page, "HIGHRANGE", highrange ? 1 : 0)
    .add(channel_page, "LOWRANGE", highrange ? 0 : 1);
  // Ensure added channels are on the same link. Then loop over them.
  if (link == 0) {
    for (int val = 0; val <= 20; ++val) {
      test_param_builder.add(pflib::utility::string_format("CH_%d", val), "LOWRANGE", highrange ? 0 : 1)
                        .add(pflib::utility::string_format("CH_%d", val), "HIGHRANGE", highrange ? 1 : 0);
    }
  }
  else if (link == 1) {
    for (int val = 36; val <= 56; ++val) {
      test_param_builder.add(pflib::utility::string_format("CH_%d", val), "LOWRANGE", highrange ? 0 : 1)
                        .add(pflib::utility::string_format("CH_%d", val), "HIGHRANGE", highrange ? 1 : 0);
    }
  }
  auto test_param_handle = test_param_builder.apply();
  int phase_strobe{0};
  int charge_to_l1a{0};
  pflib::DecodeAndWriteToCSV writer{
    fname,
    [&](std::ofstream& f) {
      boost::json::object header;
      header["channel"] = channel;
      header["calib"] = calib;
      header["highrange"] = highrange;
      f << std::boolalpha
        << "# " << boost::json::serialize(header) << '\n'
        << "charge_to_l1a,phase_strobe,"
        << pflib::packing::Sample::to_csv_header
        << '\n';
    },
    [&](std::ofstream& f, const pflib::packing::SingleROCEventPacket& ep) {
      f << charge_to_l1a << ','
        << phase_strobe << ',';
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
    for (phase_strobe = 0; phase_strobe < 16; phase_strobe++) {
      auto phase_strobe_test_handle = roc.testParameters()
        .add("TOP", "PHASE_STROBE", phase_strobe)
        .apply();
      pflib_log(info) << "TOP.PHASE_STROBE = " << phase_strobe;
      usleep(10); // make sure parameters are applied
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
      parameter_points_file,
      [&param_names,&param_values,&parameter_points_file]
      (const std::vector<int>& row) {
        if (row.size() != param_names.size()) {
          PFEXCEPTION_RAISE("BadRow",
              "A row in "+std::string(parameter_points_file)
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

static void trim_inv_scan(Target* tgt) {
  int nevents = pftool::readline_int("Number of events per point: ", 1);
  int ch_start = pftool::readline_int("Start channel (m): ", 0);
  int ch_end   = pftool::readline_int("End channel (n): ", 71);
  std::vector<std::string> trigger_types = { "PEDESTAL", "CHARGE" };
  std::string trigger = pftool::readline("Trigger type: ", trigger_types);

  std::string output_filepath = pftool::readline_path("trim_inv_scan", ".csv");

  auto roc = tgt->hcal().roc(pftool::state.iroc, pftool::state.type_version());

  boost::json::object header;
  header["scan_type"] = "CH_#.TRIM_INV sweep";
  header["trigger"] = trigger;
  header["nevents_per_point"] = nevents;
  header["channel_range"] = std::to_string(ch_start) + "-" + std::to_string(ch_end);

  pflib::DecodeAndWriteToCSV writer{
    output_filepath,
    [&](std::ofstream& f) {
      f << std::boolalpha
        << "# " << boost::json::serialize(header) << '\n';
      f << "channel,TRIM_INV," << pflib::packing::Sample::to_csv_header << '\n';
    },
    [&](std::ofstream& f, const pflib::packing::SingleROCEventPacket& ep) {
      // Only write data from the current channel
      f << header["current_channel"].as_int64() << ',' << header["current_trim_inv"].as_int64() << ',';
      ep.channel(header["current_channel"].as_int64()).to_csv(f);
      f << '\n';
    }
  };

  tgt->setup_run(1 /* dummy */, DAQ_FORMAT_SIMPLEROC, 1 /* dummy */);

  for (int ch = ch_start; ch <= ch_end; ++ch) {
    pflib_log(info) << "Running CH_" << ch;
    for (int trim = 0; trim <= 128; ++trim) {
      //pflib_log(info) << "Running CH_" << ch << ".TRIM_INV = " << trim;

      // Set the test parameter
      auto test_param = roc.testParameters()
        .add("CH_" + std::to_string(ch), "TRIM_INV", trim)
        .apply();

      // Store current scan state in header for writer access
      header["current_channel"] = ch;
      header["current_trim_inv"] = trim;

      tgt->daq_run(trigger, writer, nevents, pftool::state.daq_rate);
    }
  }
}

/**
 * TASKS.SAMPLING_PHASE_SCAN
 * 
 * Scan over phase_ck, check pedestal many times for each, for 10 channels.
 */
static void sampling_phase_scan(Target* tgt) {
  int nevents = pftool::readline_int("How many events per time point? ", 1);
  int channel = pftool::readline_int("First channel to pedestal? ", 0);
  int npedestals = pftool::readline_int("Number of pedestals per phase_ck? ", 1000);
  // std::vector<std::string> trigger_types = { "PEDESTAL", "CHARGE" };
  // std::string trigger = pftool::readline("Trigger type: ", trigger_types);
  std::string fname = pftool::readline_path("sampling-phase-scan", ".csv");
  pflib::ROC roc{tgt->hcal().roc(pftool::state.iroc, pftool::state.type_version())};

  boost::json::object header;
  header["scan_type"] = "CH_#.phase_ck scan";
  header["trigger"] = "PEDESTAL"; // hardcoded for now
  header["nevents_per_point"] = nevents;
  header["channel"] = channel;

  pflib::DecodeAndWriteToCSV writer{
    fname, //output file name
    [&](std::ofstream& f) {
      f << std::boolalpha
        << "# " << boost::json::serialize(header) << '\n'
        << "channel,phase_ck,"
        << pflib::packing::Sample::to_csv_header
        << '\n';
    },
    [&](std::ofstream& f, const pflib::packing::SingleROCEventPacket& ep) {
      // Writing channels and phase_ck to the CSV
      f << header["current_channel"].as_int64() << ',' << header["current_phase_ck"].as_int64() << ',';
      ep.channel(header["current_channel"].as_int64()).to_csv(f);
      f << '\n';
    } 
  };

  tgt->setup_run(1 /* dummy - not stored */, DAQ_FORMAT_SIMPLEROC, 1 /* dummy */);
  
  // Loop over phases, then loop over channels and do pedestals.
  for (int phase = 0; phase <= 15; ++phase) {
    pflib_log(info) << "Scanning phase_ck = " << phase;
    auto phase_test_handle = roc.testParameters()
      .add("TOP", "PHASE_CK", phase)
      .apply();
    for (int ch = channel; ch < channel + 10; ++ch) {
      pflib_log(info) << "Running channel " << ch;
      for (int pedestal = 0; pedestal < npedestals; ++pedestal) {
        pflib_log(info) << "Pedestal # = " << pedestal;
        // Store current scan state in header for writer access
        header["current_channel"] = ch;
        header["current_phase_ck"] = phase;
      
        tgt->daq_run("PEDESTAL", writer, nevents, pftool::state.daq_rate);
      }
    }
  }
}







namespace {
auto menu_tasks =
    pftool::menu("TASKS", "tasks for studying the chip and tuning its parameters")
        ->line("CHARGE_TIMESCAN", "scan charge/calib pulse over time", charge_timescan)
        ->line("SIX_CHARGE_TIMESCAN", "scan charge/calib pulse over time, 6 channels", six_charge_timescan)
        ->line("GEN_SCAN", "scan over file of input parameter points", gen_scan)
        ->line("TRIM_INV_SCAN", "scan trim_inv over a range of channels", trim_inv_scan)
        ->line("TWENTY_CHARGE_TIMESCAN", "scan charge/calib pulse over time, first 20 channels in link", twenty_charge_timescan)
        ->line("SAMPLING_PHASE_SCAN", "scan over phase_ck, pedestal trigger 16 times", sampling_phase_scan);
}
