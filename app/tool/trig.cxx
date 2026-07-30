/**
 * @file trig.cxx
 * TRIG menu commands
 */
#include "pflib/TRIG.h"

#include <optional>

#include "pflib/packing/Hex.h"
#include "pflib/packing/MultiSampleECONDEventPacket.h"
#include "pflib/packing/SingleECONTCaptureFrame.h"
#include "pflib/packing/TrigAlgoOutput.h"
#include "pflib/utility/string_format.h"
#include "pftool.h"


using pflib::packing::SingleECONTCaptureFrame;
using pflib::packing::TrigAlgoOutput;

ENABLE_LOGGING();

void trig_render(Target* tgt) {}

template <class SampleFrame>
std::vector<SampleFrame> decode_multi_sample(int n_samples,
                                             std::vector<uint32_t>& data) {
  std::vector<SampleFrame> frames(n_samples);
  int offset{0};
  for (int i{0}; i < frames.size(); i++) {
    frames[i].from(std::span<uint32_t>(data.begin() + offset, data.end()));
    offset += frames[i].length();
  }
  return frames;
}

/**
 * Interaction with trigger alignment, capture, and algo
 *
 * ## Commands
 * - RESET : call reset on the TRIG block (TRIG::reset)
 * - ELINK_SPY : spy on six TRIG elinks (Elinks::spy)
 * - EVENT_SPY : readout the last captured event (TRIG::read_event)
 * - PIPELINE : change depth of capture buffer for readout
 * - SAMPLES_PER_L1A : number of samples to readout per l1a
 * - PRESAMPLES : number of "pre-samples" to include in readout
 * - BUFFER_CLEAR : readout samples until buffer is emtpy
 */
void trig(const std::string& cmd, Target* target) {
  static std::string olink_name;
  pflib::TRIG* trig = target->trig();
  if (trig == 0) return;

  if (cmd == "RESET") {
    trig->reset();
  }
  if (cmd == "STATUS") {
    int pipeline{-1}, econ_id{-1}, samples_per_l1a{-1}, presamples{-1};
    trig->get_daq_setup(pipeline, econ_id, samples_per_l1a, presamples);
    printf("settings\n");
    printf(" %15s: %d\n", "pipeline", pipeline);
    printf(" %15s: %d\n", "econ_id", econ_id);
    printf(" %15s: %d\n", "samples_per_l1a", samples_per_l1a);
    printf(" %15s: %d\n", "presamples", presamples);
    printf(" %15s: %d\n", "capture delay", trig->get_alignment_capture());
    for (int ilink{0}; ilink < trig->n_elinks(); ilink++) {
      printf("      %d bx delay: %d\n", ilink, trig->get_bx_delay(ilink));
    }
    printf("counters\n");
    printf(" %15s: %d\n", "self-triggers", trig->get_self_trigger_count());
    printf(" %15s: %d\n", "event occupancy", target->daq().getEventOccupancy());
    printf("flags\n");
    printf(" %15s: %d\n", "trig sample", trig->is_sample_available());
    printf(" %15s: %d\n", "algo output", trig->is_algo_output_available());
  }
  if (cmd == "PIPELINE" or cmd == "SAMPLES_PER_L1A" or cmd == "PRESAMPLES" or
      cmd == "ECONID") {
    int pipeline{-1}, econ_id{-1}, samples_per_l1a{-1}, presamples{-1};
    trig->get_daq_setup(pipeline, econ_id, samples_per_l1a, presamples);
    if (cmd == "PIPELINE") {
      pipeline = pftool::readline_int("pipeline depth: ", pipeline);
    } else if (cmd == "SAMPLES_PER_L1A") {
      samples_per_l1a =
          pftool::readline_int("number of samples per L1A: ", samples_per_l1a);
    } else if (cmd == "PRESAMPLES") {
      presamples = pftool::readline_int("number of pre-samples: ", presamples);
    } else if (cmd == "ECONID") {
      econ_id = pftool::readline_int("set econ id: ", econ_id);
    }
    trig->setup_daq(pipeline, econ_id, samples_per_l1a, presamples);
  }
  if (cmd == "ELINK_SPY") {
    pflib::Elinks& elinks = target->elinks();
    std::vector<std::vector<uint32_t>> spy(6);
    printf("word :");
    for (int ilink{0}; ilink < spy.size(); ilink++) {
      spy[ilink] = elinks.spy(6 + ilink, ilink == 0);
      printf("  Link %2d", ilink);
    }
    printf("\n");
    for (int iword{0}; iword < spy[0].size(); iword++) {
      printf("%4d :", iword);
      for (int ilink{0}; ilink < spy.size(); ilink++) {
        printf(" %08x", spy[ilink][iword]);
      }
      printf("\n");
    }
  }
  if (cmd == "EVENT_SPY") {
    std::vector<uint32_t> event = trig->read_event();
    if (event.empty()) {
      pflib_log(info) << "no event available";
      return;
    }
    for (int iword{0}; iword < event.size(); iword++) {
      uint32_t word{event[iword]};
      printf("%08x\n", word);
    }

    std::vector<SingleECONTCaptureFrame> frames =
        decode_multi_sample<SingleECONTCaptureFrame>(trig->get_l1a_per_ror(),
                                                     event);
    printf("BX STC0 STC1 STC2 STC3 STC4 STC5 STC6 STC7\n");
    for (int i_l1a{0}; i_l1a < frames.size(); i_l1a++) {
      const auto& frame{frames[i_l1a]};
      for (int i{0}; i < frame.n_samples(); i++) {
        printf("%2d", frame.bx(i));
        for (int j{0}; j < 8; j++) {
          printf(" %4d", frame.stc_sum(j, i));
        }
        printf("\n");
      }
    }
  }
  if (cmd == "BUFFER_CLEAR") {
    while (trig->is_sample_available()) {
      trig->read_sample();
      usleep(100000);
    }
  }
}

/**
 * TRIG.ALIGN commands
 *
 * - SETUP : setup the alignment capture time
 * - READ : read the alignment capture buffer
 * - DELAY : set the delay for one elink
 */
void align(const std::string& cmd, Target* tgt) {
  pflib::TRIG* trig = tgt->trig();
  if (trig == 0) return;
  if (cmd == "SETUP") {
    int value = pftool::readline_int("Alignment capture delay: ",
                                     trig->get_alignment_capture());
    trig->setup_alignment_capture(value);
  }
  if (cmd == "READ") {
    bool show_raw = pftool::readline_bool(
        "Show raw data [Y] or idle word interpretation [N]? ", false);
    tgt->fc().linkreset_econs();
    usleep(3000);
    for (int ilink = 0; ilink < trig->n_elinks(); ilink++) {
      std::vector<uint32_t> val = trig->read_capture_buffer(ilink);
      // see Section 20 of the ECON-T manual,
      // at UMN we have ECON-T-P1 on the ECON Mezzanine
      // each eTx produces two 16-bit IDLE words per BX
      // and an IDLE word looks like
      //  5b BX | 11b Pattern
      // where BX is a BX counter and the 11b Pattern is set by
      // formatterbuffer.global.link_reset_pattern ? or idle_pattern?
      printf("%02d :", ilink);
      for (auto x : val) {
        if (show_raw) {
          printf(" %08x", x);
        } else {
          printf(" | %02d %03x %02d %03x", (x >> (16 + 11)) & 0x1f,
                 (x >> 16) & 0x7ff, (x >> 11) & 0x1f, (x & 0x7ff));
        }
      }
      printf("\n");
    }
  }
  if (cmd == "DELAY") {
    int ilink = pftool::readline_int("Which elink?", 0);
    trig->set_bx_delay(
        ilink, pftool::readline_int("New delay: ", trig->get_bx_delay(ilink)));
  }
}

/**
 * TRIG.ALGO commands
 *
 * - SPY: view last captured trig algo output
 * - BUFFER_CLEAR: clear buffer by reading samples until its empty
 * - CONFIG: change settings of trigger algorithm
 * - STATUS: print settings of trigger algo and if output is available
 */
void algo(const std::string& cmd, Target* target) {
  pflib::TRIG* trig = target->trig();
  if (trig == 0) return;
  if (cmd == "SPY") {
    std::vector<uint32_t> algo = trig->read_algo_output();
    if (algo.empty()) {
      pflib_log(info) << "no algo output available";
    }
    for (int iword{0}; iword < algo.size(); iword++) {
      uint32_t word{algo[iword]};
      printf("%08x", word);
      // decoding
      printf("\n");
    }
  }
  if (cmd == "BUFFER_CLEAR") {
    while (trig->is_algo_output_available()) {
      trig->read_algo_output_sample();
      usleep(100000);
    }
  }
  if (cmd == "CONFIG") {
    auto params = trig->get_algo_setup();
    params[0] =
        pftool::readline_int("veto mask for recent history: ", params[0], true);
    for (int i{0}; i < 8; i++) {
      std::stringstream prompt;
      prompt << "threshold for channel " << i;
      params[i + 1] = pftool::readline_int(prompt.str(), params[i + 1], true);
    }
    trig->setup_algo(params);
  }
  if (cmd == "STATUS") {
    auto params = trig->get_algo_setup();
    printf("veto mask for recent history: 0x%02x\n", params[0]);
    printf("thresholds (0 -> disable channel)\n");
    for (int i{0}; i < 8; i++) {
      printf("  %d : 0x%02x\n", i, params[i + 1]);
    }
    printf("algo output available? %s\n",
           trig->is_algo_output_available() ? "yes" : "no");
  }
}

static int last_charge_to_l1a{},
           last_l1offset{},
           last_pipeline{};

/**
 * TRIG.TIMEIN
 */
static void trigger_timein(Target* tgt) {
  pflib::TRIG* trig = tgt->trig();
  if (trig == 0) return;

  /**
   * This command attempts to deduce the capture delay for the trigger
   * links by taking two runs after setting some parameters on the chip.
   *
   * Assuming the pedestal values on the chip are all ~200 (as is the case
   * at UMN), setting the CH_XX.ADC_PEDESTAL and DIGITALHALF_X.ADC_TH to
   * their maxima (255 and 31 respectively) forces the trigger sums to be
   * zero for pedestals.
   */

  pflib_log(info) << "setting up parameters for trigger link testing";

  std::map<std::string, std::map<std::string, uint64_t>> roc_setup;
  for (int half{0}; half < 2; half++) {
    roc_setup[pflib::utility::string_format("HALFWISE_%d", half)]
             ["ADC_PEDESTAL"] = 255;
    roc_setup[pflib::utility::string_format("DIGITALHALF_%d", half)]["ADC_TH"] =
        31;
    auto refvol_page{
        pflib::utility::string_format("REFERENCEVOLTAGE_%d", half)};
    roc_setup[refvol_page]["CALIB"] = 3000;
    roc_setup[refvol_page]["INTCTEST"] = 1;
  }
  auto roc_test_lock = tgt->tempApplyAllROCs(roc_setup);

  /**
   * We inject a pulse into Channel 0 of ROC0 on the HcalBackplane
   * which comes out of ROC0 inside TC0_0 which goes into ECON-T1
   * DIN6 which then is summed into STC6 (I think)
   */
  static const int iroc_oi = 0, ch_oi = 0, stc_oi = 6;
  auto roc_inject =
      tgt->roc(iroc_oi).testParameters().add("CH_0", "LOWRANGE", 1).apply();

  tgt->setup_run(1, Target::DaqFormat::ECOND_SW_HEADERS, 1);

  do {
    bool enable_l1a_follow;
    int og_charge_to_l1a;
    tgt->fc().fc_get_setup_calib(og_charge_to_l1a, enable_l1a_follow);

    int charge_to_l1a =
        pftool::readline_int("Calibration to L1A offset?", og_charge_to_l1a);
    last_charge_to_l1a = charge_to_l1a;
    tgt->fc().fc_setup_calib(charge_to_l1a, enable_l1a_follow);

    auto dh_page = tgt->roc(iroc_oi).getParameters("DIGITALHALF_0");
    int og_l1offset = dh_page.at("L1OFFSET");
    int l1offset = pftool::readline_int("L1Offset on HGCROC?", og_l1offset);
    last_l1offset = l1offset;
    auto test_l1offset_handle = tgt->roc(iroc_oi)
                                    .testParameters()
                                    .add("DIGITALHALF_0", "L1OFFSET", l1offset)
                                    .add("DIGITALHALF_1", "L1OFFSET", l1offset)
                                    .apply();

    int og_pipeline, og_samples_per_l1a, og_presamples, econid;
    trig->get_daq_setup(og_pipeline, econid, og_samples_per_l1a, og_presamples);
    int pipeline{og_pipeline}, samples_per_l1a{og_samples_per_l1a},
        presamples{og_presamples};
    pipeline = pftool::readline_int("pipeline: ", pipeline);
    last_pipeline = pipeline;
    samples_per_l1a =
        pftool::readline_int("samples_per_l1a: ", samples_per_l1a);
    presamples = pftool::readline_int("presamples: ", presamples);
    trig->setup_daq(pipeline, econid, samples_per_l1a, presamples);

    pflib_log(info)
        << "pedestal runs to confirm alignment and trigger-sum suppression";
    tgt->fc().sendROR();
    usleep(10000);  // one 100Hz cycle later

    // capture data from this event
    std::vector<uint32_t> trg_pedestal_event = trig->read_event();
    std::vector<uint32_t> pedestal_algo_output_raw = trig->read_algo_output();
    std::vector<uint32_t> daq_pedestal_event = tgt->read_event();

    // decode captured data
    std::vector<SingleECONTCaptureFrame> trg_pedestals =
        decode_multi_sample<SingleECONTCaptureFrame>(trig->get_l1a_per_ror(),
                                                     trg_pedestal_event);

    std::vector<TrigAlgoOutput> pedestal_algo_output =
        decode_multi_sample<TrigAlgoOutput>(trig->get_l1a_per_ror(),
                                            pedestal_algo_output_raw);

    pflib::packing::MultiSampleECONDEventPacket daq_pedestals(2);
    daq_pedestals.from(daq_pedestal_event);

    pflib_log(info) << "charge injection run to see non-zero trigger sums in "
                       "specific places";
    tgt->fc().chargepulse();
    usleep(10000);  // one 100Hz cycle later

    // capture data output, using daq last to advance readout pointer
    std::vector<uint32_t> trg_charge_event = trig->read_event();
    std::vector<uint32_t> charge_algo_output_raw = trig->read_algo_output();
    std::vector<uint32_t> daq_charge_event = tgt->read_event();

    // decode after capturing all data so decoding errors don't cause
    // readout pointer misalignment
    std::vector<SingleECONTCaptureFrame> trg_charge =
        decode_multi_sample<SingleECONTCaptureFrame>(trig->get_l1a_per_ror(),
                                                     trg_charge_event);

    std::vector<TrigAlgoOutput> charge_algo_output =
        decode_multi_sample<TrigAlgoOutput>(trig->get_l1a_per_ror(),
                                            charge_algo_output_raw);

    pflib::packing::MultiSampleECONDEventPacket daq_charge(2);
    daq_charge.from(daq_charge_event);

    pflib_log(debug) << "reset charge_to_l1a back to " << og_charge_to_l1a;
    tgt->fc().fc_setup_calib(og_charge_to_l1a, enable_l1a_follow);

    pflib_log(debug)
        << "reset capture pipeline and n_samples back to original settings";
    pflib_log(debug) << "original pipeline = " << og_pipeline
                     << " samples_per_l1a = " << og_samples_per_l1a
                     << " presamples = " << og_presamples;
    trig->setup_daq(og_pipeline, econid, og_samples_per_l1a, og_presamples);

    pflib_log(info) << "analyze words readout from links";
    pflib_log(info) << "with charge_to_l1a = " << charge_to_l1a
                    << " roc.l1offset = " << l1offset;
    pflib_log(info) << "with pipeline = " << pipeline
                    << " samples_per_l1a = " << samples_per_l1a
                    << " presamples = " << presamples;
    printf("DAQ Data\n");
    printf("     pedestal ->  charge\n");
    printf(" i:  t-1   t  ->  t-1   t \n");
    auto [i_erx, i_ch] = tgt->getRocErxMapping().toErxChannel(iroc_oi, ch_oi);
    for (int i_sample{0}; i_sample < daq_pedestals.samples.size(); i_sample++) {
      printf("%2d: %4d %4d -> %4d %4d%s\n", i_sample,
             daq_pedestals.samples.at(i_sample).channel(i_erx, i_ch).adc_tm1(),
             daq_pedestals.samples.at(i_sample).channel(i_erx, i_ch).adc(),
             daq_charge.samples.at(i_sample).channel(i_erx, i_ch).adc_tm1(),
             daq_charge.samples.at(i_sample).channel(i_erx, i_ch).adc(),
             (i_sample == tgt->daq().soi()) ? " <- sample of interest" : "");
    }

    printf("TRG Data\n");
    printf(" i:  ped -> chrg\n");
    for (int i_l1a{0}; i_l1a < trg_pedestals.size(); i_l1a++) {
      for (int i_sample{0}; i_sample < trg_pedestals[i_l1a].n_samples();
           i_sample++) {
        int pedestal{trg_pedestals[i_l1a].stc_sum(stc_oi, i_sample)},
            charge{trg_charge[i_l1a].stc_sum(stc_oi, i_sample)};
        printf("%2d: %4d -> %4d%s\n", i_l1a + i_sample, pedestal, charge,
               (i_l1a + i_sample == tgt->daq().soi()) ? " <- sample of interest"
                                                      : "");
      }
    }

    printf("ALGO Output\n");
    printf(" i: charge output\n");
    for (int i_l1a{0}; i_l1a < pedestal_algo_output.size(); i_l1a++) {
      for (int i_sample{0}; i_sample < pedestal_algo_output[i_l1a].n_samples();
           i_sample++) {
        std::cout << std::setw(2) << i_l1a + i_sample << ": "
                  << charge_algo_output[i_l1a].sample(i_sample);
        if (i_l1a + i_sample == tgt->daq().soi())
          std::cout << " <- sample of interest";
        printf("\n");
      }
    }
  } while (pftool::readline_bool(
      "Want to try another set of timing parameters?", false));
}

/**
 * TRIG.SELF_TRIG
 */
void self_trigger(Target* tgt) {
  pflib::TRIG* trig = tgt->trig();
  if (trig == 0) return;

  /**
   * Attempts to have trigger algo do the self-triggering by
   * disabling the following L1A after a charge injection
   * and enabling external L1As originating from our trigger
   * algorithm.
   */
  pflib_log(info) << "setting up parameters for self-trigger test";

  std::map<std::string, std::map<std::string, uint64_t>> roc_setup;
  for (int half{0}; half < 2; half++) {
    roc_setup[pflib::utility::string_format("HALFWISE_%d", half)]
             ["ADC_PEDESTAL"] = 255;
    roc_setup[pflib::utility::string_format("DIGITALHALF_%d", half)]["ADC_TH"] =
        31;
    auto refvol_page{
        pflib::utility::string_format("REFERENCEVOLTAGE_%d", half)};
    roc_setup[refvol_page]["CALIB"] = 3000;
    roc_setup[refvol_page]["INTCTEST"] = 1;
  }
  auto roc_test_lock = tgt->tempApplyAllROCs(roc_setup);

  static const int iroc_oi = 0, ch_oi = 0, stc_oi = 6;
  auto roc_inject =
      tgt->roc(iroc_oi).testParameters().add("CH_0", "LOWRANGE", 1).apply();

  tgt->setup_run(1, Target::DaqFormat::ECOND_SW_HEADERS, 1);

  bool enable_l1a_follow;
  int charge_to_l1a;
  tgt->fc().fc_get_setup_calib(charge_to_l1a, enable_l1a_follow);
  pflib_log(info) << "charge_to_l1a = " << charge_to_l1a
                  << " enable_l1a_follow = " << enable_l1a_follow;
  bool l1aen, extl1a;
  tgt->fc().fc_enables_read(l1aen, extl1a);
  pflib_log(info) << "l1a_enabled = " << l1aen << " external_l1a = " << extl1a;
  pflib_log(info) << "self-trigger count: " << trig->get_self_trigger_count();
  pflib_log(info) << "event occupancy: " << tgt->daq().getEventOccupancy();

  pflib_log(info) << "disabling the L1A following the charge command";
  tgt->fc().fc_setup_calib(charge_to_l1a, false);

  pflib_log(info) << "enabling single-shot external L1A mode";
  bool og_single_shot = trig->get_enable_single_shot();
  trig->enable_single_shot(true);
  tgt->fc().fc_enables(true, true);

  pflib_log(info) << "self-trigger count: " << trig->get_self_trigger_count();
  pflib_log(info) << "event occupancy: " << tgt->daq().getEventOccupancy();

  do {
    auto dh_page = tgt->roc(iroc_oi).getParameters("DIGITALHALF_0");
    int og_l1offset = dh_page.at("L1OFFSET");
    int l1offset = pftool::readline_int("L1Offset on HGCROC?", og_l1offset);
    auto test_l1offset_handle = tgt->roc(iroc_oi)
                                    .testParameters()
                                    .add("DIGITALHALF_0", "L1OFFSET", l1offset)
                                    .add("DIGITALHALF_1", "L1OFFSET", l1offset)
                                    .apply();

    int og_pipeline, og_samples_per_l1a, og_presamples, econid;
    trig->get_daq_setup(og_pipeline, econid, og_samples_per_l1a, og_presamples);
    int pipeline{og_pipeline}, samples_per_l1a{og_samples_per_l1a},
        presamples{og_presamples};
    pipeline = pftool::readline_int("pipeline: ", pipeline);
    // samples_per_l1a = pftool::readline_int("samples_per_l1a: ",
    // samples_per_l1a); presamples = pftool::readline_int("presamples: ",
    // presamples);
    trig->setup_daq(pipeline, econid, samples_per_l1a, presamples);

    pflib_log(info) << "self-trigger count: " << trig->get_self_trigger_count();
    pflib_log(info) << "event occupancy: " << tgt->daq().getEventOccupancy();
    pflib_log(info) << "reset single shot and then inject a charge pulse";
    trig->reset_single_shot();
    tgt->fc().chargepulse();
    int i100us{0};
    do {
      usleep(100);
      i100us++;
    } while (not trig->single_shot_fired() and i100us < 10);
    pflib_log(info) << "single shot fired: " << std::boolalpha
                    << trig->single_shot_fired();
    pflib_log(info) << "self-trigger count: " << trig->get_self_trigger_count();
    pflib_log(info) << "event occupancy: " << tgt->daq().getEventOccupancy();

    if (tgt->daq().getEventOccupancy() == 1) {
      // capture data output, using daq last to advance readout pointer
      std::vector<uint32_t> trg_charge_event = trig->read_event();
      std::vector<uint32_t> charge_algo_output_raw = trig->read_algo_output();
      std::vector<uint32_t> daq_charge_event = tgt->read_event();

      // decode after capturing all data so decoding errors don't cause
      // readout pointer misalignment
      std::vector<SingleECONTCaptureFrame> trg_charge =
          decode_multi_sample<SingleECONTCaptureFrame>(trig->get_l1a_per_ror(),
                                                       trg_charge_event);

      std::vector<TrigAlgoOutput> charge_algo_output =
          decode_multi_sample<TrigAlgoOutput>(trig->get_l1a_per_ror(),
                                              charge_algo_output_raw);

      pflib::packing::MultiSampleECONDEventPacket daq_charge(2);
      daq_charge.from(daq_charge_event);

      printf("DAQ Data\n");
      printf(" i:  t-1   t \n");
      auto [i_erx, i_ch] = tgt->getRocErxMapping().toErxChannel(iroc_oi, ch_oi);
      for (int i_sample{0}; i_sample < daq_charge.samples.size(); i_sample++) {
        printf("%2d: %4d %4d\n", i_sample,
               daq_charge.samples.at(i_sample).channel(i_erx, i_ch).adc_tm1(),
               daq_charge.samples.at(i_sample).channel(i_erx, i_ch).adc());
      }

      printf("TRG Data\n");
      printf(" i: stc6\n");
      for (int i_l1a{0}; i_l1a < trg_charge.size(); i_l1a++) {
        for (int i_sample{0}; i_sample < trg_charge[i_l1a].n_samples();
             i_sample++) {
          int charge{trg_charge[i_l1a].stc_sum(stc_oi, i_sample)};
          printf("%2d: %4d\n", i_l1a + i_sample, charge);
        }
      }

      printf("ALGO Output\n");
      printf(" i: charge output\n");
      for (int i_l1a{0}; i_l1a < charge_algo_output.size(); i_l1a++) {
        for (int i_sample{0}; i_sample < charge_algo_output[i_l1a].n_samples();
             i_sample++) {
          std::cout << std::setw(2) << i_l1a + i_sample
                    << ": " << charge_algo_output[i_l1a].sample(i_sample)
                    << std::endl;
        }
      }
    }
    trig->setup_daq(og_pipeline, econid, og_samples_per_l1a, og_presamples);
  } while (pftool::readline_bool(
      "Want to try another set of timing parameters?", false));

  pflib_log(info) << "disabling single-shot external L1A";
  tgt->fc().fc_enables(l1aen, extl1a);
  trig->enable_single_shot(og_single_shot);

  tgt->fc().fc_setup_calib(charge_to_l1a, enable_l1a_follow);
}

/**
 * TRIG.SETUP
 *
 * apply the various time offset parameters that were deduced in TIMEIN
 * and SELF_TRIG
 */
void setup(Target* tgt) {
  pflib::TRIG* trig = tgt->trig();
  if (trig == 0) return;

  int l1offset = pftool::readline_int("L1Offset on HGCROC?", last_l1offset);

  std::map<std::string, std::map<std::string, uint64_t>> roc_settings;
  roc_settings["DIGITALHALF_0"]["L1OFFSET"] = l1offset;
  roc_settings["DIGITALHALF_1"]["L1OFFSET"] = l1offset;
  for (int iroc : tgt->roc_ids()) {
    tgt->roc(iroc).applyParameters(roc_settings);
  }

  int pipeline, samples_per_l1a, presamples, econid;
  trig->get_daq_setup(pipeline, econid, samples_per_l1a, presamples);
  pipeline = pftool::readline_int("trig capture pipeline depth?", last_pipeline);
  trig->setup_daq(pipeline, econid, samples_per_l1a, presamples);

  bool enable_l1a_follow;
  int charge_to_l1a;
  tgt->fc().fc_get_setup_calib(charge_to_l1a, enable_l1a_follow);
  charge_to_l1a =
      pftool::readline_int("Calibration to L1A offset?", last_charge_to_l1a);
  tgt->fc().fc_setup_calib(charge_to_l1a, enable_l1a_follow);
}

#include "pflib/zcu/zcu_trig.h"
void histo(const std::string& cmd, Target* tgt) {
  pflib::TRIG* trig = tgt->trig();
  if (!trig) return;
  auto* ztrig = dynamic_cast<pflib::zcu::ZCUtrig*>(trig);
  if (!ztrig) return;

  if (cmd == "CLEAR") {
    ztrig->clear_histograms();
  }

  if (cmd == "DEBUG") {
    static int code = 0;
    code = pftool::readline_int("debug code:", code, true);
    ztrig->debug_histogram(code);
  }

  if (cmd == "READ") {
    static int ihist = 0;
    ihist = pftool::readline_int("Which histogram?", ihist); 
    std::vector<uint32_t> hist = ztrig->read_histogram(ihist);
    for (std::size_t i{0}; i < hist.size(); i++) {
      printf("%3d %u\n", i, hist[i]);
    }
  }

  if (cmd == "DUMP") {
    printf("bin : %10u %10u %10u %10u %10u %10u %10u %10u\n", 0, 1, 2, 3, 4, 5, 6, 7);
    std::array<std::vector<uint32_t>, 8> hists;
    std::array<unsigned int, 8> total;
    total.fill(0);
    for (int ihist{0}; ihist < hists.size(); ihist++) {
      hists[ihist] = ztrig->read_histogram(ihist);
    }
    for (std::size_t i{0}; i < hists[0].size(); i++) {
      printf("%3d :", i);
      for (int ihist{0}; ihist < hists.size(); ihist++) {
        printf(" %10u", hists[ihist][i]);
        total[ihist] += hists[ihist][i];
      }
      printf("\n");
    }
    printf("tot :");
    for (int ihist{0}; ihist < total.size(); ihist++) {
      printf(" %10u", total[ihist]);
    }
    printf("\n");
  }
}

namespace {
// accessing the TRIGGER path only works on the ZCU
// where we have hardware and firmware access to the TRIGGER stream
auto menu_trig =
    pftool::menu("TRIG", "TRIGGER functionalities", trig_render, ONLY_ZCU)
        ->line("STATUS", "printout trigger settings", trig)
        ->line("PIPELINE", "set the pipeline depth for trigger capture", trig)
        ->line("SAMPLES_PER_L1A", "set number of samples to readout in an L1A",
               trig)
        ->line("PRESAMPLES", "set number of pre-samples in an L1A", trig)
        ->line("ECONID", "set econ id", trig)
        ->line("RESET", "Reset trigger firmware blocks", trig)
        ->line("TIMEIN", "scan delay settings to timein trigger capture",
               trigger_timein)
        ->line("SELF_TRIG", "attempt to trigger on a charge pulse",
               self_trigger)
        ->line(
            "SETUP",
            "apply time offset parameters deduced from TIMEIN and/or SELF_TRIG",
            setup)
        ->line("BUFFER_CLEAR",
               "clear buffer by reading events until none are left", trig)
        ->line("ELINK_SPY", "spy on the six TRIG elinks", trig)
        ->line("EVENT_SPY", "attempt to read the last captured event", trig);

auto menu_expert =
    menu_trig->submenu("EXPERT", "low-level commands for debugging behavior")
        ->line("SW_L1A", "send a single L1A from software",
               [](Target* tgt) { tgt->fc().sendL1A(); })
        ->line("SW_ROR", "send a RoR from software",
               [](Target* tgt) { tgt->fc().sendROR(); })
        ->line("ADV", "advance the readout pointers",
               [](Target* tgt) { tgt->daq().advanceLinkReadPtr(); })
        ->line("BUFFER_CLEAR",
               "clear buffer by reading events until none are left", trig)
        ->line("RESET", "Reset trigger firmware blocks", trig)
        ->line("ELINK_SPY", "spy on the six TRIG elinks", trig)
        ->line("EVENT_SPY", "attempt to read the last captured event", trig);

auto menu_algo =
    menu_trig->submenu("ALGO", "configure and view trigger algorithm")
        ->line("CONFIG", "configure trigger algorithm parameters", algo)
        ->line("SPY", "view output of trigger algorithm", algo)
        ->line("STATUS",
               "printout algorithm settings and output capture status", algo)
        ->line("BUFFER_CLEAR", "clear out buffer of algo output", algo);

auto menu_align =
    menu_trig->submenu("ALIGN", "debug trigger elink alignment")
        ->line("READ", "view alignment capture buffer after a link reset",
               align)
        ->line("DELAY", "link-specific capture delay offset", align)
        ->line("SETUP", "all-link capture delay", align);

auto menu_histo = 
    menu_trig->submenu("HISTO", "view and debug firmware histograms")
        ->line("READ", "read a histogram of an STC", histo)
        ->line("DEBUG", "fill a known value into a test histogram", histo)
        ->line("CLEAR", "clear all of the histograms", histo)
        ->line("DUMP", "print out all histograms at once", histo);
}  // namespace
