#include "led_bias_scan.h"
#include <filesystem>
#include <nlohmann/json.hpp> 

#include "../daq_run.h"
#include "pflib/utility/string_format.h"
#include "pflib/HcalBackplane.h"
#include "pflib/zcu/HGCROCBoardFiberless.h"
#include "pftool.h"

ENABLE_LOGGING();

void led_bias_scan(Target* tgt){
  //basically taken straight from the current version of bias.cxx render function
  auto hcalbp = dynamic_cast<pflib::HcalBackplane*>(tgt);
  auto hcalfl = dynamic_cast<pflib::HcalFiberless*>(tgt);
  if (!hcalbp && !hcalfl) {
      PFEXCEPTION_RAISE("ValueError", "led_bias_scan only available for HcalBackplane or HcalFiberless targets.");
  }
  int iboard = 0; 
  iboard = pftool::readline_int("Which board? ", iboard);
  pflib::Bias bias = hcalbp ? hcalbp->bias(iboard) : hcalfl->bias(iboard); //this one gets the bias method

  uint16_t LEDstart = pftool::readline_int("Which LED DAC start value (set equal to end for constant)? ", 0);
  uint16_t LEDend = pftool::readline_int("Which LED DAC end value? ", 0);
  int LEDstep = pftool::readline_int("What stepsize (choose random for constant)? ", 20);
  if ((LEDend - LEDstart) % LEDstep != 0){
    PFEXCEPTION_RAISE("ValueError", "Chosen LED DAC range needs to be divisible by stepsize without remainder.");
  }

  uint16_t SiPMstart = pftool::readline_int("Which SiPM DAC start value (set equal to end for constant)? ", 0);
  uint16_t SiPMend = pftool::readline_int("Which SiPM DAC end value? ", 0);
  int SiPMstep = pftool::readline_int("What stepsize (choose random for constant)? ", 20);
  if ((SiPMend - SiPMstart) % SiPMstep != 0){
    PFEXCEPTION_RAISE("ValueError", "Chosen SiPM DAC range needs to be divisible by stepsize without remainder.");
  }

  // TODO: raise exception if values are either very high (too high voltage kills stuff :D), or outside of doable range

  int min_cmb_ch = pftool::readline_int("Channel to start scan on? ", 0);
  int max_cmb_ch = pftool::readline_int("Channel to end scan on (if only one channel, enter same as above)? ", 15);
  int nevents = pftool::readline_int("How many events per time point? ", 1);
  int start_bx = pftool::readline_int("Starting BX? ", -1);
  int n_bx = pftool::readline_int("Number of BX? ", 3);

  pflib::ROC roc{tgt->roc(pftool::state.iroc)};
  std::string fname;
  auto test_param_builder = roc.testParameters();
  fname = pftool::readline_path("led-bias-scan", ".csv");

  // Makes sure charge injections are turned off (in all channels)
  for (int ch = 0; ch < 72; ch++) {
    int link = (ch / 36); 
    auto channel_page = pflib::utility::string_format("CH_%d", ch);
    auto refvol_page = pflib::utility::string_format("REFERENCEVOLTAGE_%d", link);
    test_param_builder.add(refvol_page, "CALIB", 0)
    .add(refvol_page, "CALIB_2V5", 0)
    .add(refvol_page, "INTCTEST", 1)
    .add(refvol_page, "CHOICE_CINJ", 0)
    .add(channel_page, "HIGHRANGE", 0)
    .add(channel_page, "LOWRANGE", 0);
  }

  auto test_param_handle = test_param_builder.apply();

  int i_cmb_ch{0};
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
        nlohmann::json header;
        header["Min channel"] = min_cmb_ch;
        header["Max channel"] = max_cmb_ch;
        header["LED DAC start"] = LEDstart;
        header["LED DAC end"] = LEDend;
        header["SiPM DAC start"] = SiPMstart;
        header["SiPM DAC end"] = SiPMend;
        f << std::boolalpha << "# " << header << '\n'
          << "time, i_cmb_ch, dacSiPM, dacLED," << pflib::packing::Sample::to_csv_header << '\n';
      },
      [&](std::ofstream& f,
          const pflib::packing::MultiSampleECONDEventPacket& ep) {
        f << time << ',' << i_cmb_ch << ',' << dacSiPM << ',' << dacLED << ',';
        ep.samples[ep.i_soi].channel(i_cmb_ch / 36, i_cmb_ch % 36).to_csv(f);
        f << '\n';
      },
      n_links};

  tgt->setup_run(1, Target::DaqFormat::ECOND_SW_HEADERS, 1);

  central_charge_to_l1a = tgt->fc().fc_get_setup_led();

  for (dacSiPM = SiPMstart; dacSiPM <= SiPMend; dacSiPM += SiPMstep) {
    for (dacLED = LEDstart; dacLED <= LEDend; dacLED += LEDstep) {
      for (i_cmb_ch = min_cmb_ch; i_cmb_ch <= max_cmb_ch; i_cmb_ch++) {

        bias.setSiPM(i_cmb_ch, dacSiPM);
        bias.setLED(i_cmb_ch, dacLED);

        for (charge_to_l1a = central_charge_to_l1a + start_bx;
            charge_to_l1a < central_charge_to_l1a + start_bx + n_bx;
            charge_to_l1a++) {

          tgt->fc().fc_setup_led(charge_to_l1a);
          pflib_log(info) << "led_to_l1a = " << tgt->fc().fc_get_setup_led();

          for (phase_strobe = 0; phase_strobe < n_phase_strobe; phase_strobe++) {
            auto phase_strobe_test_handle =
                roc.testParameters().add("TOP", "PHASE_STROBE", phase_strobe).apply();
            pflib_log(info) << "TOP.PHASE_STROBE = " << phase_strobe;
            usleep(10);  // make sure parameters are applied
            time = (charge_to_l1a - central_charge_to_l1a + offset) * clock_cycle -
                  phase_strobe * clock_cycle / n_phase_strobe;
            daq_run(tgt, "LED", writer, nevents, pftool::state.daq_rate);
          }
        }
      }
      // reset charge_to_l1a to central value
      tgt->fc().fc_setup_led(central_charge_to_l1a);
    }
  }
}