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
#include "pflib/DecodeAndBuffer.h"

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

/*
 * TASKS.SET_TOA
 *
 * Do a charge injection run for a given channel, find the toa efficiency, and set
 * the toa threshold to the first point where the toa efficiency is 1. 
 * This corresponds to a point above the pedestal where the pedestal shouldn't 
 * fluctuate and trigger the toa.
 *
 * Things that might be improved upon:
 * the number of events nevents, which determines our toa efficiency.
 * The calib value which defines a small pulse. 
 *
 */
static void set_toa(Target* tgt, pflib::ROC& roc, int channel) {
  int link = (channel / 36);
  auto refvol_page = pflib::utility::string_format("REFERENCEVOLTAGE_%d", link);
  auto channel_page = pflib::utility::string_format("CH_%d", channel);
  int nevents = 30;
  double toa_eff{2};
  // in the calibration documentation, it is suggested to send a "small" charge injection.
  // Here I used 200 but there is maybe a better value.
  int calib = 200;
  auto test_param_handle = roc.testParameters()
      .add(refvol_page, "CALIB",  calib)
      .add(refvol_page, "INTCTEST", 1)
      .add(channel_page, "LOWRANGE", 1)
      .apply();


  pflib_log(info) << "finding the TOA threshold!";
  // This class doesn't write to csv. When we just would like to
  // use the data for setting params.
  pflib::DecodeAndBuffer buffer(nevents);

  tgt->setup_run(1 /* dummy - not stored */, Target::DaqFormat::SIMPLEROC, 1 /* dummy */);
  for (int toa_vref = 100; toa_vref < 250; toa_vref++) {
    auto test_handle = roc.testParameters()
      .add(refvol_page, "TOA_VREF", toa_vref)
      .apply();
    tgt->daq_run("CHARGE", buffer, nevents, pftool::state.daq_rate);
    std::vector<double> toa_data;
    for (const pflib::packing::SingleROCEventPacket &ep : buffer.get_buffer()) {
      auto toa = ep.channel(channel).toa();
      if (toa > 0) {
        toa_data.push_back(toa);
      }
    }
    toa_eff = static_cast<double>(toa_data.size())/nevents;
    if (toa_eff == 1) {
      roc.applyParameter(refvol_page, "TOA_VREF", toa_vref);
      pflib_log(info) << "the TOA threshold is set to " << toa_vref;
      return;
    }
    toa_vref += 1;
  }
  PFEXCEPTION_RAISE("NOTOA", "No TOA threshold was found for channel " + std::to_string(channel) + "!");
}

/**
 * TASKS.CHARGE_TIMESCAN
 *
 * Scan a internal calibration pulse in time by varying the charge_to_l1a
 * and top.phase_strobe parameters
 */
static void charge_timescan(Target* tgt) {
  int nevents = pftool::readline_int("How many events per time point? ", 1);
  bool isLED = pftool::readline_bool("Flash LED instead of the internal calibration pulse?", true);
  int channel = pftool::readline_int("Channel to pulse into? ", 61);
  int link = (channel / 36);
  auto channel_page = pflib::utility::string_format("CH_%d", channel);
  auto refvol_page = pflib::utility::string_format("REFERENCEVOLTAGE_%d", link);
  int start_bx = pftool::readline_int("Starting BX? ", -1);
  int n_bx = pftool::readline_int("Number of BX? ", 3);
  pflib::ROC roc{tgt->hcal().roc(pftool::state.iroc, pftool::state.type_version())};
  std::string fname;
  bool preCC = false;
  bool highrange = false;
  int calib = 0;

  auto test_param_builder = roc.testParameters();
  if(isLED){
    fname = pftool::readline_path("led-time-scan", ".csv");
    //Makes sure charge injections are turned off (in this ch at least)
    test_param_builder
      .add(refvol_page, "CALIB", 0)
      .add(refvol_page, "CALIB_2V5", 0)
      .add(refvol_page, "INTCTEST", 1)
      .add(refvol_page, "CHOICE_CINJ", 0)
      .add(channel_page, "HIGHRANGE", 0)
      .add(channel_page, "LOWRANGE", 0);
  } else {
    preCC = pftool::readline_bool("Use pre-CC charge injection? ", false);
    highrange = false;
    if (!preCC) highrange = pftool::readline_bool("Use highrange (Y) or lowrange (N)? ", false);
    calib = pftool::readline_int("Setting for calib pulse amplitude? ", highrange ? 64 : 1024);
    fname = pftool::readline_path("charge-time-scan", ".csv");
    test_param_builder
      .add(refvol_page, "CALIB", preCC ? 0 : calib)
      .add(refvol_page, "CALIB_2V5", preCC ? calib : 0)
      .add(refvol_page, "INTCTEST", 1)
      .add(refvol_page, "CHOICE_CINJ", (highrange && !preCC) ? 1 : 0)
      .add(channel_page, "HIGHRANGE", (highrange || preCC) ? 1 : 0)
      .add(channel_page, "LOWRANGE", preCC ? 0 : highrange ? 0 : 1);
  }
  auto test_param_handle = test_param_builder.apply();
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
      header["ledflash"] = isLED;
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
  tgt->setup_run(1 /* dummy - not stored */, Target::DaqFormat::SIMPLEROC, 1 /* dummy */);
  int central_charge_to_l1a;
  if(isLED){
    central_charge_to_l1a = tgt->fc().fc_get_setup_led();
  } else {
    central_charge_to_l1a = tgt->fc().fc_get_setup_calib();
  }
  for (charge_to_l1a = central_charge_to_l1a+start_bx;
       charge_to_l1a < central_charge_to_l1a+start_bx+n_bx; charge_to_l1a++) {
    if(isLED){
      tgt->fc().fc_setup_led(charge_to_l1a);
      pflib_log(info) << "led_to_l1a = " << tgt->fc().fc_get_setup_led();
    } else {
      tgt->fc().fc_setup_calib(charge_to_l1a);
      pflib_log(info) << "charge_to_l1a = " << tgt->fc().fc_get_setup_calib();
    }
    for (phase_strobe = 0; phase_strobe < n_phase_strobe; phase_strobe++) {
      auto phase_strobe_test_handle = roc.testParameters()
        .add("TOP", "PHASE_STROBE", phase_strobe)
        .apply();
      pflib_log(info) << "TOP.PHASE_STROBE = " << phase_strobe;
      usleep(10); // make sure parameters are applied
      time = 
        (charge_to_l1a - central_charge_to_l1a + offset) * clock_cycle
        - phase_strobe * clock_cycle/n_phase_strobe;
      if(isLED){
        tgt->daq_run("LED", writer, nevents, pftool::state.daq_rate);
      } else {
        tgt->daq_run("CHARGE", writer, nevents, pftool::state.daq_rate);
      }
    }
  }
  // reset charge_to_l1a to central value
  if(isLED){
    tgt->fc().fc_setup_led(central_charge_to_l1a);
  } else {
    tgt->fc().fc_setup_calib(central_charge_to_l1a);
  }
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

  int trim_inv =0;

  pflib::DecodeAndWriteToCSV writer{
    output_filepath,
    [&](std::ofstream& f) {
      boost::json::object header;
      header["scan_type"] = "CH_#.TRIM_INV sweep";
      header["trigger"] = "PEDESTAL";
      header["nevents_per_point"] = nevents;
      f << "# " << boost::json::serialize(header) << "\n"
        << "TRIM_INV";
      for (int ch{0}; ch < 72; ch++) {
        f << ',' << ch;
      }
      f << '\n';
    },
    [&](std::ofstream& f, const pflib::packing::SingleROCEventPacket &ep) {
      f << trim_inv;
      for (int ch{0}; ch < 72; ch++) {
        f << ',' << ep.channel(ch).adc();
      }
      f << '\n';
    }
  };

  tgt->setup_run(1 /* dummy */, Target::DaqFormat::SIMPLEROC, 1 /* dummy */);

  //take pedestal run on each parameter point
  for (trim_inv = 0; trim_inv < 64; trim_inv += 4) {
    pflib_log(info) << "Running TRIM_INV = " << trim_inv;
    auto trim_inv_test_builder = roc.testParameters();
    for (int ch{0}; ch < 72; ch++) {
      trim_inv_test_builder.add("CH_"+std::to_string(ch), "TRIM_INV", trim_inv);
    }
    auto trim_inv_test = trim_inv_test_builder.apply();
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

  int inv_vref = 0;

  pflib::DecodeAndWriteToCSV writer{
    output_filepath,
    [&](std::ofstream& f) {
      boost::json::object header;
      header["scan_type"] = "CH_#.INV_VREF sweep";
      header["trigger"] = "PEDESTAL";
      header["nevents_per_point"] = nevents;
      f << "# " << boost::json::serialize(header) << "\n"
        << "INV_VREF";
      for (int ch{0}; ch < 72; ch++) {
        f << ',' << ch;
      }
      f << '\n';
    },
    [&](std::ofstream& f, const pflib::packing::SingleROCEventPacket &ep) {
      f << inv_vref;
      for (int ch{0}; ch < 72; ch++) {
        f << ',' << ep.channel(ch).adc();
      }
      f << '\n';
    }
  };

  tgt->setup_run(1 /* dummy */, Target::DaqFormat::SIMPLEROC, 1 /* dummy */);

  //set global params HZ_noinv on each link (arbitrary channel on each)
    auto test_param = roc.testParameters()
      .add("CH_17", "HZ_NOINV", 1)
      .add("CH_53", "HZ_NOINV", 1)
      .apply();

  //increment inv_vref in increments of 20. 10 bit value but only scanning to 600
  for (inv_vref = 0; inv_vref <= 600; inv_vref += 20) {
    pflib_log(info) << "Running INV_VREF = " << inv_vref;
    //set inv_vref simultaneously for both links
    auto test_param = roc.testParameters()
      .add("REFERENCEVOLTAGE_0", "INV_VREF", inv_vref)
      .add("REFERENCEVOLTAGE_1", "INV_VREF", inv_vref)
      .apply();
      //store current scan state in header for writer access
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

  std::string output_filepath = pftool::readline_path("inv_vref_scan", ".csv");

  auto roc = tgt->hcal().roc(pftool::state.iroc, pftool::state.type_version());

  int noinv_vref = 0;

  pflib::DecodeAndWriteToCSV writer{
    output_filepath,
    [&](std::ofstream& f) {
      boost::json::object header;
      header["scan_type"] = "CH_#.NOINV_VREF sweep";
      header["trigger"] = "PEDESTAL";
      header["nevents_per_point"] = nevents;
      f << "# " << boost::json::serialize(header) << "\n"
        << "NOINV_VREF";
      for (int ch{0}; ch < 72; ch++) {
        f << ',' << ch;
      }
      f << '\n';
    },
    [&](std::ofstream& f, const pflib::packing::SingleROCEventPacket &ep) {
      f << noinv_vref;
      for (int ch{0}; ch < 72; ch++) {
        f << ',' << ep.channel(ch).adc();
      }
      f << '\n';
    }
  };

  tgt->setup_run(1 /* dummy */, Target::DaqFormat::SIMPLEROC, 1 /* dummy */);

  //set global params HZ_noinv on each link (arbitrary channel on each)
    auto test_param = roc.testParameters()
      .add("CH_17", "HZ_INV", 1)
      .add("CH_53", "HZ_INV", 1)
      .apply();

  //increment inv_vref in increments of 20. 10 bit value but only scanning to 600
  for (noinv_vref = 0; noinv_vref <= 600; noinv_vref += 20) {
    pflib_log(info) << "Running NOINV_VREF = " << noinv_vref;
    //set noinv_vref simultaneously for both links
    auto test_param = roc.testParameters()
      .add("REFERENCEVOLTAGE_0", "NOINV_VREF", noinv_vref)
      .add("REFERENCEVOLTAGE_1", "NOINV_VREF", noinv_vref)
      .apply();
      //store current scan state in header for writer access
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
  bool totscan = pftool::readline_bool("Scan TOT? This setting reduced the samples per BX to 1 ", false);
  int toa_threshold = 0;
  int tot_threshold = 0;
  if (totscan) toa_threshold = pftool::readline_int("Value for TOA threshold: ", 250);
  if (totscan) tot_threshold = pftool::readline_int("Value for TOT threshold: ", 500);
  int calib = pftool::readline_int("Setting for calib pulse amplitude? ", highrange ? 64 : 1024);
  std::vector<int> channels;
  if (pftool::readline_bool("Pulse into channel 61 (N) or all channels (Y)?", false)) {
    for (int ch{0}; ch<72; ch++)
      channels.push_back(ch);
  } else {
      channels.push_back(61);
  }
  int start_bx = pftool::readline_int("Starting BX? ", -1);
  int n_bx = pftool::readline_int("Number of BX? ", 3);
  std::string fname = pftool::readline_path("param-time-scan", ".csv");
  std::array<bool,2> link{false,false};
  for (int ch : channels) link[ch / 36] = true; //link0 if ch0-35, and 1 if 36-71
  
  auto roc{tgt->hcal().roc(pftool::state.iroc, pftool::state.type_version())};
  auto test_param_handle = roc.testParameters();
  auto add_rv =[&](int l) {
    auto refvol_page = pflib::utility::string_format("REFERENCEVOLTAGE_%d", l); 
    test_param_handle
      .add(refvol_page, "CALIB", preCC ? 0 : calib)
      .add(refvol_page, "CALIB_2V5", preCC ? calib : 0)
      .add(refvol_page, "INTCTEST", 1)
      .add(refvol_page, "CHOICE_CINJ", (highrange && !preCC) ? 1 : 0)
      .add(refvol_page, "TOA_VREF", toa_threshold)
      .add(refvol_page, "TOT_VREF", tot_threshold);
  };
  if (link[0]) add_rv(0);
  if (link[1]) add_rv(1);

  for (int ch : channels) {
    auto channel_page = pflib::utility::string_format("CH_%d", ch);
    test_param_handle.add(channel_page, "HIGHRANGE", (highrange || preCC) ? 1 : 0)
      .add(channel_page, "LOWRANGE", preCC ? 0 : highrange ? 0 : 1);
  }
  auto test_param = test_param_handle.apply();

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
      header["highrange"] = highrange;
      header["preCC"] = preCC;
      f << std::boolalpha
        << "# " << boost::json::serialize(header) << '\n'
        << "time,";
      for (const auto& [ page, parameter ] : param_names) {
        f << page << '.' << parameter << ',';
      }
      f << "channel," << pflib::packing::Sample::to_csv_header << '\n';
    },
    [&](std::ofstream& f, const pflib::packing::SingleROCEventPacket& ep) {
      for (int ch : channels) {
        f << time << ',';

        for (const auto& val : param_values[i_param_point]) {
          f << val << ',';
        }
        f << ch << ',';

        ep.channel(ch).to_csv(f);
        f << '\n';
      }
    } 
  };
  tgt->setup_run(1 /* dummy - not stored */, Target::DaqFormat::SIMPLEROC, 1 /* dummy */);
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
        for (int l = 0; l<2; l++) {
          if (link[l]) {
            test_param_builder.add(str_page + "_" + std::to_string(l), param, value);
          }
        }
      }
      pflib_log(info) << param << " = " << value;
    }
    auto test_param = test_param_builder.apply();
    // timescan
    auto central_charge_to_l1a = tgt->fc().fc_get_setup_calib();
    for (charge_to_l1a = central_charge_to_l1a+start_bx;
      charge_to_l1a < central_charge_to_l1a+start_bx+n_bx; charge_to_l1a++) {
      tgt->fc().fc_setup_calib(charge_to_l1a);
      pflib_log(info) << "charge_to_l1a = " << tgt->fc().fc_get_setup_calib();
      if (!totscan) 
      {
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
      else 
      {
        time = (charge_to_l1a - central_charge_to_l1a + offset) * clock_cycle;
        tgt->daq_run("CHARGE", writer, nevents, pftool::state.daq_rate);
      }
    }
    // reset charge_to_l1a to central value
    tgt->fc().fc_setup_calib(central_charge_to_l1a);
  }
}

/**
 * TASKS.SAMPLING_PHASE_SCAN
 * 
 * Scan over phase_ck, do pedestals.
 * Used to align clock phase. Inspired by the document "Handling
 * of different HGCAL commissiong run types: procedures, results,
 * expected" by Amendola et al.
 */
static void sampling_phase_scan(Target* tgt) {
  int nevents = pftool::readline_int("How many events per time point? ", 100);

  std::string fname = pftool::readline_path("sampling-phase-scan", ".csv");

  auto roc = tgt->hcal().roc(pftool::state.iroc, pftool::state.type_version());

  int phase_ck = 0;

  pflib::DecodeAndWriteToCSV writer{
    fname, //output file name
    [&](std::ofstream& f) {
      boost::json::object header;
      header["scan_type"] = "CH_#.PHASE_CK sweep";
      header["trigger"] = "PEDESTAL";
      header["nevents_per_point"] = nevents;
      f << "#" << boost::json::serialize(header) << "\n"
        << "PHASE_CK";
      for (int ch{0}; ch < 72; ch++) {
        f << "," << ch;
      }
      f << "\n";
    },
    [&](std::ofstream& f, const pflib::packing::SingleROCEventPacket& ep) {
      f << phase_ck;
      for (int ch{0}; ch < 72; ch++) {
        f << "," << ep.channel(ch).adc();
      }
      f << "\n";
    }
  };

  tgt->setup_run(1 /* dummy - not stored */, Target::DaqFormat::SIMPLEROC, 1 /* dummy */);
  
  // Loop over phases and do pedestals
  for (phase_ck = 0; phase_ck < 16; phase_ck++) {
    pflib_log(info) << "Scanning phase_ck = " << phase_ck;
    auto phase_test_handle = roc.testParameters()
      .add("TOP", "PHASE_CK", phase_ck)
      .apply();
    tgt->daq_run("PEDESTAL", writer, nevents, pftool::state.daq_rate);
  }
}

/**
 * TASKS.VT50_SCAN
 *
 * Scans the calib range for the v_t50 of the tot.
 * The v_t50 is the point where the tot triggers 50 percent of the time.
 * We check the tot efficiency at every calib value and use a recursive method
 * to hone in on the v_t50.
 *
 * The tot efficiency is calculated depending on the number of events given. The higher
 * the number of events, the more accurate tot efficiency.
 *
 * To hone in on the v_t50, I constructed two methods: binary and bisectional search.
 * Both methods work, altough the binary search sometimes scans the same parameter point
 * twice, giving rise to tot efficiencies larger than 1. 
 * This can be adjusted for in analysis.
 *
 */
static void vt50_scan(Target* tgt) {
  int nevents = pftool::readline_int("How many events per time point? Remember that the tot efficiency's granularity depends on this number. ", 20);
  bool preCC = pftool::readline_bool("Use pre-CC charge injection? ", false);
  bool highrange = false;
  if (!preCC) highrange = pftool::readline_bool("Use highrange (Y) or lowrange (N)? ", false);
  bool search = pftool::readline_bool("Use binary (Y) or bisectional search (N)? ", false);
  int channel = pftool::readline_int("Channel to pulse into? ", 61);
  int toa_threshold = pftool::readline_int("Value for TOA threshold: ", 250);
  int vref_lower = pftool::readline_int("Smallest tot threshold value (min = 0): ", 300);
  int vref_upper = pftool::readline_int("Largest tot threshold value (max = 4095): ", 600);
  int nsteps = pftool::readline_int("Number of steps between tot values: ", 10);
  std::string fname = pftool::readline_path("vt50-scan", ".csv");
  auto roc{tgt->hcal().roc(pftool::state.iroc, pftool::state.type_version())};
  auto channel_page = pflib::utility::string_format("CH_%d", channel);
  int link = (channel / 36);
  auto refvol_page = pflib::utility::string_format("REFERENCEVOLTAGE_%d", link);
  auto test_param_handle = roc.testParameters()
    .add(refvol_page, "INTCTEST", 1)
    .add(refvol_page, "TOA_VREF", toa_threshold)
    .add(refvol_page, "CHOICE_CINJ", (highrange && !preCC) ? 1 : 0)
    .add(channel_page, "HIGHRANGE", (highrange || preCC) ? 1 : 0)
    .add(channel_page, "LOWRANGE", preCC ? 0 : highrange ? 0 : 1)
    .apply();

  std::string vref_page = refvol_page;
  std::string calib_page = refvol_page;
  std::string vref_name = "TOT_VREF";
  std::string calib_name{preCC ? "CALIB_2V5" : "CALIB"};
  int calib_value{100000};
  double tot_eff{0};

  // Vectors for storing tot_eff and calib for the current param_point
  std::vector<double> tot_eff_list;
  std::vector<int> calib_list = {0, 4095}; //min and max
  // tolerance for checking distance between tot_eff and 0.5
  double tol{0.1};
  int count{2};
  int max_its = 25;
  int vref_value{0};

  std::vector<pflib::packing::SingleROCEventPacket> buffer;
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
      f << vref_page << '.' << vref_name << ','
        << calib_page << '.' << calib_name << ',';
      f << pflib::packing::Sample::to_csv_header
        << '\n';
    },
    [&](std::ofstream& f, const pflib::packing::SingleROCEventPacket& ep) {
      f << time << ',';
      f << vref_value << ',';
      f << calib_value << ',';
      ep.channel(channel).to_csv(f);
      f << '\n';
      buffer.push_back(ep);
    }
  };

  tgt->setup_run(1 /* dummy - not stored */, Target::DaqFormat::SIMPLEROC, 1 /* dummy */);
  for (vref_value = vref_lower; vref_value <= vref_upper; vref_value += nsteps) {
    // reset for every iteration
    tot_eff_list.clear();
    calib_list = {0, 4095};
    calib_value = 100000;
    tot_eff = 0;
    auto vref_test_param = roc.testParameters()
      .add(vref_page, vref_name, vref_value)
      .apply();
    pflib_log(info) << vref_name << " = " << vref_value;
    while (true) {
      if (search) {
        // BINARY SEARCH
        if (!tot_eff_list.empty()) {
          if (tot_eff_list.back() > 0.5) {
            calib_value = std::abs(calib_list.back() - calib_list[calib_list.size() - 2])/2
                        + calib_list[calib_list.size() - count];
            count++;
          } else {
            calib_value = std::abs((calib_list[calib_list.size() - 2] - calib_list.back()))/2
                        + calib_list.back();
            count = 2;
          }
        } else {
          calib_value = calib_list.front();
        }
      } else {
        // BISECTIONAL SEARCH
        if (!tot_eff_list.empty()) {
          if (tot_eff_list.back() > 0.5) {
            calib_list.back() = calib_value;
          } else {
            calib_list.front() = calib_value;
          }
          calib_value = (calib_list.back() - calib_list.front())/2 + calib_list.front();
        } else {
          calib_value = (calib_list.back() - calib_list.front())/2;
        }
      }
      auto calib_test_param = roc.testParameters()
        .add(calib_page, calib_name, calib_value)
        .apply();
      usleep(10); // make sure parameters are applied

      buffer.clear();

      // daq run
      tgt->daq_run("CHARGE", writer, nevents, pftool::state.daq_rate);

      std::vector<double> tot_list;
      for (pflib::packing::SingleROCEventPacket ep : buffer) {
        auto tot = ep.channel(channel).tot();
        if (tot > 0) {
          tot_list.push_back(tot);
        }
      }

      // Calculate tot_eff
      tot_eff = static_cast<double>(tot_list.size())/nevents;

      if (search) {
        // BINARY SEARCH
        if (std::abs(calib_value-calib_list.back()) < tol) {
          pflib_log(info) << "Final calib is : " << calib_value
                          << " with tot_eff = " << tot_eff;
          break;
        }
      } else {
        // BISECTIONAL SEARCH
        if (std::abs(tot_eff - 0.5) < tol) {
          pflib_log(info) << "Final calib is : " << calib_value
                          << " with tot_eff = " << tot_eff;
          break;
        }
        if (calib_value == 0 || calib_value == 4094) {
          pflib_log(info) << "No v_t50 was found!";
          break;
        }
      }

      tot_eff_list.push_back(tot_eff);
      if (search) calib_list.push_back(calib_value);

      // Sometimes we can't hone in close enough to 0.5 because calib are whole ints
      if (tot_eff_list.size() > max_its) {
        pflib_log(info) << "Ended after " << max_its << " iterations!" << '\n' 
                        << "Final calib is : " << calib_value
                        << " with tot_eff = " << tot_eff;
        break;
      }
    }
  }
}


/**
 * TASKS.TOA_VREF_SCAN
 *
 * Scan TOA_VREF for both halves of the chip
 * Check ADC Pedestals for each point
 * Used to align TOA trigger to fire just above pedestals
 * Should be done after pedestal alignment
 */
static void toa_vref_scan(Target* tgt) {
  int nevents = pftool::readline_int("Number of events per point: ", 100);

  std::string output_filepath = pftool::readline_path("toa_vref_scan", ".csv");

  auto roc = tgt->hcal().roc(pftool::state.iroc, pftool::state.type_version());

  int toa_vref = 0;

  pflib::DecodeAndWriteToCSV writer{
    output_filepath,
    [&](std::ofstream& f) {
      boost::json::object header;
      header["scan_type"] = "CH_#.TOA_VREF sweep";
      header["trigger"] = "PEDESTAL";
      header["nevents_per_point"] = nevents;
      f << "# " << boost::json::serialize(header) << "\n"
        << "TOA_VREF";
      for (int ch{0}; ch < 72; ch++) {
        f << "," << ch;
      }
      f << "\n";
    },
    [&](std::ofstream& f, const pflib::packing::SingleROCEventPacket &ep) {
      f << toa_vref;
      for (int ch{0}; ch < 72; ch++) {
        f << "," << ep.channel(ch).toa();
      }
      f << "\n";
    }
  };

  tgt->setup_run(1 /* dummy */, Target::DaqFormat::SIMPLEROC, 1 /* dummy */);

  // Take a pedestal run on each parameter point
  // Toa_vref has a 10 b range, but we're not going to need all of that
  // since the pedestal variations are likely lower than ~300 adc.
  for (toa_vref = 0; toa_vref < 256; toa_vref++) {
    pflib_log(info) << "Running TOA_VREF = " << toa_vref;
    auto toa_vref_test = roc.testParameters()
      .add("REFERENCEVOLTAGE_0", "TOA_VREF", toa_vref)
      .add("REFERENCEVOLTAGE_1", "TOA_VREF", toa_vref)
      .apply();
    tgt->daq_run("PEDESTAL", writer, nevents, pftool::state.daq_rate);
  }
}

/**
 * TASKS.TRIM_TOA_SCAN
 * 
 * Scan TRIM_TOA  and CALIB for each channel on chip
 * Used to align TOA measurement after TOA_VREF scan
 */

static void trim_toa_scan(Target* tgt) {
  int nevents = pftool::readline_int("Number of events per point: ", 100);

  std::string output_filepath = pftool::readline_path("trim_toa_scan", ".csv");

  auto roc = tgt->hcal().roc(pftool::state.iroc, pftool::state.type_version());

  int trim_toa = 0;
  int calib = 0;

  pflib::DecodeAndWriteToCSV writer{
    output_filepath,
    [&](std::ofstream& f) {
      boost::json::object header;
      header["scan_type"] = "CH_#.TRIM_TOA sweep";
      header["trigger"] = "CHARGE";
      header["nevents_per_point"] = nevents;
      f << "# " << boost::json::serialize(header) << "\n"
        << "TRIM_TOA" << "," << "CALIB";
      for (int ch{0}; ch < 72; ch++) {
        f << "," << ch;
      }
      f << "\n";
    },
    [&](std::ofstream& f, const pflib::packing::SingleROCEventPacket &ep) {
      f << trim_toa << "," << calib;
      // Write the TOA values for each channel
      for (int ch{0}; ch < 72; ch++) {
        f << "," << ep.channel(ch).toa();
      }
      f << "\n";
    }
  };

  tgt->setup_run(1 /* dummy */, Target::DaqFormat::SIMPLEROC, 1 /* dummy */);

  // Take a charge injection run at each trim_toa. Do post-CC lowrange
  // and test all channels at a same time. Trim_toa has a range of 6 b.
  // And vary calib, from 0 to 800. Want to see how toa_efficiency
  // changes with trim_toa and calib. More details in the ana script.
  auto setup_builder = roc.testParameters()
    .add("REFERENCEVOLTAGE_0", "CALIB", calib)
    .add("REFERENCEVOLTAGE_1", "CALIB", calib)
    .add("REFERENCEVOLTAGE_0", "INTCTEST", 1)
    .add("REFERENCEVOLTAGE_1", "INTCTEST", 1);
  for (int ch{0}; ch < 72; ch++) {
    setup_builder.add("CH_"+std::to_string(ch), "LOWRANGE", 1);
  }
  auto setup_test = setup_builder.apply();

  for (trim_toa = 0; trim_toa < 32; trim_toa += 4) {
    pflib_log(info) << "Running TRIM_TOA = " << trim_toa;
    auto trim_toa_test_builder = roc.testParameters();
    for (int ch{0}; ch < 72; ch++) {
      trim_toa_test_builder.add("CH_"+std::to_string(ch), "TRIM_TOA", trim_toa);
    }
    // set TRIM_TOA for each channel
    auto trim_toa_test = trim_toa_test_builder.apply();
    for (calib = 0; calib < 800; calib += 4) {
      pflib_log(info) << "Running CALIB = " << calib;
      // Set the CALIB parameters for both halves
      auto calib_test = roc.testParameters()
        .add("REFERENCEVOLTAGE_0", "CALIB", calib)
        .add("REFERENCEVOLTAGE_1", "CALIB", calib)
        .apply();
      tgt->daq_run("CHARGE", writer, nevents, pftool::state.daq_rate);
    }
  }
}

/**
 * TASKS.TOA_SCAN
 * 
 * Just a simple scan that measures toa FOR a range of CALIB. Use to look
 * at results of the TRIM_TOA_SCAN and get_trim_toa.py files
 */

 static void toa_scan(Target* tgt) {
  int nevents = pftool::readline_int("Number of events per point: ", 100);

  std::string output_filepath = pftool::readline_path("toa_scan", ".csv");

  auto roc = tgt->hcal().roc(pftool::state.iroc, pftool::state.type_version());

  int calib = 0;

  pflib::DecodeAndWriteToCSV writer{
    output_filepath,
    [&](std::ofstream& f) {
      boost::json::object header;
      header["scan_type"] = "CH_#.TOA sweep";
      header["trigger"] = "CHARGE";
      header["nevents_per_point"] = nevents;
      f << "# " << boost::json::serialize(header) << "\n"
        << "CALIB";
      for (int ch{0}; ch < 72; ch++) {
        f << "," << ch;
      }
      f << "\n";
    },
    [&](std::ofstream& f, const pflib::packing::SingleROCEventPacket &ep) {
      f << calib;
      // Write the TOA values for each channel
      for (int ch{0}; ch < 72; ch++) {
        f << "," << ep.channel(ch).toa();
      }
      f << "\n";
    }
  };

  tgt->setup_run(1 /* dummy */, Target::DaqFormat::SIMPLEROC, 1 /* dummy */);

  // Take a charge injection run.
  auto setup_builder = roc.testParameters()
    .add("REFERENCEVOLTAGE_0", "CALIB", calib)
    .add("REFERENCEVOLTAGE_1", "CALIB", calib)
    .add("REFERENCEVOLTAGE_0", "INTCTEST", 1)
    .add("REFERENCEVOLTAGE_1", "INTCTEST", 1);
  for (int ch{0}; ch < 72; ch++) {
    setup_builder.add("CH_"+std::to_string(ch), "LOWRANGE", 1);
  }
  auto setup_test = setup_builder.apply();

  for (calib = 0; calib < 800; calib += 4) {
    pflib_log(info) << "Running CALIB = " << calib;
    // Set the CALIB parameters for both halves
    auto calib_test = roc.testParameters()
      .add("REFERENCEVOLTAGE_0", "CALIB", calib)
      .add("REFERENCEVOLTAGE_1", "CALIB", calib)
      .apply();
    tgt->daq_run("CHARGE", writer, nevents, pftool::state.daq_rate);
  }
}

#include "tasks/level_pedestals.h"

namespace {
auto menu_tasks =
    pftool::menu("TASKS", "tasks for studying the chip and tuning its parameters")
        ->line("CHARGE_TIMESCAN", "scan charge/calib pulse over time", charge_timescan)
        ->line("GEN_SCAN", "scan over file of input parameter points", gen_scan)
        ->line("PARAMETER_TIMESCAN", "scan charge/calib pulse over time for varying parameters", parameter_timescan)
        ->line("TRIM_INV_SCAN", "scan trim_inv parameter", trim_inv_scan)
        ->line("INV_VREF_SCAN", "scan over INV_VREF parameter", inv_vref_scan)
        ->line("NOINV_VREF_SCAN", "scan over NOINV_VREF parameter", noinv_vref_scan)
        ->line("SAMPLING_PHASE_SCAN", "scan phase_ck, pedestal for clock phase alignment", sampling_phase_scan)
        ->line("VT50_SCAN", "Hones in on the vt50 with a binary or bisectional scan", vt50_scan)
        ->line("LEVEL_PEDESTALS", "tune trim_inv and dacb to level pedestals with their link median", level_pedestals)
        ->line("TOA_VREF_SCAN", "scan over VREF parameters for TOA calibration", toa_vref_scan)
        ->line("TRIM_TOA_SCAN", "scan over TRIM parameters for TOA calibration", trim_toa_scan)
        ->line("TOA_SCAN", "just does that bro", toa_scan)
;
}