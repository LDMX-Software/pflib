/**
 * @file econ.cxx
 * ECON menu commands and support functions
 */
#include "pftool.h"

/// print available econ IDs and their types
void print_econs(Target* tgt) {
  for (auto iecon : tgt->hcal().econ_ids()) {
    printf("  %d (%s)\n", iecon, tgt->hcal().econ(iecon).type().c_str());
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
    auto econ{tgt->hcal().econ(pftool::state.iecon)};
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
  auto econ = tgt->hcal().econ(pftool::state.iecon);
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
    printf("Wrote 0x%llx to register 0x%04x (%d bytes)\n", value, address,
           nbytes);
  }
}

static void econ_status(const std::string& cmd, Target* tgt) {
  auto econ{tgt->hcal().econ(pftool::state.iecon)};

  // request that the counters are synchronously copied from internal to
  // readable registers avoiding compiler overhead for these parameters since
  // the compiler is very slow
  econ.setValue(0x40f + 2, 1, 1);
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

  printf(" %18s: %d\n", "PUSM Run Val", econ.getPUSMRunValue());
  printf(" %18s: %d\n", "PUSM State Val", econ.getPUSMStateValue());
  printf(" %18s: %d\n", "Run Mode", econ.isRunMode());
}

/**
 * ECON menu commands
 *
 * When necessary, the ECON interaction object pflib::ECON is created
 * via pflib::Hcal::econ with the currently active econ.
 *
 * ## Commands
 * - HARDRESET : pflib::Hcal::hardResetECONs
 * - SOFTRESET : pflib::Hcal::softResetECON with which=-1
 * - IECON : Change which ECON to focus on
 * - PAGE_NAMES : Use pflib::parameters to get list ECON page names
 * - PARAM_NAMES : Use pflib::parameters to get list ECON parameter names
 * - RUNMODE : enable run bit on the ECON
 * - POKE : pflib::ECON::applyParameter
 * - LOAD : Load parameters from a YAML file
 * - READ : pflib::ECON::readParameter
 * - READCONFIG : Read parameters from a YAML file
 * - DUMP : pflib::ECON::dumpSettings with decompile=true
 *
 * @param[in] cmd ECON command
 * @param[in] pft active target
 */
static void econ(const std::string& cmd, Target* pft) {
  if (cmd == "HARDRESET") {
    pft->hcal().hardResetECONs();
  }
  if (cmd == "SOFTRESET") {
    pft->hcal().softResetECON();
  }
  if (cmd == "IECON") {
    print_econs(pft);
    pftool::state.iecon =
        pftool::readline_int("Which ECON to manage: ", pftool::state.iecon);
  }
  pflib::ECON econ = pft->hcal().econ(pftool::state.iecon);
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
    int edgesel = 0;
    int invertfcmd = 1;
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
    econ.readParameter(page, param);
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
    std::string fname = pftool::readline_path(
        econ.type() + "_" + std::to_string(pftool::state.iecon) + "_settings",
        ".yaml");
    econ.dumpSettings(fname, true);
  }
}

namespace {
auto menu_econ =
    pftool::menu("ECON", "ECON Chip Configuration", econ_render)
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
        ->line("READ", "read one parameter and page", econ);

auto menu_econ_expert =
    menu_econ
        ->submenu("EXPERT", "expert interaction with ECON", econ_expert_render)
        ->line("READ", "read a single register's value", econ_expert)
        ->line("WRITE", "read a single register's value", econ_expert);
}  // namespace
