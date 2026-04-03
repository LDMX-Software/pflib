#include <nlohmann/json.hpp>

#include "../daq_run.h"
#include "charge_timescan.h"
#include "pflib/utility/string_format.h"

ENABLE_LOGGING();

void channel_wise_calib_scan(Target* tgt) {
  int nevents = pftool::readline_int("How many events per time point? ", 10);
  int stepsize = pftool::readline_int("How many steps between calibs? ", 50);
  int start_bx = pftool::readline_int("Starting BX? ", 0);
  int n_bx = pftool::readline_int("Number of BX? ", 2);
  int min_ch = pftool::readline_int("Channel to start scan on? ", 0);
  int max_ch = pftool::readline_int("Channel to end scan on? ", 71);
  pflib::ROC roc{tgt->roc(pftool::state.iroc)};
  std::string fname;

  auto test_param_builder = roc.testParameters();

  fname = pftool::readline_path("channel-wise-calib-scan", ".csv");
  for (int i_link = 0; i_link < 2; i_link++) {
    auto refvol_page =
        pflib::utility::string_format("REFERENCEVOLTAGE_%d", i_link);
    test_param_builder.add(refvol_page, "INTCTEST", 1)
        .add(refvol_page, "CHOICE_CINJ", 1);
  }
  auto test_param_handle = test_param_builder.apply();

  int central_charge_to_l1a;
  int charge_to_l1a{0};
  int phase_strobe{0};
  double time{0};
  double clock_cycle{25.0};
  int n_phase_strobe{16};
  int offset{1};
  int n_links{tgt->nrocs() * 2};
  int calib{0};
  int ch{0};
  int link{1};
  DecodeAndWriteToCSV writer{
      fname,
      [&](std::ofstream& f) {
        nlohmann::json header;
        f << "time,calib,channel," << pflib::packing::Sample::to_csv_header
          << '\n';
      },
      [&](std::ofstream& f,
          const pflib::packing::MultiSampleECONDEventPacket& ep) {
        // TODO 348
        if (ch < 36) {
          link = 0;
        } else if (ch >= 36) {
          link = 1;
        }
        f << time << ',' << calib << ',' << ch << ',';
        ep.samples[ep.i_soi].channel(link, ch).to_csv(f);
        f << '\n';
      },
      n_links};

  tgt->setup_run(1, Target::DaqFormat::ECOND_SW_HEADERS, 1);

  central_charge_to_l1a = tgt->fc().fc_get_setup_calib();

  for (ch = min_ch; ch < max_ch + 1; ch++) {
    pflib_log(info) << "Scanning channel " << ch;
    auto channel_page = pflib::utility::string_format("CH_%d", ch);
    for (calib = 0; calib < 550; calib += stepsize) {
      pflib_log(info) << "CALIB = " << calib;
      auto calib_handle_builder = roc.testParameters();
      if (ch < 36) {
        calib_handle_builder.add("REFERENCEVOLTAGE_0", "CALIB", calib)
            .add(channel_page, "HIGHRANGE", 1);
      } else {
        calib_handle_builder.add("REFERENCEVOLTAGE_1", "CALIB", calib)
            .add(channel_page, "HIGHRANGE", 1);
      }
      auto calib_handle = calib_handle_builder.apply();
      for (charge_to_l1a = central_charge_to_l1a + start_bx;
           charge_to_l1a < central_charge_to_l1a + start_bx + n_bx;
           charge_to_l1a++) {
        tgt->fc().fc_setup_calib(charge_to_l1a);
        pflib_log(info) << "charge_to_l1a = " << tgt->fc().fc_get_setup_calib();
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
      }
      // reset charge_to_l1a to central value
      tgt->fc().fc_setup_calib(central_charge_to_l1a);
    }
  }
}
