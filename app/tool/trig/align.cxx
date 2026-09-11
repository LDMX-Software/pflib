/**
 * @file align.cxx
 */
#include "align.h"

#include "pflib/TRIG.h"

void align(const std::string& cmd, Target* tgt) {
  /**
   * TRIG.ALIGN commands
   *
   * - SETUP : setup the alignment capture time
   * - READ : read the alignment capture buffer
   * - DELAY : set the delay for one elink
   */
  pflib::TRIG* trig = tgt->trig();
  if (trig == 0) return;
  if (cmd == "SETUP") {
    int delay;
    uint16_t pattern;
    bool bypass_pattern;
    trig->get_alignment_setup(delay, pattern, bypass_pattern);
    delay = pftool::readline_int("Alignment capture delay: ", delay);
    pattern = pftool::readline_int("11bit Pattern to search for: ", pattern, true);
    bypass_pattern = pftool::readline_bool("Bypass Pattern for capture?", bypass_pattern);
    trig->setup_alignment(delay, pattern, bypass_pattern);
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
}
