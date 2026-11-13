/**
 * @file align.cxx
 */
#include "align.h"

#include "pflib/TRIG.h"
#include "pflib/packing/ECONTAlignmentWord.h"

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
  if (cmd == "RESET") {
    trig->reset_alignment();
  }
  if (cmd == "SETUP") {
    int delay;
    uint16_t pattern;
    uint8_t elinks_to_ignore;
    bool bypass_pattern;
    trig->get_alignment_setup(delay, pattern, elinks_to_ignore, bypass_pattern);
    delay = pftool::readline_int("Alignment capture delay: ", delay);
    pattern =
        pftool::readline_int("11bit Pattern to search for: ", pattern, true);
    elinks_to_ignore = pftool::readline_int(
        "Bit mask of elinks to ignore: ", elinks_to_ignore, true);
    bypass_pattern =
        pftool::readline_bool("Bypass Pattern for capture?", bypass_pattern);
    trig->setup_alignment(delay, pattern, elinks_to_ignore, bypass_pattern);
  }
  if (cmd == "READ") {
    bool show_raw = pftool::readline_bool(
        "Show raw data [Y] or idle word interpretation [N]? ", false);
    tgt->fc().linkreset_econs();
    usleep(3000);
    for (int ilink = 0; ilink < trig->n_elinks(); ilink++) {
      std::vector<uint32_t> val = trig->read_capture_buffer(ilink);
      printf("%02d :", ilink);
      for (auto x : val) {
        if (show_raw) {
          printf(" %08x", x);
        } else {
          pflib::packing::ECONTAlignmentWord w(x);
          std::cout << " | " << w;
        }
      }
      printf("\n");
    }
  }
}
