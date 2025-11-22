#include "align_econ_lpgbt.h"

static void print_locked_status(pflib::lpGBT& lpgbt) {
  constexpr uint16_t REG_EPRX0LOCKED = 0x152;

  auto read_result = lpgbt.read({REG_EPRX0LOCKED});

  printf(" EPRX0LOCKED register raw result = 0x%02X\n", read_result);

  uint8_t ch_locked = (read_result >> 4) & 0xF;

  printf(" Channel lock status...\n");
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
  constexpr uint8_t REG_EPRX0CURRENTPHASE10 = 0x153;

  auto read_result = lpgbt.read({REG_EPRX0CURRENTPHASE10});

  printf(" EPRX0CURRENTPHASE10 register raw result = 0x%02X\n", read_result);

  uint8_t ch_0 = (read_result >> 0) & 0xF;

  printf(" Channel 0 phase: %u\n", ch_0);
}

void align_econ_lpgbt(Target* tgt) {
  auto econ = tgt->econ(pftool::state.iecon);

  int chipaddr = 0x78;
  chipaddr |= 0x4;

  pflib::zcu::lpGBT_ICEC_Simple ic("standardLpGBTpair-0", false, chipaddr);
  pflib::lpGBT lpgbt_daq(ic);
  pflib::zcu::lpGBT_ICEC_Simple ec("standardLpGBTpair-0", true, 0x78);
  pflib::lpGBT lpgbt_trg(ec);

  int pusm_daq = lpgbt_daq.status();
  printf(" lpGBT-DAQ PUSM %s (%d)\n", lpgbt_daq.status_name(pusm_daq).c_str(),
         pusm_daq);

  int pusm_trg = lpgbt_trg.status();
  printf(" lpGBT-TRG PUSM %s (%d)\n", lpgbt_trg.status_name(pusm_trg).c_str(),
         pusm_trg);

  printf("\n --- PRE-PRBS STATUS ---\n");
  print_phase_status(lpgbt_daq);
  print_locked_status(lpgbt_daq);

  auto econ_setup_builder =
      econ.testParameters().add("FORMATTERBUFFER", "GLOBAL_PRBS_ON", 1);
      econ.testParameters().add("Mapping", "0_INVERT_DATA", 1); 
  auto econ_setup_test = econ_setup_builder.apply();

  printf(" Checking ECOND PRBS on group 0, channel 0...\n");
  lpgbt_daq.check_prbs_errors_erx(0, 0,
                                  false);  // group 0, ch 0, false for ECON

  printf("\n --- POST-PRBS STATUS ---\n");
  print_phase_status(lpgbt_daq);
  print_locked_status(lpgbt_daq);

  auto econ_finish_builder =
      econ.testParameters().add("FORMATTERBUFFER", "GLOBAL_PRBS_ON", 0);

  auto econ_finish_test = econ_finish_builder.apply();
}
