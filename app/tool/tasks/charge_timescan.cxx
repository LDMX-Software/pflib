#include "charge_timescan.h"

#include "pflib/utility/string_format.h"
#include "pflib/utility/json.h"
#include "pflib/DecodeAndWrite.h"

ENABLE_LOGGING();

void charge_timescan(Target* tgt) {
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
