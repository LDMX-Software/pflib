#include "align_econ_lpgbt.h"

#include <algorithm>

#include "pflib/OptoLink.h"
#include "pflib/TRIG.h"
#include "pflib/utility/string_format.h"

ENABLE_LOGGING();

static void print_locked_status(pflib::lpGBT& lpgbt) {
  constexpr uint16_t REG_EPRX0LOCKED = 0x152;

  for (int ierx{0}; ierx < 6; ierx++) {
    int grp = ierx;
    if (grp > 2) grp++;
    // get EPRXnLocked and EPRXnCurrentPhase10 where n is grp
    std::vector<uint8_t> read_result = lpgbt.read(REG_EPRX0LOCKED + 3 * grp, 2);
    uint8_t locked_status = read_result.at(0);
    uint8_t current_phase10 = read_result.at(1);

    // lpGBT mezzanine directs inputs into channel 0 of their group
    bool ch0_locked = ((locked_status >> 4) & 0x1);
    uint8_t ch0_phase = (current_phase10 & 0xf);
    uint8_t state = (locked_status & 0x3);
    const char* state_name{"???"};
    switch (state) {
      case 0:
        state_name = "Reset";
        break;
      case 1:
        state_name = "Force Down";
        break;
      case 2:
        state_name = "Confirm early state";
        break;
      case 3:
        state_name = "Free running state";
        break;
      default:
        state_name = "Unknown";
        break;
    }

    printf(" Group %d\n", grp);
    printf("  state: %s (%d)\n", state_name, state);
    printf("  ch 0: %s\n", (ch0_locked ? "LOCKED" : "UNLOCKED"));
    printf("  ch 0 phase: %u\n", ch0_phase);
  }
}

static void align_econ_lpgbt_bit(Target* tgt, pflib::ECON& econ, int iecon,
                                 bool check_all_phases) {
  // ----- bit alignment with PRBS7 as input -----
  // assumes the OptoLinks are named "DAQ" and "TRG" like in HcalBackplaneZCU,
  // EcalSMMTargetZCU, HcalBackplaneBW, and EcalSMMTargetBW
  bool is_econd = (econ.type() == "econd");

  pflib::lpGBT lpgbt{(is_econd)
                         ? (tgt->get_opto_link("DAQ").lpgbt_transport())
                         : (tgt->get_opto_link("TRG").lpgbt_transport())};

  printf("\n --- PRE-PRBS STATUS ---\n");
  print_locked_status(lpgbt);
  uint32_t prbs_state;

  if (is_econd) {
    printf(" NOTE: Only checking Group 0, Channel 0\n");
    prbs_state = econ.readParameter("FORMATTERBUFFER", "GLOBAL_PRBS_ON");
    printf(" ECON PRBS State: %u\n", prbs_state);

    bool default_invert = (pftool::state.readout_config_is_hcal());

    bool do_invert =
        pftool::readline_bool("Invert elink data?", default_invert);
    uint8_t invert = (do_invert) ? (1) : (0);
    std::map<std::string, std::map<std::string, uint64_t>> parameters = {};

    parameters["CLOCKSANDRESETS"]["GLOBAL_PUSM_RUN"] = 0;
    econ.applyParameters(parameters);
    usleep(10000);

    parameters.clear();
    parameters["CLOCKSANDRESETS"]["GLOBAL_PUSM_RUN"] = 1;
    econ.applyParameters(parameters);
    usleep(10000);

    auto invert_state = econ.readParameter("ETX", "0_INVERT_DATA");
    printf(" ECOND data invert state: %lu\n", invert_state);
  }
  if (is_econd)
    econ.applyParameter("FORMATTERBUFFER", "GLOBAL_PRBS_ON", 1);
  else
    econ.applyParameter("FORMATTERBUFFER", "GLOBAL_ETX_PATTERN", 1);

  if (is_econd)
    prbs_state = econ.readParameter("FORMATTERBUFFER", "GLOBAL_PRBS_ON");
  else
    prbs_state = econ.readParameter("FORMATTERBUFFER", "GLOBAL_ETX_PATTERN");

  printf(" ECON PRBS State: %u\n", prbs_state);

  if (is_econd) {
    // ECON-D only has one output link through lpGBT
    printf("Checking ECON-D -> DAQ lpGBT eRx 0...\n");
    lpgbt.check_prbs_errors_erx(0, check_all_phases);
  } else if (pftool::state.readout_config_is_hcal()) {
    // ECON-T has multiple output links through lpGBT
    // connected to channel 0 of a series of groups
    std::vector<std::vector<int>> i_econ_to_group = {
        {0},        // ECON-D
        {0, 1, 2},  // ECON-T1
        {3, 4, 5}   // ECON-T2
    };
    for (int ierx : i_econ_to_group.at(iecon)) {
      printf("Checking ECON-T -> TRG lpGBT eRx %d...\n", ierx);
      lpgbt.check_prbs_errors_erx(ierx, check_all_phases);
    }
  } else {
    // ECON-T with Ecal system
    pflib_log(warn) << "unsure on which links to check for EcalSMM";
    for (int ierx{0}; ierx < 6; ierx++) {
      printf("Checking ECON-T -> TRG lpGBT eRx %d...\n", ierx);
      lpgbt.check_prbs_errors_erx(ierx, check_all_phases);
    }
  }

  printf("\n --- POST-PRBS STATUS ---\n");
  print_locked_status(lpgbt);

  if (is_econd)
    econ.applyParameter("FORMATTERBUFFER", "GLOBAL_PRBS_ON", 0);
  else
    econ.applyParameter("FORMATTERBUFFER", "GLOBAL_ETX_PATTERN", 0);
}

static void align_econ_lpgbt_word(Target* tgt, pflib::ECON& econ,
                                  bool check_all_phases) {
  if (econ.type() == "econd") {
    // word-alignment
    uint32_t idle = pftool::readline_int("Idle pattern", 0x1277CC, true);

    bool found_alignment = false;

    std::vector<uint32_t> got;
    for (int phase = 0; phase < 32; phase++) {
      econ.applyParameter("FormatterBuffer", "Global_align_serializer_0",
                          phase);
      usleep(1000);
      std::vector<uint32_t> spy = tgt->elinks().spy(0, true);
      got.push_back(spy[0]);
      uint32_t obs = spy[0] >> 8;
      if (obs == idle) {
        printf(" Found alignment at %d\n", phase);
        found_alignment = true;
        if (not check_all_phases) {
          break;
        }
      }
    }
    if (!found_alignment) {
      for (int phase = 0; phase < 32; phase++) {
        printf(" %2d 0x%08x\n", phase, got[phase]);
      }
      printf(" WARNING: Did not find alignment\n");
    }
  }
  if (econ.type() == "econt") {
    using pflib::utility::string_format;
    // word-alignment
    uint32_t idle = pftool::readline_int("Idle pattern", 0x526, true);
    static uint32_t ALIGN_MASK = 0x7FF;
    pflib::TRIG* trig = tgt->trig(0);

    static const int ICAPTURE_DELAY = 30;
    trig->setup_alignment_capture(ICAPTURE_DELAY);

    bool all_succeed = true;
    for (int ilink = 0; ilink < trig->n_elinks(); ilink++) {
      std::string reg_name = string_format("GLOBAL_ALIGN_SERIALIZER_%d", ilink);
      int got_idle_phase = -1;
      for (int phase = 0; phase < 16; phase++) {
        std::vector<uint16_t> readings;
        econ.applyParameter("FORMATTERBUFFER", reg_name, phase);
        usleep(2000);
        tgt->fc().linkreset_econs();
        usleep(2000);
        std::vector<uint32_t> samples = trig->read_capture_buffer(ilink);
        for (size_t i = 4; i < 8; i++) {
          readings.push_back((samples[i] >> 16) & ALIGN_MASK);
          readings.push_back(samples[i] & ALIGN_MASK);
        }
        if (std::count(readings.begin(), readings.end(), idle) ==
            readings.size()) {
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
        econ.applyParameter("FORMATTERBUFFER", reg_name, got_idle_phase);
      }
    }

    if (all_succeed) {
      pflib_log(info) << "checking if aligned links are in time";
      bool different_first_bx = false;
      int first_bx = -1;
      for (int ilink{0}; ilink < trig->n_elinks(); ilink++) {
        tgt->fc().linkreset_econs();
        usleep(2000);
        std::vector<uint32_t> samples = trig->read_capture_buffer(ilink);
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

void align_econ_lpgbt(Target* tgt) {
  int iecon =
      pftool::readline_int("Which ECON to manage: ", pftool::state.iecon);

  bool check_all_phases = pftool::readline_bool("Check all phases? ", false);

  pflib::ECON& econ = tgt->econ(iecon);

  if (pftool::readline_bool("Do bit alignment?", true)) {
    align_econ_lpgbt_bit(tgt, econ, iecon, check_all_phases);
  }
  if (pftool::readline_bool("Continue to word alignment?", true)) {
    align_econ_lpgbt_word(tgt, econ, check_all_phases);
  }
}
