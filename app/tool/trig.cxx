/**
 * @file trig.cxx
 * TRIG menu commands
 */
#include "pflib/TRIG.h"
#include "pflib/OptoLink.h"

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
 * - DAQ_DEBUG : debug the DAQ functionality of the block
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
  if (cmd == "PIPELINE") {
    int pipeline{-1}, econ_id{-1}, samples_per_l1a{-1}, presamples{-1};
    trig->get_daq_setup(pipeline, econ_id, samples_per_l1a, presamples);
    pipeline = pftool::readline_int("pipeline depth: ", pipeline);
    trig->setup_daq(pipeline, econ_id, samples_per_l1a, presamples);
  }
  if (cmd == "ALIGN_SETUP") {
    int value = pftool::readline_int("Alignment capture delay: ",
                                     trig->get_alignment_capture());
    trig->setup_alignment_capture(value);
  }
  if (cmd == "ALIGN_READ") {
    pflib::lpGBT lpgbt{target->get_opto_link("TRG").lpgbt_transport()};
    // each output group {0..3} has 2 bits shifted by group*2
    // 0 -> link data
    // 1 -> prbs
    // 2 -> bin_cntr_up
    // 3 -> const pattern (copy 32b from DPDATAPATTERN)
    static const uint16_t ULDATASOURCE1 = 0x129;
    static const uint16_t DPDATAPATTERN[4] = {0x131, 0x130, 0x12f, 0x12e};
    // copy a recognizable pattern into output link
    bool using_const_pattern = pftool::readline_bool("Have lpGBT output a const pattern? ", true);
    if (using_const_pattern) {
      uint32_t known_pattern = 0x12345678;
      printf("const pattern: 0x%08x\n", known_pattern);
      for (int i_byte{0}; i_byte < 4; i_byte++) {
        lpgbt.write(DPDATAPATTERN[i_byte], ((known_pattern >> (i_byte*8)) & 0xff));
      }
      int link = pftool::readline_int("which link (-1 for all)? ", 0);
      auto output_const_pattern = [&](int ilink) {
        int grp = ilink;
        // on lpGBT mezzanine, links 3, 4, 5 are connected to eTx groups 4, 5, 6
        if (grp > 2) grp++;
        uint16_t datasource_config_reg = ULDATASOURCE1 + (grp/2);
        uint8_t datasource = lpgbt.read(datasource_config_reg);
        datasource |= (4 << ((grp % 2)*3)); // 4 == use const pattern
        lpgbt.write(datasource_config_reg, datasource);
        printf("apply data source config 0x%02x on reg 0x%03x\n", datasource, datasource_config_reg);
      };
      if (link < 0) {
        for (int ilink{0}; ilink < 6; ilink++) {
          output_const_pattern(ilink);
        }
      } else if (link > 5) {
        pflib_log(error) << "invalid link index";
        return;
      } else {
        output_const_pattern(link);
      }
    }

    if (pftool::readline_bool("do elink spy rather than align capture?", true)) {
      pflib::Elinks& elinks = target->elinks();
      std::vector<std::vector<uint32_t>> spy(6);
      printf("word :");
      for (int ilink{0}; ilink < 6; ilink++) {
        // trial and error revealed, 6-11 as the elink indices
        spy[ilink] = elinks.spy(6+ilink); 
        printf("   Link %d", ilink);
      }
      printf("\n");
      for (int iword{0}; iword < 64; iword++) {
        printf("%4d :", iword);
        for (int ilink{0}; ilink < 6; ilink++) {
          printf(" %08x", spy[ilink][iword]);
        }
        printf("\n");
      }
    } else {
      bool do_fc = pftool::readline_bool("Generate LINKRESET_ECONT?", true);
      bool show_raw = pftool::readline_bool("Show raw data [Y] or idle word interpretation [N]? ", using_const_pattern);
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
            printf(" | %02d %03x %02d %03x",
                    (x >> (16+11)) & 0x1f,
                    (x >> 16) & 0x7ff,
                    (x >> 11) & 0x1f,
                    (x & 0x7ff)
                  );
          }
        }
        printf("\n");
      }
    }
    // reset data source to all zeros for all groups (normal operation)
    for (int i{0}; i < 4; i++) {
      lpgbt.write(ULDATASOURCE1+i, 0x00);
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
        ->line("RESET", "Reset trigger firmware blocks", trig)
        ->line("ALIGN_SETUP", "Setup the alignment delay", trig)
        ->line("ALIGN_READ", "Capture and read the alignment windows", trig)
        ->line("ALIGN_DELAY", "Setup the word delay for an elink", trig)
        ->line("DAQ_DEBUG", "DAQ debugging function", trig);
}  // namespace
