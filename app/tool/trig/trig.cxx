/**
 * @file trig.cxx
 * TRIG menu commands
 */
#include "pflib/TRIG.h"

#include "algo.h"
#include "align.h"
#include "decode_multi_sample.h"
#include "histo.h"
#include "pflib/packing/DecompressAEBM.h"
#include "pflib/packing/SingleECONTCaptureFrame.h"
#include "pflib/zcu/zcu_trig.h"
#include "self_trig.h"
#include "timein.h"
#include "watch_run.h"
using pflib::packing::SingleECONTCaptureFrame;

#include <optional>

#include "../pftool.h"

ENABLE_LOGGING();

void trig_render(Target* tgt) {}

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
    printf(" %20s: %d\n", "pipeline", pipeline);
    printf(" %20s: %d\n", "econ_id", econ_id);
    printf(" %20s: %d\n", "samples_per_l1a", samples_per_l1a);
    printf(" %20s: %d\n", "presamples", presamples);
    printf(" %20s: %d\n", "capture delay", trig->get_alignment_capture());
    for (int ilink{0}; ilink < trig->n_elinks(); ilink++) {
      printf("           %d bx delay: %d\n", ilink, trig->get_bx_delay(ilink));
    }
    printf("status\n");
    printf(" %20s: %d\n", "DAQ event occupancy",
           target->daq().getEventOccupancy());
    for (const auto& [name, val] : trig->get_debug()) {
      printf(" %20s: %d\n", name.c_str(), val);
    }
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
}

/**
 * ZCU trigger-specific stuff
 */
void ztrig(const std::string& cmd, Target* tgt) {
  pflib::zcu::ZCUtrig* trig = dynamic_cast<pflib::zcu::ZCUtrig*>(tgt->trig());
  if (!trig) return;

  static uint32_t decoder_lut_addr = 0;
  if (cmd == "DECODER_LUT_READ") {
    decoder_lut_addr =
        pftool::readline_int("Encoded Value to Read: ", decoder_lut_addr, true);
    uint32_t val = trig->get_decoder_lut(decoder_lut_addr);
    printf("0x%03x : %d\n", decoder_lut_addr, val);
  }

  if (cmd == "DECODER_LUT_WRITE") {
    decoder_lut_addr = pftool::readline_int(
        "Encoded Value to Write for: ", decoder_lut_addr, true);
    uint32_t val = pftool::readline_int(
        "Value to write: ", trig->get_decoder_lut(decoder_lut_addr));
    trig->set_decoder_lut(decoder_lut_addr, val);
  }

  if (cmd == "DECODER_LUT_INIT") {
    int divisor =
        pftool::readline_int("Scale to divide decoded values by: ", 1);
    bool show_lut =
        pftool::readline_bool("Show LUT that is being written? ", true);
    for (int encoded_val{0}; encoded_val < (1 << 9); encoded_val++) {
      unsigned long full_decoded_val =
          pflib::packing::decompressAEBM<5, 4>(encoded_val) / divisor;
      // the LUT only has an output width of 16 bits, so we saturate
      // if the full decoded val (after dividing out the scale) would
      // be above this limit
      uint32_t lut_decoded_val = 0xffff;
      if (full_decoded_val < (1 << 16)) {
        lut_decoded_val = static_cast<uint32_t>(full_decoded_val);
      }
      if (show_lut) printf("0x%03x : %d\n", encoded_val, lut_decoded_val);
      trig->set_decoder_lut(encoded_val, lut_decoded_val);
    }
  }
}

/**
 * TRIG.SETUP
 *
 * apply the various time offset parameters that were deduced in TIMEIN
 * and SELF_TRIG
 */
void setup(Target* tgt) {
  TimeInSettings::last.init(tgt);
  TimeInSettings::last.l1offset = pftool::readline_int(
      "L1Offset on HGCROC?", TimeInSettings::last.l1offset);
  TimeInSettings::last.pipeline = pftool::readline_int(
      "trig capture pipeline depth?", TimeInSettings::last.pipeline);
  TimeInSettings::last.charge_to_l1a = pftool::readline_int(
      "Calibration to L1A offset?", TimeInSettings::last.charge_to_l1a);
  TimeInSettings::last.apply(tgt);
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
               timein)
        ->line("SELF_TRIG", "attempt to trigger on a charge pulse", self_trig)
        ->line(
            "SETUP",
            "apply time offset parameters deduced from TIMEIN and/or SELF_TRIG",
            setup)
        ->line("WATCH_RUN", "collect data following self-trigger", watch_run)
        ->line("ELINK_SPY", "spy on the six TRIG elinks", trig)
        ->line("EVENT_SPY", "attempt to read the last captured event", trig)
        ->line("DECODER_LUT_READ", "read a value from the decoding LUT", ztrig)
        ->line("DECODER_LUT_WRITE", "write a value from the decoding LUT",
               ztrig)
        ->line("DECODER_LUT_INIT", "initialize the entire decoding LUT", ztrig);

auto menu_expert =
    menu_trig->submenu("EXPERT", "low-level commands for debugging behavior")
        ->line("SW_L1A", "send a single L1A from software",
               [](Target* tgt) { tgt->fc().sendL1A(); })
        ->line("SW_ROR", "send a RoR from software",
               [](Target* tgt) { tgt->fc().sendROR(); })
        ->line("ADV", "advance the readout pointers",
               [](Target* tgt) { tgt->daq().advanceLinkReadPtr(); })
        ->line("RESET", "Reset trigger firmware blocks", trig)
        ->line("ELINK_SPY", "spy on the six TRIG elinks", trig)
        ->line("EVENT_SPY", "attempt to read the last captured event", trig);

auto menu_algo =
    menu_trig->submenu("ALGO", "configure and view trigger algorithm")
        ->line("CONFIG", "configure trigger algorithm parameters", algo)
        ->line("SPY", "view output of trigger algorithm", algo)
        ->line("STATUS",
               "printout algorithm settings and output capture status", algo);

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
