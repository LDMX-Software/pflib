/**
 * @file roc.cxx
 * ROC menu commands and support functions
 */
#include "pftool.h"

/**
 * Simply print the currently selective ROC so that user is aware
 * which ROC they are interacting with by default.
 *
 * @param[in] pft active target (not used)
 */
static void roc_render(Target* pft) {
  printf(" Active ROC: %d\n", pftool::state.iroc);
}

/**
 * Extra instructsion for user
 */
static void roc_expert_render(Target* tgt) {
  roc_render(tgt);
  std::cout << "This menu avoids using the 'compiler' to translate parameter "
               "names into\n"
               "register values and instead allows you to read/write registers "
               "directly.\n";
}

/**
 * ROC.EXPERT menu commands
 *
 * Detailed interaction with ROC doing things like interacting
 * with the registers that hold the HGCROC parameters without
 * using the compiler
 *
 * ## Commands
 * - PAGE : print a page of raw registers pflib::ROC::readPage
 * - POKE : set a specific register to a specific value pflib::ROC::setValue
 * - LOAD : load a CSV of register values onto chip pflib::ROC::loadRegisters
 * - DUMP : dump HGCROC registers to CSV pflib::ROC::dumpSettings with
 * decompile=false
 * - DIRECT_ACCESS_PARAMETERS : prints all direct access parameters in the ROC
 * - GET_DIRECT_ACCESS : print direct access parameters and their values
 * - SET_DIRECT_ACCESS : set direct access parameter to specific value
 */
static void roc_expert(const std::string& cmd, Target* tgt) {
  auto roc = tgt->hcal().roc(pftool::state.iroc);
  if (cmd == "PAGE") {
    int page = pftool::readline_int("Which page? ", 0);
    int len = pftool::readline_int("Length?", 8);
    std::vector<uint8_t> v = roc.readPage(page, len);
    for (int i = 0; i < int(v.size()); i++) printf("%02d : %02x\n", i, v[i]);
  }
  if (cmd == "POKE") {
    int page = pftool::readline_int("Which page? ", 0);
    int entry = pftool::readline_int("Offset: ", 0);
    int value = pftool::readline_int("New value: ");

    roc.setValue(page, entry, value);
  }
  if (cmd == "LOAD") {
    std::cout << "\n"
                 " --- This command expects a CSV file with columns "
                 "[page,offset,value].\n"
              << std::flush;
    std::string fname = pftool::readline("Filename: ");
    roc.loadRegisters(fname);
  }
  if (cmd == "DUMP") {
    std::string fname = pftool::readline_path(
        "hgcroc_" + std::to_string(pftool::state.iroc) + "_settings", ".csv");
    roc.dumpSettings(fname, false);
  }
  if (cmd == "DIRECT_ACCESS_PARAMETERS") {
    for (const auto& name : roc.getDirectAccessParameters()) {
      std::cout << "  " << name << "\n";
    }
    std::cout << std::flush;
  }
  if (cmd == "GET_DIRECT_ACCESS") {
    auto options = roc.getDirectAccessParameters();
    auto name =
        pftool::readline("Direct Access Parameter to Read: ", options, "all");
    if (name == "all") {
      for (auto name : roc.getDirectAccessParameters()) {
        printf("  %10s = %d\n", name.c_str(), roc.getDirectAccess(name));
      }
    } else {
      printf("  %10s = %d\n", name.c_str(), roc.getDirectAccess(name));
    }
  }
  if (cmd == "SET_DIRECT_ACCESS") {
    auto options = roc.getDirectAccessParameters();
    auto name = pftool::readline("Direct Access Parameter to Set: ", options);
    bool val = pftool::readline_bool("On/Off: ", true);
    roc.setDirectAccess(name, val);
  }
}

/**
 * ROC menu commands
 *
 * When necessary, the ROC interaction object pflib::ROC is created
 * via pflib::Hcal::roc with the currently active roc.
 *
 * ## Commands
 * - HARDRESET : pflib::Hcal::hardResetROCs
 * - SOFTRESET : pflib::Hcal::softResetROC
 * - RUNMODE : enable run mode on the ROC
 * - IROC : Change which ROC to focus on
 * - PAGE : pflib::ROC::readPage
 * - PARAM_NAMES : Use pflib::parameters to get list ROC parameter names
 * - POKE : pflib::ROC::setValue
 * - LOAD : pflib::ROC::loadParameters
 * - DUMP : pflib::ROC::dumpSettings with decompile=true
 *
 * @param[in] cmd ROC command
 * @param[in] pft active target
 */
static void roc(const std::string& cmd, Target* pft) {
  if (cmd == "HARDRESET") {
    pft->hcal().hardResetROCs();
  }
  if (cmd == "SOFTRESET") {
    pft->hcal().softResetROC();
  }
  if (cmd == "IROC") {
    pftool::state.iroc =
        pftool::readline_int("Which ROC to manage: ", pftool::state.iroc);
  }
  pflib::ROC roc = pft->hcal().roc(pftool::state.iroc);
  if (cmd == "RUNMODE") {
    bool isRunMode = roc.isRunMode();
    isRunMode = pftool::readline_bool("Set ROC runmode: ", isRunMode);
    roc.setRunMode(isRunMode);
  }
  if (cmd == "PAGE") {
    auto page = pftool::readline("Page? ", pftool::state.page_names());
    auto params{roc.getParameters(page)};
    for (const auto& [name, val] : params) {
      std::cout << name << ": " << val << '\n';
    }
    std::cout << std::flush;
  }
  if (cmd == "PARAM_NAMES") {
    auto page = pftool::readline("Page? ", pftool::state.page_names());
    for (const std::string& pn : pftool::state.param_names(page)) {
      std::cout << pn << "\n";
    }
    std::cout << std::endl;
  }
  if (cmd == "POKE") {
    auto page = pftool::readline("Page: ", pftool::state.page_names());
    auto param =
        pftool::readline("Parameter: ", pftool::state.param_names(page));
    int val = pftool::readline_int("New value: ");
    roc.applyParameter(page, param, val);
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
    roc.loadParameters(fname, prepend_defaults);
  }
  if (cmd == "DUMP") {
    std::string fname = pftool::readline_path(
        "hgcroc_" + std::to_string(pftool::state.iroc) + "_settings", ".yaml");
    roc.dumpSettings(fname, true);
  }
}

namespace {
auto menu_roc =
    pftool::menu("ROC", "Read-Out Chip Configuration", roc_render)
        ->line("HARDRESET", "Hard reset to all rocs", roc)
        ->line("SOFTRESET", "Soft reset to all rocs", roc)
        ->line("IROC", "Change the active ROC number", roc)
        ->line("RUNMODE", "set/clear the run mode", roc)
        ->line("PAGE", "a page of the parameters on the chip", roc)
        ->line("PARAM_NAMES", "Print a list of parameters on a page", roc)
        ->line("POKE", "change a single parameter value", roc)
        ->line("LOAD", "Load parameter values onto the chip from a YAML file",
               roc)
        ->line("DUMP", "Dump hgcroc settings to a file", roc);

auto menu_roc_expert =
    menu_roc
        ->submenu("EXPERT", "expert interaction with ROC", roc_expert_render)
        ->line("PAGE", "view a page of register values", roc_expert)
        ->line("POKE", "change a single register's value", roc_expert)
        ->line("LOAD", "load many register values from a CSV", roc_expert)
        ->line("DUMP", "write out all the register values into a CSV",
               roc_expert)
        ->line("DIRECT_ACCESS_PARAMETERS",
               "print out the names of the direct access parameters",
               roc_expert)
        ->line("GET_DIRECT_ACCESS",
               "print out values of direct access parameters", roc_expert)
        ->line("SET_DIRECT_ACCESS", "set direct access parameter bits",
               roc_expert);
}  // namespace
