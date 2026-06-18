#include "align_econ_lpgbt.h"

#include "pflib/OptoLink.h"
#include "pflib/TRIG.h"
#include "pflib/utility/string_format.h"

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

static void align_econ_lpgbt_bit(Target* tgt, pflib::ECON& econ, int iecon) {
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
    printf(" ECON PRBS State: %lu\n", prbs_state);

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

  printf(" ECON PRBS State: %lu\n", prbs_state);

  if (is_econd) {
    // ECON-D only has one output link through lpGBT
    printf("Checking ECON-D -> DAQ lpGBT eRx 0...\n");
    lpgbt.check_prbs_errors_erx(0);
  } else {
    // ECON-T has multiple output links through lpGBT
    // connected to channel 0 of a series of groups
    std::vector<std::vector<int>> i_econ_to_group = {
        {0},        // ECON-D
        {0, 1, 2},  // ECON-T1
        {3, 4, 5}   // ECON-T2
    };
    for (int ierx : i_econ_to_group.at(iecon)) {
      printf("Checking ECON-T -> TRG lpGBT eRx %d...\n", ierx);
      lpgbt.check_prbs_errors_erx(ierx);
    }
  }

  printf("\n --- POST-PRBS STATUS ---\n");
  print_locked_status(lpgbt);

  if (is_econd)
    econ.applyParameter("FORMATTERBUFFER", "GLOBAL_PRBS_ON", 0);
  else
    econ.applyParameter("FORMATTERBUFFER", "GLOBAL_ETX_PATTERN", 0);
}

static uint16_t majority_vote_econt(const std::vector<uint16_t>& data) {
  uint16_t result = 0;
  size_t n = data.size();
  size_t threshold = n / 2;

  // Iterate through each bit position (0 to 15)
  for (int i = 0; i < 16; ++i) {
    size_t setBitCount = 0;
    uint16_t mask = (1 << i);

    for (uint16_t value : data) {
      if (value & mask) {
        setBitCount++;
      }
    }

    // If more than half have the bit set, set it in the result
    if (setBitCount > threshold) {
      result |= mask;
    }
  }

  return result;
}

static void align_econ_lpgbt_word(Target* tgt, pflib::ECON& econ) {
  if (econ.type() == "econd") {
    // word-alignment
    uint32_t idle = pftool::readline_int("Idle pattern", 0x1277CC, true);

    bool found_alignment = false;

    std::vector<uint32_t> got;
    for (int phase = 0; phase < 32; phase++) {
      econ.applyParameter("FormatterBuffer", "Global_align_serializer_0",
                          phase);
      usleep(1000);
      std::vector<uint32_t> spy = tgt->elinks().spy(0);
      got.push_back(spy[0]);
      uint32_t obs = spy[0] >> 8;
      if (obs == idle) {
        printf(" Found alignment at %d\n", phase);
        found_alignment = true;
        break;
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

    static const int ICAPTURE_DELAY = 28;
    trig->setup_alignment_capture(ICAPTURE_DELAY);

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
        printf("Link %d Phase %2d:", ilink, phase);
        for (size_t i = 4; i < 8; i++) {
          printf(" %08x %04x %04x", samples[i], (samples[i] >> 16) & ALIGN_MASK,
                 samples[i] & ALIGN_MASK);
          readings.push_back((samples[i] >> 16) & ALIGN_MASK);
          readings.push_back(samples[i] & ALIGN_MASK);
        }
        printf("\n");
        uint16_t got = majority_vote_econt(readings);
        if (got == idle) {
          printf(" Majority voted in favor of an idle!\n");
          got_idle_phase = phase;
        }
      }
      if (got_idle_phase < 0) {
        printf(" Unable to find alignment for ilink %d\n", ilink);
      } else {
        econ.applyParameter("FORMATTERBUFFER", reg_name, got_idle_phase);
      }
    }
  }
}

void align_econ_lpgbt(Target* tgt) {
  int iecon =
      pftool::readline_int("Which ECON to manage: ", pftool::state.iecon);

  pflib::ECON& econ = tgt->econ(iecon);

  if (pftool::readline_bool("Do bit alignment?", true)) {
    align_econ_lpgbt_bit(tgt, econ, iecon);
  }
  if (pftool::readline_bool("Continue to word alignment?", true)) {
    align_econ_lpgbt_word(tgt, econ);
  }
}
