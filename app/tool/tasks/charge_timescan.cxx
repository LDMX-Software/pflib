#include "charge_timescan.h"

#include <nlohmann/json.hpp>

#include "../daq_run.h"
#include "pflib/utility/string_format.h"

ENABLE_LOGGING();

template <class EventPacket>
static void charge_timescan_writer(Target* tgt, pflib::ROC& roc, size_t nevents, bool isLED, int channel, int link, int i_ch, std::string fname, int calib, bool highrange, int start_bx, int n_bx){
  
  int central_charge_to_l1a;
  int charge_to_l1a{0};
  int phase_strobe{0};
  double time{0};
  double clock_cycle{25.0};
  int n_phase_strobe{16};
  int offset{1};

  DecodeAndWriteToCSV<EventPacket> writer{
      fname,
      [&](std::ofstream& f) {
        nlohmann::json header;
        header["channel"] = channel;
        header["calib"] = calib;
        header["highrange"] = highrange;
        header["ledflash"] = isLED;
        f << std::boolalpha << "# " << header << '\n'
          << "time," << pflib::packing::Sample::to_csv_header << '\n';
      },
      [&](std::ofstream& f, const EventPacket& ep) {
        f << time << ',';
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
      }};

  tgt->setup_run(1 /* dummy - not stored */, pftool::state.daq_format_mode,
                 1 /* dummy */);

  if (isLED) {
    central_charge_to_l1a = tgt->fc().fc_get_setup_led();
  } else {
    central_charge_to_l1a = tgt->fc().fc_get_setup_calib();
  }
  for (charge_to_l1a = central_charge_to_l1a + start_bx;
       charge_to_l1a < central_charge_to_l1a + start_bx + n_bx;
       charge_to_l1a++) {
    if (isLED) {
      tgt->fc().fc_setup_led(charge_to_l1a);
      pflib_log(info) << "led_to_l1a = " << tgt->fc().fc_get_setup_led();
    } else {
      tgt->fc().fc_setup_calib(charge_to_l1a);
      pflib_log(info) << "charge_to_l1a = " << tgt->fc().fc_get_setup_calib();
    }
    for (phase_strobe = 0; phase_strobe < n_phase_strobe; phase_strobe++) {
      auto phase_strobe_test_handle =
          roc.testParameters().add("TOP", "PHASE_STROBE", phase_strobe).apply();
      pflib_log(info) << "TOP.PHASE_STROBE = " << phase_strobe;
      usleep(10);  // make sure parameters are applied
      time = (charge_to_l1a - central_charge_to_l1a + offset) * clock_cycle -
             phase_strobe * clock_cycle / n_phase_strobe;
      if (isLED) {
        daq_run(tgt, "LED", writer, nevents, pftool::state.daq_rate);
      } else {
        daq_run(tgt, "CHARGE", writer, nevents, pftool::state.daq_rate);
      }
    }
  }
  // reset charge_to_l1a to central value
  if (isLED) {
    tgt->fc().fc_setup_led(central_charge_to_l1a);
  } else {
    tgt->fc().fc_setup_calib(central_charge_to_l1a);
  }
}

void charge_timescan(Target* tgt) {
  int nevents = pftool::readline_int("How many events per time point? ", 1);
  bool isLED = pftool::readline_bool(
      "Flash LED instead of the internal calibration pulse?", true);
  int channel = pftool::readline_int("Channel to pulse into? ", 61);
  int link = (channel / 36);
  int i_ch = channel % 36;  // 0–35
  auto channel_page = pflib::utility::string_format("CH_%d", channel);
  auto refvol_page = pflib::utility::string_format("REFERENCEVOLTAGE_%d", link);
  int start_bx = pftool::readline_int("Starting BX? ", -1);
  int n_bx = pftool::readline_int("Number of BX? ", 3);
  pflib::ROC roc{tgt->roc(pftool::state.iroc)};
  std::string fname;
  bool preCC = false;
  bool highrange = false;
  int calib = 0;

  auto test_param_builder = roc.testParameters();
  if (isLED) {
    fname = pftool::readline_path("led-time-scan", ".csv");
    // Makes sure charge injections are turned off (in this ch at least)
    test_param_builder.add(refvol_page, "CALIB", 0)
        .add(refvol_page, "CALIB_2V5", 0)
        .add(refvol_page, "INTCTEST", 1)
        .add(refvol_page, "CHOICE_CINJ", 0)
        .add(channel_page, "HIGHRANGE", 0)
        .add(channel_page, "LOWRANGE", 0);
  } else {
    preCC = pftool::readline_bool("Use pre-CC charge injection? ", false);
    highrange = false;
    if (!preCC)
      highrange =
          pftool::readline_bool("Use highrange (Y) or lowrange (N)? ", false);
    calib = pftool::readline_int("Setting for calib pulse amplitude? ",
                                 highrange ? 64 : 1024);
    fname = pftool::readline_path("charge-time-scan", ".csv");
    test_param_builder.add(refvol_page, "CALIB", preCC ? 0 : calib)
        .add(refvol_page, "CALIB_2V5", preCC ? calib : 0)
        .add(refvol_page, "INTCTEST", 1)
        .add(refvol_page, "CHOICE_CINJ", (highrange && !preCC) ? 1 : 0)
        .add(channel_page, "HIGHRANGE", (highrange || preCC) ? 1 : 0)
        .add(channel_page, "LOWRANGE",
             preCC       ? 0
             : highrange ? 0
                         : 1);
  }
  auto test_param_handle = test_param_builder.apply();

  if (pftool::state.daq_format_mode == Target::DaqFormat::SIMPLEROC) {
    charge_timescan_writer<pflib::packing::SingleROCEventPacket>(
        tgt, roc, nevents, isLED, channel, link, i_ch, fname, calib, highrange,
        start_bx, n_bx);
  } else if (pftool::state.daq_format_mode ==
             Target::DaqFormat::ECOND_SW_HEADERS) {
    charge_timescan_writer<pflib::packing::MultiSampleECONDEventPacket>(
        tgt, roc, nevents, isLED, channel, link, i_ch, fname, calib, highrange,
        start_bx, n_bx);
  }

  // switch (pftool::state.daq_format_mode) {
  //   case Target::DaqFormat::ECOND_SW_HEADERS: {
  //     DecodeAndWriteToCSV<pflib::packing::MultiSampleECONDEventPacket>
  //     writer{
  //         fname,
  //         [&](std::ofstream& f) {
  //           nlohmann::json header;
  //           header["channel"] = channel;
  //           header["calib"] = calib;
  //           header["highrange"] = highrange;
  //           header["ledflash"] = isLED;
  //           f << std::boolalpha << "# " << header << '\n'
  //             << "time," << pflib::packing::Sample::to_csv_header << '\n';
  //         },
  //         [&](std::ofstream& f,
  //             const pflib::packing::MultiSampleECONDEventPacket& ep) {
  //           f << time << ',';
  //           ep.samples[ep.i_soi].channel(link, i_ch).to_csv(f);
  //           f << '\n';
  //         }};
  //     break;
  //   }
  //   case Target::DaqFormat::SIMPLEROC: {
  //     DecodeAndWriteToCSV<pflib::packing::SingleROCEventPacket> writer{
  //         fname,
  //         [&](std::ofstream& f) {
  //           nlohmann::json header;
  //           header["channel"] = channel;
  //           header["calib"] = calib;
  //           header["highrange"] = highrange;
  //           header["ledflash"] = isLED;
  //           f << std::boolalpha << "# " << header << '\n'
  //             << "time," << pflib::packing::Sample::to_csv_header << '\n';
  //         },
  //         [&](std::ofstream& f,
  //             const pflib::packing::SingleROCEventPacket& ep) {
  //           f << time << ',';
  //           ep.channel(channel).to_csv(f);
  //           f << '\n';
  //         }};
  //     break;
  //   }
  //   default:
  //     PFEXCEPTION_RAISE("BadConf", "Unable to do all_channels_to_csv for the
  //     currently configured format.");
  // }

  // DecodeAndWriteToCSV writer{
  //     fname,
  //     [&](std::ofstream& f) {
  //       nlohmann::json header;
  //       header["channel"] = channel;
  //       header["calib"] = calib;
  //       header["highrange"] = highrange;
  //       header["ledflash"] = isLED;
  //       f << std::boolalpha << "# " << header << '\n'
  //         << "time," << pflib::packing::Sample::to_csv_header << '\n';
  //     },
  //     [&](std::ofstream& f, const pflib::packing::SingleROCEventPacket& ep) {
  //       f << time << ',';
  //       ep.channel(channel).to_csv(f);
  //       f << '\n';
  //     }
  //   };

  // tgt->setup_run(1 /* dummy - not stored */, pftool::state.daq_format_mode,
  //                1 /* dummy */);
  // int central_charge_to_l1a;
  // if (isLED) {
  //   central_charge_to_l1a = tgt->fc().fc_get_setup_led();
  // } else {
  //   central_charge_to_l1a = tgt->fc().fc_get_setup_calib();
  // }
  // for (charge_to_l1a = central_charge_to_l1a + start_bx;
  //      charge_to_l1a < central_charge_to_l1a + start_bx + n_bx;
  //      charge_to_l1a++) {
  //   if (isLED) {
  //     tgt->fc().fc_setup_led(charge_to_l1a);
  //     pflib_log(info) << "led_to_l1a = " << tgt->fc().fc_get_setup_led();
  //   } else {
  //     tgt->fc().fc_setup_calib(charge_to_l1a);
  //     pflib_log(info) << "charge_to_l1a = " <<
  //     tgt->fc().fc_get_setup_calib();
  //   }
  //   for (phase_strobe = 0; phase_strobe < n_phase_strobe; phase_strobe++) {
  //     auto phase_strobe_test_handle =
  //         roc.testParameters().add("TOP", "PHASE_STROBE",
  //         phase_strobe).apply();
  //     pflib_log(info) << "TOP.PHASE_STROBE = " << phase_strobe;
  //     usleep(10);  // make sure parameters are applied
  //     time = (charge_to_l1a - central_charge_to_l1a + offset) * clock_cycle -
  //            phase_strobe * clock_cycle / n_phase_strobe;
  //     if (isLED) {
  //       daq_run(tgt, "LED", writer, nevents, pftool::state.daq_rate);
  //     } else {
  //       daq_run(tgt, "CHARGE", writer, nevents, pftool::state.daq_rate);
  //     }
  //   }
  // }
  // // reset charge_to_l1a to central value
  // if (isLED) {
  //   tgt->fc().fc_setup_led(central_charge_to_l1a);
  // } else {
  //   tgt->fc().fc_setup_calib(central_charge_to_l1a);
  // }
}
