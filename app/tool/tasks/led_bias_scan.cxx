#include "led_bias_scan.h"

#include <filesystem>
#include <nlohmann/json.hpp>

#include "../daq_run.h"
#include "../pftool.h"
#include "pflib/HcalTarget.h"
#include "pflib/utility/string_format.h"

ENABLE_LOGGING();

void led_bias_scan(Target* tgt) {
  // mapping of CMB ports to HGCROC channels, because some channels are skipped
  //  first entry is CMB port 0, then CMB port 1, etc. and the individual
  //  numbers are the HGCROC channels
  int cmb_to_ch[16][4] = {
      {0, 1, 2, 3},     {4, 5, 6, 7},     {9, 10, 11, 12},  {13, 14, 15, 16},
      {18, 19, 20, 21}, {22, 23, 24, 25}, {27, 28, 29, 30}, {31, 32, 33, 34},
      {36, 37, 38, 39}, {40, 41, 42, 43}, {45, 46, 47, 48}, {49, 50, 51, 52},
      {54, 55, 56, 57}, {58, 59, 60, 61}, {63, 64, 65, 66}, {67, 68, 69, 70}};

  auto hcalbp = dynamic_cast<pflib::HcalTarget*>(tgt);
  if (!hcalbp) {
    PFEXCEPTION_RAISE("BadTarget",
                      "led_bias_scan only available for Hcal targets");
  }
  int iboard = 1;
  iboard = pftool::readline_int("Which board? ", iboard);
  // static void bias(const std::string& cmd, pflib::HcalBackplane* pft){
  pflib::Bias bias = hcalbp->bias(iboard);  // bad method

  uint16_t LEDstart = pftool::readline_int(
      "Which LED DAC start value (set equal to end for constant)? ", 2000);
  uint16_t LEDend = pftool::readline_int("Which LED DAC end value? ", 2500);
  int LEDstep =
      pftool::readline_int("What stepsize (choose random for constant)? ", 50);
  if ((LEDend - LEDstart) % LEDstep != 0) {
    PFEXCEPTION_RAISE("ValueError",
                      "Chosen LED DAC range needs to be divisible by stepsize "
                      "without remainder.");
  }

  uint16_t SiPMstart = pftool::readline_int(
      "Which SiPM DAC start value (set equal to end for constant)? ", 3600);
  uint16_t SiPMend = pftool::readline_int("Which SiPM DAC end value? ", 3600);
  int SiPMstep =
      pftool::readline_int("What stepsize (choose random for constant)? ", 20);
  if ((SiPMend - SiPMstart) % SiPMstep != 0) {
    PFEXCEPTION_RAISE("ValueError",
                      "Chosen SiPM DAC range needs to be divisible by stepsize "
                      "without remainder.");
  }
  if (SiPMstart < 0) {
    PFEXCEPTION_RAISE("ValueError", "Chosen SiPM_start needs to be above 0");
  }
  if (SiPMend > 4095) {
    PFEXCEPTION_RAISE("ValueError", "Chosen SiPM_end needs to be below 4095");
  }
  if (SiPMend < 0) {
    PFEXCEPTION_RAISE("ValueError", "Chosen SiPM_end needs to be above 0");
  }
  if (SiPMstart > 4095) {
    PFEXCEPTION_RAISE("ValueError", "Chosen SiPM_start needs to be below 4095");
  }

  int min_cmb_port = pftool::readline_int("Channel to start scan on? ", 0);
  int max_cmb_port = pftool::readline_int(
      "Channel to end scan on (if only one channel, enter same as above)? ", 0);
  int nevents = pftool::readline_int("How many events per time point? ", 1);
  int start_bx = pftool::readline_int("Starting BX? ", 0);
  int n_bx = pftool::readline_int("Number of BX? ", 10);
  int start_led = tgt->fc().fc_get_setup_led();
  int end_led = pftool::readline_int(
      "Calibration L1A offset for LED? (Hopefully 16-20)", start_led);
  tgt->fc().fc_setup_led(end_led);

  pflib::ROC roc{tgt->roc(iboard)};

  std::string fname;
  auto test_param_builder = roc.testParameters();
  fname = pftool::readline_path("led-bias-scan", ".csv");

  // Makes sure charge injections are turned off (in all channels)
  for (int ch = 0; ch < 72; ch++) {
    int link = (ch / 36);
    auto channel_page = pflib::utility::string_format("CH_%d", ch);
    auto refvol_page =
        pflib::utility::string_format("REFERENCEVOLTAGE_%d", link);
    test_param_builder.add(refvol_page, "CALIB", 0)
        .add(refvol_page, "CALIB_2V5", 0)
        .add(refvol_page, "INTCTEST", 1)
        .add(refvol_page, "CHOICE_CINJ", 0)
        .add(channel_page, "HIGHRANGE", 0)
        .add(channel_page, "LOWRANGE", 0);
  }

  auto test_param_handle = test_param_builder.apply();

  auto& mapping{tgt->getRocErxMapping()};

  std::map<int, int> ch_to_SiPM;
  std::map<int, int> ch_to_LED;

  for (int i = min_cmb_port; i <= max_cmb_port; i++) {
    ch_to_SiPM[i] = bias.readSiPM(i);
    ch_to_LED[i] = bias.readLED(i);
  }

  int i_cmb_port{0};
  int ch{0};
  uint16_t dacSiPM{0};
  uint16_t dacLED{0};
  int central_charge_to_l1a;
  int charge_to_l1a{0};
  int phase_strobe{0};
  double time{0};
  double clock_cycle{25.0};
  int n_phase_strobe{16};
  int offset{1};
  int n_links = 2 * tgt->nrocs();

  DecodeAndWriteToCSV writer{
      fname,
      [&](std::ofstream& f) {
        nlohmann::ordered_json header;
        header["Min CMB port"] = min_cmb_port;
        header["Max CMB port"] = max_cmb_port;
        header["LED DAC start"] = LEDstart;
        header["LED DAC end"] = LEDend;
        header["SiPM DAC start"] = SiPMstart;
        header["SiPM DAC end"] = SiPMend;
        f << std::boolalpha << "# " << header << '\n'
          << "time,i_cmb_port,ch,dacSiPM,dacLED,"
          << pflib::packing::Sample::to_csv_header << '\n';
      },
      [&](std::ofstream& f,
          const pflib::packing::MultiSampleECONDEventPacket& ep) {
        for (int j = 0; j < 4; j++) {
          ch = cmb_to_ch[i_cmb_port][j];
          auto [i_erx, i_ch] = mapping.toErxChannel(iboard, ch);
          f << time << ',' << i_cmb_port << ',' << i_ch << ',' << dacSiPM << ','
            << dacLED << ',';
          ep.samples[ep.i_soi].channel(i_erx, i_ch).to_csv(f);
          f << '\n';
        }
      },
      n_links};

  tgt->setup_run(1, Target::DaqFormat::ECOND_SW_HEADERS, 1);

  bool enable_l1a_follow;
  central_charge_to_l1a = tgt->fc().fc_get_setup_led();

  for (dacSiPM = SiPMstart; dacSiPM <= SiPMend; dacSiPM += SiPMstep) {
    pflib_log(info) << "DAC SiPM = " << dacSiPM;
    for (dacLED = LEDstart; dacLED <= LEDend; dacLED += LEDstep) {
      pflib_log(info) << "DAC LED = " << dacLED;
      for (i_cmb_port = min_cmb_port; i_cmb_port <= max_cmb_port;
           i_cmb_port++) {
        usleep(10);
        bias.setSiPM(i_cmb_port, dacSiPM);
        usleep(10);
        bias.setLED(i_cmb_port, dacLED);

        for (charge_to_l1a = central_charge_to_l1a + start_bx;
             charge_to_l1a < central_charge_to_l1a + start_bx + n_bx;
             charge_to_l1a++) {
          tgt->fc().fc_setup_led(charge_to_l1a);
          pflib_log(info) << "led_to_l1a = " << tgt->fc().fc_get_setup_led();

          for (phase_strobe = 0; phase_strobe < n_phase_strobe;
               phase_strobe++) {
            auto phase_strobe_test_handle =
                roc.testParameters()
                    .add("TOP", "PHASE_STROBE", phase_strobe)
                    .apply();
            pflib_log(info) << "TOP.PHASE_STROBE = " << phase_strobe;
            usleep(10);  // make sure parameters are applied
            time =
                (charge_to_l1a - central_charge_to_l1a + offset) * clock_cycle -
                phase_strobe * clock_cycle / n_phase_strobe;
            daq_run(tgt, "LED", writer, nevents, pftool::state.daq_rate);
          }
        }
      }
      // reset charge_to_l1a to central value
      tgt->fc().fc_setup_led(central_charge_to_l1a);
    }
  }
  // reset the biases to zero for all channels
  for (int i = min_cmb_port; i <= max_cmb_port; i++) {
    bias.setSiPM(i, ch_to_SiPM[i]);
    bias.setLED(i, ch_to_LED[i]);
  }
}
