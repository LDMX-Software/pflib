/**
 * @file trig.cxx
 * TRIG menu commands
 */
#include "pflib/TRIG.h"

#include "pftool.h"

ENABLE_LOGGING();

void trig_render(Target* tgt) {}

/**
 * Interaction with Optical links
 *
 * ## Commands
 * - RESET : call reset on the TRIG block
 * - ALIGN_SETUP : setup the alignment capture time
 * - ALIGN_READ : read the alignment capture buffer
 * - ALIGN_DELAY : set the delay for one elink
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
    std::cout << "pipeline: " << pipeline << "\n"
              << "econ_id : " << econ_id << "\n"
              << "samples_per_l1a: " << samples_per_l1a << "\n"
              << "presamples: " << presamples << "\n"
              << "capture delay: " << trig->get_alignment_capture() << "\n"
              << std::flush;
    for (int ilink{0}; ilink < trig->n_elinks(); ilink++) {
      std::cout << "link " << ilink
                << " bx delay: " << trig->get_bx_delay(ilink) << "\n";
    }
    std::cout << std::flush;
  }
  if (cmd == "PIPELINE" or cmd == "SAMPLES_PER_L1A" or cmd == "PRESAMPLES") {
    int pipeline{-1}, econ_id{-1}, samples_per_l1a{-1}, presamples{-1};
    trig->get_daq_setup(pipeline, econ_id, samples_per_l1a, presamples);
    if (cmd == "PIPELINE") {
      pipeline = pftool::readline_int("pipeline depth: ", pipeline);
    } else if (cmd == "SAMPLES_PER_L1A") {
      samples_per_l1a = pftool::readline_int("number of samples per L1A: ", samples_per_l1a);
    } else if (cmd == "PRESAMPLES") {
      presamples = pftool::readline_int("number of pre-samples: ", presamples);
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
    }
    for (uint32_t word : event) {
      printf("%08x\n", word);
    }
  }
  if (cmd == "ALIGN_SETUP") {
    int value = pftool::readline_int("Alignment capture delay: ",
                                     trig->get_alignment_capture());
    trig->setup_alignment_capture(value);
  }
  if (cmd == "ALIGN_READ") {
    bool do_fc = pftool::readline_bool("Generate LINKRESET_ECONT?", true);
    bool show_raw = pftool::readline_bool(
        "Show raw data [Y] or idle word interpretation [N]? ", false);
    if (do_fc) target->fc().linkreset_econs();
    usleep(2000);
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
  if (cmd == "ALIGN_DELAY") {
    int ilink = pftool::readline_int("Which elink?", 0);
    trig->set_bx_delay(
        ilink, pftool::readline_int("New delay: ", trig->get_bx_delay(ilink)));
  }
}

namespace {
// accessing the TRIGGER path only works on the ZCU
// where we have hardware and firmware access to the TRIGGER stream
auto menu_trig =
    pftool::menu("TRIG", "TRIGGER functionalities", trig_render, ONLY_ZCU)
        ->line("STATUS", "printout trigger settings", trig)
        ->line("PIPELINE", "set the pipeline depth for trigger capture", trig)
        ->line("SAMPLES_PER_L1A", "set number of samples to readout in an L1A", trig)
        ->line("PRESAMPLES", "set number of pre-samples in an L1A", trig)
        ->line("RESET", "Reset trigger firmware blocks", trig)
        ->line("ALIGN_SETUP", "Setup the alignment delay", trig)
        ->line("ALIGN_READ", "Capture and read the alignment windows", trig)
        ->line("ALIGN_DELAY", "Setup the word delay for an elink", trig)
        ->line("SW_L1A", "send a L1A from software",
               [](Target* tgt) { tgt->fc().sendL1A(); })
        ->line("ADV", "advance the readout pointers",
               [](Target* tgt) { tgt->daq().advanceLinkReadPtr(); })
        ->line("ELINK_SPY", "spy on the six TRIG elinks", trig)
        ->line("EVENT_SPY", "attempt to read the last captured event", trig);
}  // namespace
