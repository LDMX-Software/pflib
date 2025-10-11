/**
 * @file econ.cxx
 * ECON menu commands and support functions
 */
#include "pftool.h"

/**
 * Simply print the currently selective ECON so that user is aware
 * which ECON they are interacting with by default.
 *
 * @param[in] pft active target (not used)
 */
static void econ_render(Target* pft) {
  printf(" Active ECON: %d (type_econ_ = %s)\n", pftool::state.iecon, pftool::state.type_econ().c_str());
}

/**
 * Extra instruction for user
 */
static void econ_expert_render(Target* tgt) {
  econ_render(tgt);
  std::cout << "This menu avoids using the 'compiler' to translate parameter names into\n"
               "register values and instead allows you to read/write registers directly.\n";
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
  auto econ = tgt->hcal().econ(pftool::state.iecon, pftool::state.type_econ());
  if (cmd == "READ") {
    std::string addr_str = pftool::readline("Register address (hex): ", "0x0000");
    int address = std::stoi(addr_str, nullptr, 16);
    int nbytes = pftool::readline_int("Number of bytes to read: ", 1);

    std::vector<uint8_t> data = econ.getValues(address, nbytes);

    printf("Read %d bytes from register 0x%04x:\n", nbytes, address);
    for (size_t i = 0; i < data.size(); i++) {
      printf("  [%02zu] = 0x%02x\n", i, data[i]);
    }
  }
  else if (cmd == "WRITE") {
    int address = pftool::readline_int("Register address (hex): ", 0x0000);
    int nbytes = pftool::readline_int("Number of bytes to write: ", 1);
    uint64_t value = pftool::readline_int("Value to write (hex): ", 0x0);

    econ.setValue(address, value, nbytes);
    printf("Wrote 0x%llx to register 0x%04x (%d bytes)\n", value, address, nbytes);
  }
}

/**
 * ECON menu commands
 *
 * When necessary, the ECON interaction object pflib::ECON is created
 * via pflib::Hcal::econ with the currently active econ.
 *
 * ## Commands
 * - HARDRESET : pflib::Hcal::hardResetECONs
 * - SOFTRESET : pflib::Hcal::softResetECONs
 * - RUNMODE : enable run bit on the ECON
 * - IECON : Change which ECON to focus on
 * - LOAD : Load parameters from a YAML file
 *
 * @param[in] cmd ECON command
 * @param[in] pft active target
 */
static void econ(const std::string& cmd, Target* pft) {
  if (cmd == "HARDRESET") {
    pft->hcal().hardResetECONs();
  }
  if (cmd == "SOFTRESET") {
    pft->hcal().softResetECONs();
  }
  if (cmd == "IECON") {
    pftool::state.iecon = pftool::readline_int("Which ECON to manage: ", pftool::state.iecon);
    pftool::state.update_type_econ(
        pftool::readline("type of the ECON (D or T): ", pftool::state.type_econ())
    );
  }
  pflib::ECON econ = pft->hcal().econ(pftool::state.iecon, pftool::state.type_econ());
  if (cmd == "RUNMODE") {
    bool isRunMode = econ.isRunMode();
    isRunMode = pftool::readline_bool("Set ECON runbit: ", isRunMode);
    econ.setRunMode(isRunMode);
  }
  if (cmd == "POKE") {
    auto param =
        pftool::readline("Parameter: ", pftool::state.param_names(0));
    int val = pftool::readline_int("New value: ");
    econ.applyParameter(param, val);
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
  if (cmd == "DUMP") {
    std::string fname = pftool::readline_path("econ_" + std::to_string(pftool::state.iroc) + "_settings", ".yaml");
    econ.dumpSettings(fname, true);
  }
}


namespace {
auto menu_econ =
    pftool::menu("ECON", "ECON Chip Configuration", econ_render)
        ->line("HARDRESET", "Hard reset to all econs", econ)
        ->line("SOFTRESET", "Soft reset to all econs", econ)
        ->line("IECON", "Change the active ECON number", econ)
        ->line("RUNMODE", "set/clear the run mode", econ)
        ->line("POKE", "change a single parameter value", econ)
        ->line("LOAD", "load all parameters", econ)
        ->line("DUMP", "dump parameters", econ)
    ;

auto menu_econ_expert =
  menu_econ->submenu("EXPERT", "expert interaction with ECON", econ_expert_render)
  ->line("READ", "read a single register's value", econ_expert)
  ->line("WRITE", "read a single register's value", econ_expert)
    ;
}
