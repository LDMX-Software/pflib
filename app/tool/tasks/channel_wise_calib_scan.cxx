#include <nlohmann/json.hpp>
#include <tuple>

#include "../daq_run.h"
#include "charge_timescan.h"
#include "pflib/utility/string_format.h"

ENABLE_LOGGING();

void channel_wise_calib_scan(Target* tgt) {
  int nevents = pftool::readline_int("How many events per time point? ", 10);
  int stepsize = pftool::readline_int("How many steps between calibs? ", 10);
  int start_bx = pftool::readline_int("Starting BX? ", 0);
  int n_bx = pftool::readline_int("Number of BX? ", 1);
  int min_ch = pftool::readline_int("Channel to start scan on? ", 0);
  int max_ch = pftool::readline_int("Channel to end scan on? ", 71);
  int min_calib = pftool::readline_int("Minimum calib value = ", 0);
  int max_calib = pftool::readline_int("Maximum calib value = ", 600);
  if ((min_calib < 0) || (min_calib > 4095) || (max_calib < 0) ||
      (max_calib > 4095)) {
    PFEXCEPTION_RAISE(
        "InvalidArg",
        "Min and Max calib values have to be within the range: 0 <= calib <= "
        "4095");
  }
  std::map<std::string, std::map<std::string, uint64_t>> setup_parameters;
  auto mapping = tgt->getRocErxMapping();
  for (int i_link = 0; i_link < 2; i_link++) {
    auto refvol_page =
        pflib::utility::string_format("REFERENCEVOLTAGE_%d", i_link);
    setup_parameters[refvol_page]["INTCTEST"] = 1;
    setup_parameters[refvol_page]["CHOICE_CINJ"] = 1;
  }
  for (int ch = 0; ch < 72; ch++) {
    auto ch_page = pflib::utility::string_format("CH_%d", ch);
    setup_parameters[ch_page]["LOWRANGE"] = 0;
    setup_parameters[ch_page]["HIGHRANGE"] = 0;
  }
  auto test_setup_params = tgt->tempApplyAllROCs(setup_parameters);

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

  std::map<int, std::string> fnames;
  std::map<int, std::ofstream> outputs;
  for (int i_roc : tgt->roc_ids()) {
    fnames[i_roc] = pftool::readline_path(
        "channel-wise-calib-scan-roc-" + std::to_string(i_roc), ".csv");
    outputs[i_roc].open(fnames[i_roc]);
    outputs[i_roc] << "time,calib,channel,"
                   << pflib::packing::Sample::to_csv_header << '\n';
  }

  DecodeAndWriteToCSV writer{
      "channel-wise-calib-scan", [&](std::ofstream&) {},
      [&](std::ofstream&,
          const pflib::packing::MultiSampleECONDEventPacket& ep) {
        // TODO 348
        for (int i_roc : tgt->roc_ids()) {
          auto [i_erx, i_ch] = mapping.toErxChannel(i_roc, ch);
          auto& output = outputs[i_roc];
          output << time << ',' << calib << ',' << ch << ',';
          ep.soi().channel(i_erx, i_ch).to_csv(output);
          output << '\n';
        }
      },
      n_links};

  tgt->setup_run(1, Target::DaqFormat::ECOND_SW_HEADERS, 1);

  central_charge_to_l1a = tgt->fc().fc_get_setup_calib();
  for (ch = min_ch; ch < max_ch + 1; ch++) {
    pflib_log(info) << "Scanning channel " << ch;
    auto channel_page = pflib::utility::string_format("CH_%d", ch);
    for (calib = min_calib; calib < max_calib; calib += stepsize) {
      pflib_log(info) << "CALIB = " << calib;
      std::map<std::string, std::map<std::string, uint64_t>> parameters;
      if (ch < 36) {
        parameters["REFERENCEVOLTAGE_0"]["CALIB"] = calib;
        parameters[channel_page]["HIGHRANGE"] = 1;
      } else {
        parameters["REFERENCEVOLTAGE_1"]["CALIB"] = calib;
        parameters[channel_page]["HIGHRANGE"] = 1;
      }
      auto test_params = tgt->tempApplyAllROCs(parameters);

      for (charge_to_l1a = central_charge_to_l1a + start_bx;
           charge_to_l1a < central_charge_to_l1a + start_bx + n_bx;
           charge_to_l1a++) {
        tgt->fc().fc_setup_calib(charge_to_l1a);
        pflib_log(info) << "charge_to_l1a = " << tgt->fc().fc_get_setup_calib();
        for (phase_strobe = 0; phase_strobe < n_phase_strobe; phase_strobe++) {
          std::map<std::string, std::map<std::string, uint64_t>>
              phase_parameters;
          phase_parameters["TOP"]["PHASE_STROBE"] = phase_strobe;
          auto test_phase_params = tgt->tempApplyAllROCs(phase_parameters);
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
