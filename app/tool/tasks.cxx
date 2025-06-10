/**
 * @file tasks.cxx
 *
 * Definition of TASKS menu commands
 */
#include "pftool.h"

ENABLE_LOGGING();

#include "pflib/utility/string_format.h"
#include "pflib/utility/json.h"
#include "pflib/DecodeAndWrite.h"

static void charge_timescan(Target* tgt) {
  int nevents = pftool::readline_int("How many events per time point? ", 1);
  bool highrange = pftool::readline_bool("Use highrange (Y) or lowrange (N)? ", false);
  int calib = pftool::readline_int("Setting for calib pulse amplitude? ", highrange ? 64 : 1024);
  int channel = pftool::readline_int("Channel to pulse into? ", 61);
  int start_bx = pftool::readline_int("Starting BX? ", -1);
  int n_bx = pftool::readline_int("Number of BX? ", 3);
  std::string fname = pftool::readline_path("charge-time-scan", ".csv");
  pflib::ROC roc{tgt->hcal().roc(pftool::state.iroc, pftool::state.type_version)};
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

namespace {
auto menu_tasks =
    pftool::menu("TASKS", "tasks for studying the chip and tuning its parameters")
        ->line("CHARGE_TIMESCAN", "scan charge/calib pulse over time", charge_timescan);
}
