#include "align_econ_lpgbt.h"

static void print_locked_status(pflib::lpGBT& lpgbt) {
  constexpr uint16_t REG_EPRX0LOCKED = 0x152;

  auto read_result = lpgbt.read({REG_EPRX0LOCKED});

  printf(" EPRX0LOCKED register raw result = 0x%02X\n", read_result);

  uint8_t ch_locked = (read_result >> 4) & 0xF;

  printf(" Channel lock status:\n");
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

void align_econ_lpgbt(Target* tgt) {
  auto econ = tgt->hcal().econ(pftool::state.iecon);

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

  // Group 0
  constexpr uint16_t REG_EPRX0LOCKED = 0x152;
  constexpr uint16_t REG_EPRX0CURRENTPHASE10 = 0x153;
  constexpr uint16_t REG_EPRX0CURRENTPHASE32 = 0x154;

  auto pre_phase10_result = lpgbt_daq.read({REG_EPRX0CURRENTPHASE10});
  auto pre_phase32_result = lpgbt_daq.read({REG_EPRX0CURRENTPHASE32});
  auto pre_lock_result = lpgbt_daq.read({REG_EPRX0LOCKED});

  printf("\n --- PRE-PRBS STATUS ---\n");
  printf(" PRE: Current EPRX0 Phase10 = %d\n", pre_phase10_result);
  printf(" PRE: Current EPRX0 Phase32 = %d\n", pre_phase32_result);
  // printf(" PRE: Is EPRX0 locked? = %d\n", pre_lock_result);
  print_locked_status(lpgbt_daq);

  auto econ_setup_builder =
      econ.testParameters().add("FORMATTERBUFFER", "GLOBAL_PRBS_ON", 1);

  auto econ_setup_test = econ_setup_builder.apply();

  printf(" Checking ECOND PRBS on group 0, channel 0...\n");
  lpgbt_daq.check_prbs_errors_erx(0, 0,
                                  false);  // group 0, ch 0, false for ECON

  auto post_phase10_result = lpgbt_daq.read({REG_EPRX0CURRENTPHASE10});
  auto post_phase32_result = lpgbt_daq.read({REG_EPRX0CURRENTPHASE32});
  auto post_lock_result = lpgbt_daq.read({REG_EPRX0LOCKED});

  printf("\n --- POST-PRBS STATUS ---\n");
  printf(" POST: Current EPRX0 Phase10 = %d\n", post_phase10_result);
  printf(" POST: Current EPRX0 Phase32 = %d\n", post_phase32_result);
  // printf(" POST: Is EPRX0 locked? = %d\n", post_lock_result);
  print_locked_status(lpgbt_daq);

  auto econ_finish_builder =
      econ.testParameters().add("FORMATTERBUFFER", "GLOBAL_PRBS_ON", 0);

  auto econ_finish_test = econ_finish_builder.apply();
}
