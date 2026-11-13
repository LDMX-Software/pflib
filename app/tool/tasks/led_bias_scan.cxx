#include "led_bias_scan.h"

#include <filesystem>
#include <nlohmann/json.hpp>

#include "../daq_run.h"
#include "../pftool.h"
#include "pflib/Bias.h"
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
  // static void bias(const std::string& cmd, pflib::HcalTarget* pft){
  auto& bias = hcalbp->bias(iboard);

  int LEDstart = pftool::readline_int(
      "Which LED DAC start value (set equal to end for constant)? ", 2000);
  int LEDend = pftool::readline_int("Which LED DAC end value? ", 2500);
  int LEDstep = pftool::readline_int("What LED DAC stepsize? ", 50);

  if (LEDstart < 0 || LEDstart > 4095 || LEDend < 0 || LEDend > 4095 || LEDend < LEDstart) {
      PFEXCEPTION_RAISE("ValueError", "Invalid LED DAC range");
  }

  if (LEDstep <= 0 || (LEDend - LEDstart) % LEDstep != 0) {
      PFEXCEPTION_RAISE("ValueError", "Invalid LED DAC stepsize");
  }

  int SiPMstart = pftool::readline_int(
      "Which SiPM DAC start value (set equal to end for constant)? ", 3600);
  int SiPMend = pftool::readline_int("Which SiPM DAC end value? ", 3600);
  int SiPMstep =
      pftool::readline_int("What stepsize (choose random for constant)? ", 20);
  if (SiPMstart < 0 || SiPMstart > 4095 || SiPMend < 0 || SiPMend > 4095 || SiPMend < SiPMstart) {
      PFEXCEPTION_RAISE("ValueError", "Invalid SiPM DAC range");
  }

  if (SiPMstep <= 0 || (SiPMend - SiPMstart) % SiPMstep != 0) {
      PFEXCEPTION_RAISE("ValueError", "Invalid SiPM DAC stepsize");
  }

  int min_cmb_port_flash = pftool::readline_int("CMB port to start flashing LEDs on? ", 0);
  int max_cmb_port_flash = pftool::readline_int(
      "CMB port to end flashing LEDs on (if only one port, enter same as above)? ", 0);
  bool rec = pftool::readline_bool("Do you want to record on another CMB (no LED flashing)[Y] or not [N]?", false);
  int min_cmb_port_rec = 0;
  int max_cmb_port_rec = -1;
  if (rec) {
    min_cmb_port_rec = pftool::readline_int("CMB port to start recording on (no LED bias)?", 1);
    max_cmb_port_rec = pftool::readline_int("CMB port to end recording on (if only one port enter same as above)?", 1);
  }

  if (min_cmb_port_flash < 0 || max_cmb_port_flash > 15 || min_cmb_port_flash > max_cmb_port_flash) {
      PFEXCEPTION_RAISE("ValueError", "Invalid flashing CMB port range");
  }

  if (rec) {
      if (min_cmb_port_rec < 0 || max_cmb_port_rec > 15 || min_cmb_port_rec > max_cmb_port_rec) {
          PFEXCEPTION_RAISE("ValueError", "Invalid recording CMB port range");
      }
  }

  if (rec) {
      if (min_cmb_port_flash <= max_cmb_port_rec && min_cmb_port_rec <= max_cmb_port_flash) {
          PFEXCEPTION_RAISE("ValueError", "Flashing and recording CMB port ranges must not overlap");
      }
  }

  int nevents = pftool::readline_int("How many events per time point? ", 1);
  int start_bx = pftool::readline_int("Starting BX? ", 0);
  int n_bx = pftool::readline_int("Number of BX? ", 10);
  int start_led = tgt->fc().fc_get_setup_led();
  int end_led = pftool::readline_int(
      "Calibration L1A offset for LED? (Hopefully 16-20)", start_led);
  tgt->fc().fc_setup_led(end_led);

  pflib::ROC roc{tgt->roc(iboard)};

  std::string fname;
  fname = pftool::readline_path("led-bias-scan", ".csv");

  auto test_param_builder = roc.testParameters();

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

  for (int i = min_cmb_port_flash; i <= max_cmb_port_flash; i++) {
    ch_to_SiPM[i] = bias.readSiPM(i).value_or(-1);
    ch_to_LED[i] = bias.readLED(i).value_or(-1);
  }

  if (rec) {
    for (int i = min_cmb_port_rec; i <= max_cmb_port_rec; i++) {
        ch_to_SiPM[i] = bias.readSiPM(i).value_or(-1);
        ch_to_LED[i] = bias.readLED(i).value_or(-1);
    }
  }

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
  double temperature{0};

  // for CMB ports that just record data but don't flash the LEDs
  DecodeAndWriteToCSV writer{
      fname,
      [&](std::ofstream& f) {
        nlohmann::ordered_json header;
        header["Min CMB port with LED flashing"] = min_cmb_port_flash;
        header["Max CMB port with LED flashing"] = max_cmb_port_flash;
        header["Second CMB recording enabled"] = rec;
        if (rec) {
          header["Min CMB port only recording"] = min_cmb_port_rec;
          header["Max CMB port only recording"] = max_cmb_port_rec;
        }
        header["LED DAC start"] = LEDstart;
        header["LED DAC end"] = LEDend;
        header["SiPM DAC start"] = SiPMstart;
        header["SiPM DAC end"] = SiPMend;
        f << std::boolalpha << "# " << header << '\n'
          << "time,cmb_role,i_cmb_port,ch,dacSiPM,dacLED,temperature,"
          << pflib::packing::Sample::to_csv_header << '\n';
      },
      [&](std::ofstream& f,
          const pflib::packing::MultiSampleECONDEventPacket& ep) {

        auto write_ports = [&](int min_port, int max_port, const std::string& role, uint16_t led_dac) {
          for (int port = min_port; port <= max_port; port++) {
            for (int j = 0; j < 4; j++) {

              int ch = cmb_to_ch[port][j];
              auto [i_erx, i_ch] = mapping.toErxChannel(iboard, ch);

              f << time << ',' << role << ',' << port << ',' << ch << ',' << dacSiPM << ','
                << led_dac << ',' << temperature << ',';

              ep.samples[ep.i_soi].channel(i_erx, i_ch).to_csv(f);
              f << '\n';
            }
          }
        };

        write_ports(min_cmb_port_flash, max_cmb_port_flash, "flash", dacLED);
        if (rec) {
          write_ports(min_cmb_port_rec, max_cmb_port_rec, "record", 0);
        }
      },
      n_links};

  tgt->setup_run(1, Target::DaqFormat::ECOND_SW_HEADERS, 1);

  bool enable_l1a_follow;
  central_charge_to_l1a = tgt->fc().fc_get_setup_led();

  for (dacSiPM = SiPMstart; dacSiPM <= SiPMend; dacSiPM += SiPMstep) {
    for (dacLED = LEDstart; dacLED <= LEDend; dacLED += LEDstep) {
      // Setting SiPM and LED bias on CMB with LED flashing
      for (int i = min_cmb_port_flash; i <= max_cmb_port_flash; i++) {
          usleep(10);
          bias.setSiPM(i, dacSiPM);
          usleep(10);
          bias.setLED(i, dacLED);

          pflib_log(info) << "Flashing CMB port " << i
                          << " | SiPM bias = " << dacSiPM
                          << " | LED bias = " << dacLED;
      }

      // Setting only SiPM bias and zero LED bias on recording CMB
      if (rec) {
          for (int i = min_cmb_port_rec; i <= max_cmb_port_rec; i++) {
              usleep(10);
              bias.setSiPM(i, dacSiPM);
              usleep(10);
              bias.setLED(i, 0);

              pflib_log(info) << "Recording CMB port " << i
                              << " | SiPM bias = " << dacSiPM
                              << " | LED bias = 0";
          }
      }

      temperature = bias.readTemp();

      for (charge_to_l1a = central_charge_to_l1a + start_bx;
          charge_to_l1a < central_charge_to_l1a + start_bx + n_bx;
          charge_to_l1a++) {
          tgt->fc().fc_setup_led(charge_to_l1a);
          for (phase_strobe = 0; phase_strobe < n_phase_strobe;
              phase_strobe++) {
              auto phase_strobe_test_handle =
                  roc.testParameters()
                      .add("TOP", "PHASE_STROBE", phase_strobe)
                      .apply();
              usleep(10);
              time =
                  (charge_to_l1a - central_charge_to_l1a + offset) * clock_cycle -
                  phase_strobe * clock_cycle / n_phase_strobe;
              daq_run(tgt, "LED", writer, nevents, pftool::state.daq_rate);
          }
      }
      // reset charge_to_l1a to central value
      tgt->fc().fc_setup_led(central_charge_to_l1a);
    }
  }
  // reset the biases to original value for all channels
  for (int i = min_cmb_port_flash; i <= max_cmb_port_flash; i++) {
    bias.setSiPM(i, ch_to_SiPM[i]);
    bias.setLED(i, ch_to_LED[i]);
  }

  // reset the biases to original value for all channels
  if (rec) {
    for (int i = min_cmb_port_rec; i <= max_cmb_port_rec; i++) {
      bias.setSiPM(i, ch_to_SiPM[i]);
      bias.setLED(i, ch_to_LED[i]);
    }
  }

}
