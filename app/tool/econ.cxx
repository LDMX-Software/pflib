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
 * - POKE : set a specific register to a specific value pflib::ECON::setValue
 */
static void econ_expert(const std::string& cmd, Target* tgt) {
  auto econ = tgt->hcal().econ(pftool::state.iecon, pftool::state.type_econ());
  if (cmd == "POKE") {
    int page = pftool::readline_int("Which page? ", 0);
    int entry = pftool::readline_int("Offset: ", 0);
    int value = pftool::readline_int("New value: ");

    econ.setValue(page, entry, value);
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
 * - POKE : pflib::ECON::setValue
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
    //auto page = pftool::readline("Page: ", pftool::state.page_names());
    //auto param = pftool::readline("Parameter: ", pftool::state.param_names(page));
    //int val = pftool::readline_int("New value: ");
    //econ.applyParameter(page, param, val);
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
    ;

auto menu_econ_expert =
    menu_econ->submenu("EXPERT", "expert interaction with ECON", econ_expert_render)
        ->line("POKE", "change a single register's value", econ_expert)
    ;
}
