#include "SPS_readout.h"

#include <filesystem>
#include <nlohmann/json.hpp>

#include "../daq_run.h"
#include "../pftool.h"
#include "pflib/Bias.h"
#include "pflib/HcalTarget.h"
#include "pflib/TRIG.h"
#include "pflib/packing/Hex.h"
#include "pflib/utility/string_format.h"

ENABLE_LOGGING();

void sps_readout(Target* tgt) {
  int cmb_to_ch[16][4] = {
      {0, 1, 2, 3},     {4, 5, 6, 7},     {9, 10, 11, 12},  {13, 14, 15, 16},
      {18, 19, 20, 21}, {22, 23, 24, 25}, {27, 28, 29, 30}, {31, 32, 33, 34},
      {36, 37, 38, 39}, {40, 41, 42, 43}, {45, 46, 47, 48}, {49, 50, 51, 52},
      {54, 55, 56, 57}, {58, 59, 60, 61}, {63, 64, 65, 66}, {67, 68, 69, 70}};

  tgt->setup_run(1, Target::DaqFormat::ECOND_SW_HEADERS, 1);
  pflib::DAQ& daq = tgt->daq();

  auto hcalbp = dynamic_cast<pflib::HcalTarget*>(tgt);
  if (!hcalbp) {
    PFEXCEPTION_RAISE("BadTarget",
                      "led_bias_scan only available for Hcal targets");
  }

  int iboard = 1;
  iboard = pftool::readline_int("Which board? ", iboard);
  // static void bias(const std::string& cmd, pflib::HcalTarget* pft){
  auto& bias = hcalbp->bias(iboard);
  auto& mapping{tgt->getRocErxMapping()};

  int cmb_port = pftool::readline_int("Channel to scan on? ", 0);
  int nevents = pftool::readline_int("How many events per time point? ", 1000);
  int start_led = tgt->fc().fc_get_setup_led();
  int tgt_bx =
      pftool::readline_int("Target BX? (~22 should be BX = 4) ", start_led);
  int len_bx = pftool::readline_int("Number of BX to scan over? ", 2);
  tgt->fc().setL1AperROR(len_bx);
  // int start_led_new = pftool::readline_int("Calibration L1A offset for LED
  // start point", start_led);
  int n_links = 2 * tgt->nrocs();

  int start_SiPM = bias.readSiPM(cmb_port).value_or(-1);
  int start_LED = bias.readLED(cmb_port).value_or(-1);
  int new_SiPM = pftool::readline_int("SiPM DAC value? ", start_SiPM);
  int new_LED = pftool::readline_int("LED DAC value? ", start_LED);

  int start_phase_ck = pftool::readline_int("Starting PHASE_CK value? ", 2);
  int end_phase_ck = pftool::readline_int("Ending PHASE_CK value? ", 2);
  int trim_inv = pftool::readline_int("TRIM_INV value? ", 2);
  int trim_range = pftool::readline_int(
      "Range of TRIM_INV values to scan over for convolution? ", 1);

  bias.setSiPM(cmb_port, new_SiPM);
  bias.setLED(cmb_port, new_LED);

  pflib::ROC roc{tgt->roc(iboard)};

  std::string fname;
  auto test_param_builder = roc.testParameters();
  fname = pftool::readline_path("SPS-scan", ".csv");

  int g = 0;
  int phase_ck = 0;

  DecodeAndWriteToCSV writer{
      fname,
      [&](std::ofstream& f) {
        nlohmann::ordered_json header;
        f << std::boolalpha << "# " << header << '\n'
          << "i_cmb_port,ch,trim_inv,phase_ck,SiPM_DAC,LED_DAC,"
          << pflib::packing::Sample::to_csv_header << '\n';
      },
      [&](std::ofstream& f,
          const pflib::packing::MultiSampleECONDEventPacket& ep) {
        for (int j = 0; j < 4; j++) {
          auto ch = cmb_to_ch[cmb_port][j];
          auto [i_erx, i_ch] = mapping.toErxChannel(iboard, ch);
          f << cmb_port << ',' << i_ch << ',' << g << ',' << phase_ck << ','
            << new_SiPM << ',' << new_LED << ',';
          ep.samples[ep.i_soi].channel(i_erx, i_ch).to_csv(f);
          f << '\n';
        }
      },
      n_links};

  // Makes sure charge injections are turned on for this individual channel
  for (int j = 0; j < 4; j++) {
    auto ch = cmb_to_ch[cmb_port][j];
    int link = (ch / 36);
    auto channel_page = pflib::utility::string_format("CH_%d", ch);
    auto refvol_page =
        pflib::utility::string_format("REFERENCEVOLTAGE_%d", link);
    auto calib_page = pflib::utility::string_format("CALIB_%d", link);
    auto global_page = pflib::utility::string_format("GLOBALANALOG_%d", link);
    test_param_builder.add(refvol_page, "CALIB", 0)
        .add(refvol_page, "CALIB_2V5", 0)
        .add(refvol_page, "INTCTEST", 1)
        .add(refvol_page, "CHOICE_CINJ", 0)
        .add(global_page, "CD", 2)
        .add(global_page, "CF", 8)
        .add(global_page, "RF", 10)
        .add(channel_page, "HIGHRANGE", 0)
        .add(channel_page, "LOWRANGE", 0)
        .add(calib_page, "INPUTDAC",
             32)  // No idea what this should be (MAXES out at 63)
        .add(channel_page, "INPUTDAC", 32)  // No idea what this should be
        .add(global_page, "GAIN_CONV", 1)   // 0 or 1
        .add(calib_page, "GAIN_CONV", 7);   // 0 or 1
    for (int k = 0; k < 4; k++) {
      auto cm_page = pflib::utility::string_format("CM_%d", link);
      test_param_builder.add(cm_page, "GAIN_CONV", 7);  // 0 to 7
    }
    test_param_builder.add(channel_page, "GAIN_CONV", 7);  // 0 to 7
  }

  auto test_param_handle = test_param_builder.apply();

  for (g = (trim_inv - trim_range); g < (trim_inv + trim_range + 1); g++) {
    pflib_log(info) << "TRIM_INV set to = " << g;

    std::map<std::string, std::map<std::string, uint64_t>> page_stat;

    for (int j = 0; j < 4; j++) {
      auto ch = cmb_to_ch[cmb_port][j];
      auto ch_str = pflib::utility::string_format("CH_%d", ch);
      page_stat[ch_str]["TRIM_INV"] = g;
    }
    auto trim_inv_apply = tgt->tempApplyAllROCs(page_stat);

    for (phase_ck = start_phase_ck; phase_ck <= end_phase_ck; phase_ck++) {
      pflib_log(info) << "PHASE_CK = " << phase_ck;

      auto phase_test_handle =
          roc.testParameters().add("TOP", "PHASE_CK", phase_ck).apply();

      bool enable_l1a_follow;
      // int central_charge_to_l1a = tgt->fc().fc_get_setup_led();

      // tgt->fc().fc_setup_led(start_led_new);
      tgt->fc().fc_setup_led(tgt_bx);
      pflib_log(info) << " Target BX  = " << tgt_bx << "\n";

      daq_run(tgt, "LED", writer, nevents, pftool::state.daq_rate);
      usleep(10);
      // auto data = buffer.get_buffer();
      // auto mapping = tgt->getRocErxMapping();
      // auto [i_erx, i_ch] = mapping.toErxChannel(i_roc, 17);
      // for (std::size_t i{0}; i < data.size(); i++) {
      // for (int j = 0; j < nr_bx; j++) {
      // adcs[j].push_back(data[i].samples.at(j).channel(i_erx, i_ch).adc());
      //}
      //}
    }
  }

}  // sps_readout
