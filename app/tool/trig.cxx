/**
 * @file trig.cxx
 * TRIG menu commands
 */
#include "pflib/TRIG.h"
#include "pftool.h"

ENABLE_LOGGING();

void trig_render(Target* tgt) {

}

/**
 * Interaction with Optical links
 *
 * ## Commands
 * - RESET : call reset on the TRIG block
 * - ALIGN_SETUP : setup the alignment capture time
 * - ALIGN_READ : read the alignment capture buffer
 * - ALIGN_DELAY : set the delay for one elink
 * - DAQ_DEBUG : debug the DAQ functionality of the block
 */
void trig(const std::string& cmd, Target* target) {
  static std::string olink_name;
  pflib::TRIG* trig=target->trig();
  if (trig==0) return;

  if (cmd == "RESET") {
    trig->reset();
  }
  if (cmd == "ALIGN_SETUP") {
    int value=pftool::readline_int("Alignment capture delay: ",trig->get_alignment_capture());
    trig->setup_alignment_capture(value);
  }
  if (cmd == "ALIGN_READ") {
    bool do_fc=pftool::readline_bool("Generate LINKRESET_ECONT?",true);
    if (do_fc) target->fc().linkreset_econs();
    for (int ilink=0; ilink<trig->n_elinks(); ilink++) {      
      std::vector<uint32_t> val=trig->read_capture_buffer(ilink);
      printf("%02d : ",ilink);
      for (auto x : val)
        printf(" %08x",x);
      printf("\n");
    }
  }
  if (cmd == "ALIGN_DELAY") {
    int ilink=pftool::readline_int("Which elink?",0);
    trig->set_bx_delay(ilink,pftool::readline_int("New delay: ",trig->get_bx_delay(ilink)));
  }
}

namespace {
auto trigm =
    pftool::menu("TRIG", "TRIGGER functionalities")
                 ->line("RESET",
                        "Reset trigger firmware blocks", trig)
                 ->line("ALIGN_SETUP", "Setup the alignment delay",trig)
                 ->line("ALIGN_READ", "Capture and read the alignment windows",trig)
                 ->line("ALIGN_DELAY", "Setup the word delay for an elink",trig)
                 ->line("DAQ_DEBUG", "DAQ debugging function",trig)
                 ;
}
