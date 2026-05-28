/**
 * @file econ.cxx
 * ECON menu commands and support functions
 */
#include "./econ_snapshot.h"
#include "pftool.h"

/// print available econ IDs and their types
void print_econs(Target* tgt) {
  for (auto iecon : tgt->econ_ids()) {
    printf("  %d (%s)\n", iecon, tgt->econ(iecon).type().c_str());
  }
}

/**
 * Simply print the currently selective ECON so that user is aware
 * which ECON they are interacting with by default.
 *
 * @param[in] pft active target (not used)
 */
static void econ_render(Target* tgt) {
  try {
    auto& econ{tgt->econ(pftool::state.iecon)};
    printf(" Active ECON: %d (%s)\n", pftool::state.iecon, econ.type().c_str());
  } catch (const std::exception&) {
    printf(" Active ECON: %d (Not Configured)\n", pftool::state.iecon);
    print_econs(tgt);
  }
}

/**
 * Extra instruction for user
 */
static void econ_expert_render(Target* tgt) {
  econ_render(tgt);
  std::cout << "This menu avoids using the 'compiler' to translate parameter "
               "names into\n"
               "register values and instead allows you to read/write registers "
               "directly.\n";
}

/**
 * ECON.EXPERT menu commands
 *
 * Detailed interaction with ECON doing things like interacting
 * with the registers that hold the ECON-D/ECON-T parameters without
 * using the compiler
 *
 * ## Commands
 * - READ : read a specific register
 * - WRITE : write to a specific register
 */
static void econ_expert(const std::string& cmd, Target* tgt) {
  auto& econ = tgt->econ(pftool::state.iecon);
  if (cmd == "READ") {
    std::string addr_str =
        pftool::readline("Register address (hex): ", "0x0000");
    int address = std::stoi(addr_str, nullptr, 16);
    int nbytes = pftool::readline_int("Number of bytes to read: ", 1);

    std::vector<uint8_t> data = econ.getValues(address, nbytes);

    printf("Read %d bytes from register 0x%04x:\n", nbytes, address);
    for (size_t i = 0; i < data.size(); i++) {
      printf("  [%02zu] = 0x%02x\n", i, data[i]);
    }
  } else if (cmd == "WRITE") {
    int address = pftool::readline_int("Register address (hex): ", 0x0000);
    int nbytes = pftool::readline_int("Number of bytes to write: ", 1);
    uint64_t value = pftool::readline_int("Value to write (hex): ", 0x0);

    econ.setValue(address, value, nbytes);
    printf("Wrote 0x%lx to register 0x%04x (%d bytes)\n", value, address,
           nbytes);
  }
}

static void econ_status(const std::string& cmd, Target* tgt) {
  auto& econ{tgt->econ(pftool::state.iecon)};

  // request that the counters are synchronously copied from internal to
  // readable registers avoiding compiler overhead for these parameters since
  // the compiler is very slow
  if (econ.type() == "econd") {
    econ.setValue(0x40f, (0 << 2), 1);
    usleep(100);
    econ.setValue(0x40f, (1 << 2), 1);
    usleep(100);
  } else {
    econ.setValue(0xce0, (1 << 1), 1);
  }
  // read fast command counters
  static const int fctrl_base = 0x3ab;
  static const std::vector<std::pair<std::string, int>> counters = {
      {"LOCK_COUNT", 0x1},
      {"BCR", 0x2},
      {"OCR", 0x3},
      {"L1A", 0x4},
      {"NZS", 0x5},
      {"CAL_PULSE_INT", 0x6},
      {"CAL_PULSE_EXT", 0x7},
      {"EBR", 0x8},
      {"ECR", 0x9},
      {"LINK_RESET_ROC_T", 0xa},
      {"LINK_RESET_ROC_D", 0xb},
      {"LINK_RESET_ECON_T", 0xc},
      {"LINK_RESET_ECON_D", 0xd},
      // skipping spare fcmd_count slots
      {"UNASSIGNED", 0x16},
      {"FC_ERROR", 0x17}};
  static const int locked = 1;
  static const int cmd_rx_inverted = 0;
  printf(" %18s: %d\n", "LOCKED",
         ((econ.getValues(fctrl_base, 1).at(0) >> locked) & 0b1));
  printf(" %18s: %d\n", "CMD RX Inverted",
         ((econ.getValues(fctrl_base, 1).at(0) >> cmd_rx_inverted) & 0b1));
  for (const auto& [counter, offset] : counters) {
    printf(" %18s: %d\n", counter.c_str(),
           econ.getValues(fctrl_base + offset, 1).at(0));
  }

  /**
   * The PUSM state names are copied from the online ECON-D/T manual
   * https://econ-user-manual.docs.cern.ch/CommonBlocks/pusm/
   *
   * The numbering there is offset by one compared to the indices that
   * we are reading out from the chip, but I think that makes sense.
   */
  static const std::array<const char*, 9> pusm_state_names = {
      "RESET",         "IDLE",
      "RESET_PLL",     "WAIT_PLL_LOCK",
      "RESET_DLLS",    "WAIT_DLL_RESET_DONE",
      "WAIT_DLL_LOCK", "RESET_LOGIC_USING_DLL",
      "READY"};
  int pusm_state = econ.getPUSMStateValue();
  const char* pusm_state_name =
      ((pusm_state >= 0 and pusm_state < pusm_state_names.size())
           ? pusm_state_names[pusm_state]
           : "???");

  printf(" %18s: %d\n", "PUSM Run Val", econ.getPUSMRunValue());
  printf(" %18s: %d %s\n", "PUSM State Val", pusm_state, pusm_state_name);
  printf(" %18s: %d\n", "Run Mode", econ.isRunMode());

  /**
   * The RO counters and statuses providing extra detail
   */
  static const std::array<const char*, 13> sm_state_names = {
    "Reset", "Init", "CapSearchStart", "CapSearchClearCounters0",
    "CapSearchClearCounters1", "CapSearchEnableCounter", "CapSearchWaitFreqDecision",
    "CapSearchVCOFaster", "CapSearchRefClkFaster", "PLLInit", "CDRInit",
    "PLLEnd", "CDREnd"
  };
  static const std::array<const char*, 4> lock_and_filter_state_names = {
    "Unlocked", "ConfirmLock", "Locked", "ConfirmUnlock"
  };
  static const std::array<const char*, 6> clocks_counters = {
    "timeout_pll", "timeout_dll", "watchdog_pll", "watchdog_dll", "left_ready", "state_upset"
  };
  auto clocks_and_resets_status = econ.getValues(0x39b, 0xb);
  int lock_and_filter_state = ((clocks_and_resets_status.at(0x6) >> 4) & 0x3);
  const char* lock_and_filter_name = ((lock_and_filter_state >= 0 and lock_and_filter_state < lock_and_filter_state_names.size())
      ? lock_and_filter_state_names[lock_and_filter_state]
      : "???");
  for (int i{0}; i < clocks_counters.size(); i++) {
    printf(" %10s counter: %d\n", clocks_counters.at(i), clocks_and_resets_status.at(i));
  }
  printf(" %18s: %d\n", "PUSM State", clocks_and_resets_status.at(0x6) & 0xf); // should be repeat
  printf(" %18s: %d %s\n", "LockFilter State", lock_and_filter_state, lock_and_filter_name);
  printf(" %18s: %d\n", "lock_filter_locked", ((clocks_and_resets_status.at(0x6) >> 6) & 0x1));
  int loss_of_lock_count = ((clocks_and_resets_status.at(0x6) >> 7) & 0x1);
  loss_of_lock_count |= ((clocks_and_resets_status.at(0x7) & 0x7f) << 1);
  printf(" %18s: %d\n", "loss_of_lock_count", loss_of_lock_count);
  int sm_state = ((clocks_and_resets_status.at(0x7) >> 7) & 0x1);
  sm_state |= ((clocks_and_resets_status.at(0x8) & 0x7) << 1);
  const char* sm_state_name = (
      (sm_state >= 0 and sm_state < sm_state_names.size())
      ? sm_state_names[sm_state]
      : "???"
  );
  printf(" %18s: %d %s\n", "PLL Init State", sm_state, sm_state_name);
  printf(" %18s: %d\n", "PLL Locked", (clocks_and_resets_status.at(0x8) >> 3) & 0x1);

  std::map<std::string, std::map<std::string, uint64_t>> clock_state = {
    { "CLOCKSANDRESETS", 
      {
        {"global_pusm_timeout_pll_action_counter", 0},
        {"global_pusm_timeout_dll_action_counter", 0},
        {"global_pusm_watchdog_pll_action_counter", 0},
        {"global_pusm_watchdog_dll_action_counter", 0},
        {"global_pusm_left_ready_action_counter", 0},
        {"global_pusm_state_upset_action_counter", 0},
        {"global_pusm_state", 0},
        {"global_lock_filter_state", 0},
        {"global_lock_filter_locked", 0},
        {"global_lock_filter_loss_of_lock_count", 0},
        {"global_sm_state", 0},
        {"global_sm_locked", 0}
      }
    }
  };
  clock_state = econ.readParameters(clock_state, false);
  for (const auto& [name, value]: clock_state.at("CLOCKSANDRESETS")) {
    printf(" %s: %d\n", name.c_str(), value);
  }

}


/**
 * ECON menu commands
 *
 * When necessary, the ECON interaction object pflib::ECON is created
 * via pflib::Target::econ with the currently active econ.
 *
 * ## Commands
 * - HARDRESET : pflib::Target::hardResetECONs
 * - SOFTRESET : pflib::Target::softResetECON with which=-1
 * - IECON : Change which ECON to focus on
 * - PAGE_NAMES : Use pflib::parameters to get list ECON page names
 * - PARAM_NAMES : Use pflib::parameters to get list ECON parameter names
 * - RUNMODE : enable run bit on the ECON
 * - POKE : pflib::ECON::applyParameter
 * - LOAD : Load parameters from a YAML file
 * - READ : pflib::ECON::readParameter
 * - READCONFIG : Read parameters from a YAML file
 * - DUMP : pflib::ECON::dumpSettings with decompile=true
 * - ECON_SNAPSHOT : Outputs snapshot of ECON channels
 *
 * @param[in] cmd ECON command
 * @param[in] pft active target
 */
static void econ(const std::string& cmd, Target* pft) {
  if (cmd == "HARDRESET") {
    pft->hardResetECONs();
  }
  if (cmd == "SOFTRESET") {
    pft->softResetECON();
  }
  if (cmd == "IECON") {
    print_econs(pft);
    pftool::state.iecon =
        pftool::readline_int("Which ECON to manage: ", pftool::state.iecon);
  }
  pflib::ECON& econ = pft->econ(pftool::state.iecon);
  if (cmd == "PAGENAMES") {
    for (const std::string& pn : pftool::state.econ_page_names(econ)) {
      std::cout << pn << "\n";
    }
    std::cout << std::endl;
  }
  if (cmd == "PARAMNAMES") {
    auto page = pftool::readline("Page? ", pftool::state.econ_page_names(econ));
    for (const std::string& pn : pftool::state.econ_param_names(econ, page)) {
      std::cout << pn << "\n";
    }
    std::cout << std::endl;
  }
  if (cmd == "RUNMODE") {
    bool isRunMode = econ.isRunMode();
    isRunMode = pftool::readline_bool("Set ECON runbit: ", ~isRunMode);
    int edgesel = pftool::readline_int("edgesel: ", 0);
    int invertfcmd = 0;
    if (pftool::state.readout_config() == pftool::State::CFG_HCALOPTO_ZCU ||
        pftool::state.readout_config() == pftool::State::CFG_HCALOPTO_BW) {
      invertfcmd = 1;
    }
    invertfcmd = pftool::readline_int("invertfcmd: ", invertfcmd);
    econ.setRunMode(isRunMode, edgesel, invertfcmd);
    // read status again
    econ.isRunMode();
  }
  if (cmd == "POKE") {
    auto page = pftool::readline("Page? ", pftool::state.econ_page_names(econ));
    auto param = pftool::readline("Parameter: ",
                                  pftool::state.econ_param_names(econ, page));
    int val = pftool::readline_int("New value: ");
    econ.applyParameter(page, param, val);
  }
  if (cmd == "LOAD") {
    std::cout << "\n"
                 " --- This command expects a YAML file with page names, "
                 "parameter names and their values.\n"
              << std::flush;
    std::string fname = pftool::readline("Filename: ");
    bool prepend_defaults = pftool::readline_bool(
        "Update all parameter values on the chip using the defaults in the "
        "manual for any values not provided? ",
        false);
    econ.loadParameters(fname, prepend_defaults);
  }
  if (cmd == "READ") {
    auto page = pftool::readline("Page? ", pftool::state.econ_page_names(econ));
    auto param = pftool::readline("Parameter: ",
                                  pftool::state.econ_param_names(econ, page));
    econ.readParameter(page, param, true);
  }
  if (cmd == "READCONFIG") {
    std::cout << "\n"
                 " --- This command expects a YAML file with page names, "
                 "parameter names and their values.\n"
              << std::flush;
    std::string fname = pftool::readline("Filename: ");
    econ.readParameters(fname);
  }
  if (cmd == "DUMP") {
    // request that the counters are synchronously copied from internal to
    // readable registers avoiding compiler overhead for these parameters since
    // the compiler is very slow
    if (econ.type() == "econd") {
      // ROCDAQCTRL.STROBE_2_STATUS_CAPTURE
      econ.setValue(0x40f, (1 << 2), 1);
      // WATCHDOG.CAPTURE
      econ.setValue(0xd55, (1 << 2), 1);
    } else {
      // MISC.STATUS_CAPTURE
      econ.setValue(0xce0, (1 << 1), 1);
    }
    std::string fname = pftool::readline_path(
        econ.type() + "_" + std::to_string(pftool::state.iecon) + "_settings",
        ".yaml");
    econ.dumpSettings(fname, true);
  }
  if (cmd == "SNAPSHOT") {
    int iecon =
        pftool::readline_int("Which ECON to manage: ", pftool::state.iecon);

    auto& econ = pft->econ(iecon);

    std::string ch_str = pftool::readline(
        "Enter channels (comma-separated), default is all channels: ",
        "0,1,2,3,4,5,6,7");

    std::vector<int> channels;
    std::stringstream ss(ch_str);
    std::string item;

    while (std::getline(ss, item, ',')) {
      try {
        channels.push_back(std::stoi(item));
      } catch (...) {
        std::cerr << "Invalid channel entry: " << item << std::endl;
      }
    }
    econ_snapshot(pft, econ, channels);
  }
}

namespace {
auto menu_econ =
    pftool::menu("ECON", "ECON Chip Configuration", econ_render, NEED_FIBER)
        ->line("STATUS", "print status and FC counters", econ_status)
        ->line("HARDRESET", "Hard reset to all ECONs", econ)
        ->line("SOFTRESET", "Soft reset to all ECONs", econ)
        ->line("IECON", "Change the active ECON number", econ)
        ->line("PAGENAMES", "List all page names for ECON", econ)
        ->line("PARAMNAMES", "List all parameter names for a given page", econ)
        ->line("RUNMODE", "set/clear the run mode", econ)
        ->line("POKE", "change a single parameter value", econ)
        ->line("LOAD", "load all parameters", econ)
        ->line("DUMP", "dump parameters", econ)
        ->line("READCONFIG", "read a yaml file", econ)
        ->line("READ", "read one parameter and page", econ)
        ->line("SNAPSHOT", "Output snapshot of ECON channels", econ);

auto menu_econ_expert =
    menu_econ
        ->submenu("EXPERT", "expert interaction with ECON", econ_expert_render)
        ->line("READ", "read a single register's value", econ_expert)
        ->line("WRITE", "read a single register's value", econ_expert);
}  // namespace
