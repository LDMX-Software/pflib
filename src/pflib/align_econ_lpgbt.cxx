#include "pflib/align_econ_lpgbt.h"

#include <algorithm>

#include "pflib/Target.h"
#include "pflib/OptoLink.h"
#include "pflib/TRIG.h"
#include "pflib/packing/ECONTAlignmentWord.h"
#include "pflib/utility/string_format.h"

namespace pflib {

static auto the_log_{pflib::logging::get("align_econ_lpgbt")};

/**
 * The HcalBackplane has two ECON-Ts connected to the TRIG lpGBT
 * and each ECON-T has multiple output links through lpGBT
 * connected to channel 0 of a series of groups
 */
std::vector<std::vector<int>> HCAL_BACKPLANE_I_ECON_TO_GROUP = {
    {0},        // ECON-D
    {0, 1, 2},  // ECON-T1
    {3, 4, 5}   // ECON-T2
};

/**
 * The EcalSMM has one ECON-D connected to the DAQ lpGBT
 * and one ECON-T connected to the first six inputs to the
 * TRIG lpGBT
 */
std::vector<std::vector<int>> ECAL_SMM_I_ECON_TO_GROUP = {
  {0},               // ECON-D
  {0, 1, 2, 3, 4, 5} // ECON-T
};

void align_econ_lpgbt_bit(Target* tgt, int iecon,
                          bool check_all_phases, bool is_hcal) {
  // ----- bit alignment with PRBS7 as input -----
  // assumes the OptoLinks are named "DAQ" and "TRG" like in HcalBackplaneZCU,
  // EcalSMMTargetZCU, HcalBackplaneBW, and EcalSMMTargetBW
  pflib::ECON& econ = tgt->econ(iecon);
  bool is_econd = (econ.type() == "econd");
  std::string prbs_parameter = (is_econd ? "GLOBAL_PRBS_ON" : "GLOBAL_ETX_PATTERN");

  pflib::lpGBT lpgbt{(is_econd)
                         ? (tgt->get_opto_link("DAQ").lpgbt_transport())
                         : (tgt->get_opto_link("TRG").lpgbt_transport())};

  econ.applyParameter("FORMATTERBUFFER", prbs_parameter, 1);

  const auto& i_econ_to_group{is_hcal ? HCAL_BACKPLANE_I_ECON_TO_GROUP : ECAL_SMM_I_ECON_TO_GROUP};
  for (int ierx : i_econ_to_group.at(iecon)) {
    pflib_log(info) << "checking ECON -> lpGBT eRx " << ierx << "...";
    lpgbt.check_prbs_errors_erx(ierx, check_all_phases);

    pflib::lpGBT::eRxStatus status = lpgbt.get_erx_status(ierx);
    pflib_log(info) << "locked: " << status.locked() << ", phase: " << status.phase();
  }

  econ.applyParameter("FORMATTERBUFFER", prbs_parameter, 0);
}

void align_econ_lpgbt_word(Target* tgt, int iecon,
                           bool check_all_phases, uint32_t idle_pattern,
                           bool is_hcal) {
  /**
   * This function is where we deduce the value for
   * FormatterBuffer.Global.align_serializer_N
   * where N is the output link index from the ECON.
   *
   * Technically (for both ECONs) this value is 6 bits and therefore can
   * go up to 64. We only check up to 32 here because we can shift by
   * increments of 32 using our capture firmware.
   */
  pflib::ECON& econ = tgt->econ(iecon);
  if (econ.type() == "econd") {
    bool found_alignment = false;

    std::vector<uint32_t> got;
    for (int phase = 0; phase < 32; phase++) {
      econ.applyParameter("FormatterBuffer", "Global_align_serializer_0",
                          phase);
      usleep(1000);
      std::vector<uint32_t> spy = tgt->elinks().spy(0, true);
      got.push_back(spy[0]);
      uint32_t obs = spy[0] >> 8;
      if (obs == idle_pattern) {
        pflib_log(info) << "found alignment at " << phase;
        found_alignment = true;
        if (not check_all_phases) {
          break;
        }
      }
    }
    if (!found_alignment) {
      pflib_log(warn) << "did not find alignment, recieved the following words at each phase.";
      for (int phase = 0; phase < 32; phase++) {
        printf(" %2d 0x%08x\n", phase, got[phase]);
      }
    }
  }
  if (econ.type() == "econt") {
    using pflib::utility::string_format;
    // word-alignment
    static uint32_t ALIGN_MASK = 0x7FF;
    pflib::TRIG* trig = tgt->trig(0);
    if (not trig) {
      PFEXCEPTION_RAISE("BadConf",
          "TRIG object is not available, meaning we cannot do word alignment between the ECON-T and the TRIG lpGBT within pflib alone.");
    }

    int delay;
    uint16_t pattern;
    uint8_t elinks_to_ignore;
    bool bypass_pattern;
    trig->get_alignment_setup(delay, pattern, elinks_to_ignore, bypass_pattern);
    int new_delay = delay;
    if (delay == 0) {
      // capture delay of 0 will /not/ work and is the default
      // of a newly-loaded firmware, update to a reasonable default
      // 30 was from when the backplane was on the same table as the ZCU
      // we also have used 42 after moving the backplane to the dark room
      // and using much longer optical fibers
      static const int DEFAULT_CAPTURE_DELAY = 30;
      new_delay = DEFAULT_CAPTURE_DELAY;
    }

    // we want to bypass pattern matching during alignment
    trig->setup_alignment(new_delay, pattern, elinks_to_ignore, bypass_pattern);

    /**
     * we are assuming that the active elinks in the TRIG match
     * the eRx labeling for the different ECON connections.
     *
     * We are also assuming that, in the Hcal backplane case,
     * ECON-T1 eTx 0, 1, 2 -> TRIG lpGBT eRx 0, 1, 2
     * ECON-T2 eTx 0, 1, 2 -> TRIG lpGBT eRx 3, 4, 5
     */
    const auto& i_econ_to_group{is_hcal ? HCAL_BACKPLANE_I_ECON_TO_GROUP : ECAL_SMM_I_ECON_TO_GROUP};
    std::vector<int> i_elinks = i_econ_to_group.at(iecon);
    bool all_succeed = true;
    for (int ilink{0}; ilink < i_elinks.size(); ilink++) {
      std::string reg_name = string_format("GLOBAL_ALIGN_SERIALIZER_%d", ilink);
      int got_idle_phase = -1;
      for (int phase = 0; phase < 32; phase++) {
        std::vector<uint16_t> readings;
        econ.applyParameter("FORMATTERBUFFER", reg_name, phase);
        usleep(3000);
        tgt->fc().linkreset_econs();
        usleep(3000);
        std::vector<pflib::packing::ECONTAlignmentWord> samples;
        for (const auto& w : trig->read_capture_buffer(i_elinks.at(ilink))) {
          samples.emplace_back(w);
        }
        // only checking first sample because read_capture_buffer
        // is hardcoded for 1 sample to reflect current firmware
        pflib_log(info) << "ilink=" << ilink << " phase=" << phase
                        << " sample={" << samples[0] << "}";
        if (samples[0].match(idle_pattern)) {
          // all of the readings match the configured idle, success!
          got_idle_phase = phase;
          if (not check_all_phases) {
            break;
          }
        }
      }
      if (got_idle_phase < 0) {
        all_succeed = false;
        pflib_log(warn) << "unable to find word alignment for link " << ilink
                        << " from ECON-T";
      } else {
        pflib_log(info) << "found pattern on phase " << got_idle_phase
                        << " for link " << ilink;
        econ.applyParameter("FORMATTERBUFFER", reg_name, got_idle_phase);
      }
    }

    if (all_succeed) {
      pflib_log(info) << "checking if aligned links are in time";
      bool different_first_bx = false;
      int first_bx = -1;
      for (int ilink{0}; ilink < i_elinks.size(); ilink++) {
        tgt->fc().linkreset_econs();
        usleep(3000);
        std::vector<uint32_t> samples =
            trig->read_capture_buffer(i_elinks.at(ilink));
        int last_bx = -1;
        for (int i_sample{0}; i_sample < samples.size(); i_sample++) {
          int bx = ((samples[i_sample] >> (16 + 11)) & 0x1f);
          // should be two equal bx
          if (bx != ((samples[i_sample] >> 11) & 0x1f)) {
            pflib_log(warn) << "unequal BX in same sample for link " << ilink;
          }
          if (i_sample == 0) {
            if (ilink == 0) {
              first_bx = bx;
            } else if (bx != first_bx) {
              different_first_bx = true;
              pflib_log(warn)
                  << "different BX between link " << ilink << " and link 0";
            }
          } else if (bx != ((last_bx + 1) % 16) and bx != 31) {
            pflib_log(warn)
                << "BX not incrementing by one within link " << ilink;
          }
          last_bx = bx % 16;
        }
      }
      if (not different_first_bx) {
        pflib_log(info) << "separate links are in time";
      }
    } else {
      pflib_log(warn) << "unable to check if the separate links are in time";
    }
  }
}

}
