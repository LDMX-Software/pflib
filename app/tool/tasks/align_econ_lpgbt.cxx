#include "align_econ_lpgbt.h"

#include "pflib/OptoLink.h"

static void print_locked_status(pflib::lpGBT& lpgbt) {
  constexpr uint16_t REG_EPRX0LOCKED = 0x142;

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
  pflib::lpGBT lpgbt_daq{tgt->get_opto_link("DAQ").lpgbt_transport()};
  // pflib::lpGBT lpgbt_trg{tgt->get_opto_link("TRG").lpgbt_transport()};

  printf(" NOTE: Only checking Group 0, Channel 0\n");
  auto prbs_state = econ.readParameter("FORMATTERBUFFER", "GLOBAL_PRBS_ON");
  printf(" ECOND PRBS State: %lu\n", prbs_state);
  printf("\n --- PRE-PRBS STATUS ---\n");
  print_phase_status(lpgbt_daq);
  print_locked_status(lpgbt_daq);

  bool default_invert = (pftool::state.readout_config_is_hcal());

  bool do_invert = pftool::readline_bool("Invert elink data?", default_invert);
  uint8_t invert = (do_invert) ? (1) : (0);
  std::map<std::string, std::map<std::string, uint64_t>> parameters = {};

  parameters["CLOCKSANDRESETS"]["GLOBAL_PUSM_RUN"] = 0;
  econ.applyParameters(parameters);
  usleep(10000);

  parameters.clear();
  parameters["ETX"]["0_INVERT_DATA"] = invert;
  parameters["FORMATTERBUFFER"]["GLOBAL_PRBS_ON"] = 1;
  econ.applyParameters(parameters);
  usleep(10000);

  parameters.clear();
  parameters["CLOCKSANDRESETS"]["GLOBAL_PUSM_RUN"] = 1;
  econ.applyParameters(parameters);
  usleep(10000);

  auto invert_state = econ.readParameter("ETX", "0_INVERT_DATA");
  printf(" ECOND data invert state: %lu\n", invert_state);
  // Only checking group 0 and channel 0 right now
  printf(" Checking ECOND PRBS on group 0, channel 0...\n");

  prbs_state = econ.readParameter("FORMATTERBUFFER", "GLOBAL_PRBS_ON");
  printf(" ECOND PRBS State: %lu\n", prbs_state);

  lpgbt_daq.check_prbs_errors_erx(0, 0,
                                  false);  // group 0, ch 0, false for ECON

  printf("\n --- POST-PRBS STATUS ---\n");
  print_phase_status(lpgbt_daq);
  print_locked_status(lpgbt_daq);

  parameters.clear();
  parameters["FORMATTERBUFFER"]["GLOBAL_PRBS_ON"] = 0;
  econ.applyParameters(parameters);
}

static void align_econ_lpgbt_word(Target* tgt, pflib::ECON& econ) {
  // word-alignment
  uint32_t idle = pftool::readline_int("Idle pattern", 0x1277CC, true);

  bool found_alignment = false;

  std::vector<uint32_t> got;
  for (int phase = 0; phase < 32; phase++) {
    econ.applyParameter("FormatterBuffer", "Global_align_serializer_0", phase);
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
