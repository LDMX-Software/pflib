#include "align_econ_lpgbt.h"

#include "pflib/OptoLink.h"
#include "pflib/TRIG.h"
#include "pflib/utility/string_format.h"

static void print_locked_status(pflib::lpGBT& lpgbt) {
  constexpr uint16_t REG_EPRX0LOCKED = 0x152;

  auto read_result = lpgbt.read({REG_EPRX0LOCKED});

  uint8_t ch_locked = (read_result >> 4) & 0xF;

  bool locked = (ch_locked >> 0) & 0x1;
  printf(" Channel %d: %s\n", 0, locked ? "LOCKED" : "UNLOCKED");

  uint8_t state = read_result & 0x3;
  const char* state_name;
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
  printf(" Group 0 state: %s (%d)\n\n", state_name, state);
}

static void print_phase_status(pflib::lpGBT& lpgbt) {
  constexpr uint16_t REG_EPRX0CURRENTPHASE10 = 0x153;

  auto read_result = lpgbt.read({REG_EPRX0CURRENTPHASE10});

  uint16_t ch_0 = (read_result >> 0) & 0xF;

  printf(" Channel 0 phase: %u\n", ch_0);
}

static void align_econ_lpgbt_bit(Target* tgt, pflib::ECON& econ) {
  // ----- bit alignment with PRBS7 as input -----
  // assumes the OptoLinks are named "DAQ" and "TRG" like in HcalBackplaneZCU,
  // EcalSMMTargetZCU, HcalBackplaneBW, and EcalSMMTargetBW
  bool is_econd = (econ.type() == "econd");

  pflib::lpGBT lpgbt{(is_econd)
                         ? (tgt->get_opto_link("DAQ").lpgbt_transport())
                         : (tgt->get_opto_link("TRG").lpgbt_transport())};
  uint32_t prbs_state;

  if (is_econd) {
    printf(" NOTE: Only checking Group 0, Channel 0\n");
    prbs_state = econ.readParameter("FORMATTERBUFFER", "GLOBAL_PRBS_ON");
    printf(" ECON PRBS State: %lu\n", prbs_state);
    printf("\n --- PRE-PRBS STATUS ---\n");
    print_phase_status(lpgbt);
    print_locked_status(lpgbt);

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

  // Only checking group 0 and channel 0 right now
  printf(" Checking ECOND PRBS on group 0, channel 0...\n");

  if (is_econd)
    prbs_state = econ.readParameter("FORMATTERBUFFER", "GLOBAL_PRBS_ON");
  else
    prbs_state = econ.readParameter("FORMATTERBUFFER", "GLOBAL_ETX_PATTERN");

  printf(" ECON PRBS State: %lu\n", prbs_state);

  lpgbt.check_prbs_errors_erx(0, 0,
                              false);  // group 0, ch 0, false for ECON

  printf("\n --- POST-PRBS STATUS ---\n");
  print_phase_status(lpgbt);
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
        uint16_t got = majority_vote_econt(readings);
        if (got == idle) break;
        if (phase == 15) {
          printf(" Unable to find alignment for ilink %d\n", ilink);
        }
      }
    }
  }
}

void align_econ_lpgbt(Target* tgt) {
  int iecon =
      pftool::readline_int("Which ECON to manage: ", pftool::state.iecon);

  if (pftool::state.iecon != 0) {
    printf(" I only know how to align ECON-D to link 0\n");
    return;
  }

  pflib::ECON& econ = tgt->econ(iecon);

  if (pftool::readline_bool("Do bit alignment?", true))
    align_econ_lpgbt_bit(tgt, econ);
  align_econ_lpgbt_word(tgt, econ);
}
