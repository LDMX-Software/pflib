#include "tool.h"

/**
 * Simply print the currently selective ROC so that user is aware
 * which ROC they are interacting with by default.
 *
 * @param[in] pft active target (not used)
 */
static void roc_render(Target* pft) {
  printf(" Active ROC: %d (typ_version = %s)\n", pftool::state.iroc, pftool::state.type_version.c_str());
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
 * - RESYNCLOAD : pflib::Hcal::resyncLoadROC
 * - RUNMODE :
 * - IROC : Change which ROC to focus on
 * - PAGE : pflib::ROC::readPage
 * - PARAM_NAMES : Use pflib::parameters to get list ROC parameter names
 * - POKE_REG : pflib::ROC::setValue
 * - POKE_PARAM : pflib::ROC::applyParameter
 * - LOAD_REG : pflib::Target::loadROCRegisters
 * - LOAD_PARAM : pflib::Target::loadROCParameters
 * - DUMP : pflib::Target::dumpSettings
 *
 * @param[in] cmd ROC command
 * @param[in] pft active target
 */
static void roc(const std::string& cmd, Target* pft) {
  static std::vector<std::string> page_names;
  static std::map<std::string, std::vector<std::string>> param_names;
  if (page_names.empty()) {
    // generate lists of page names and param names for those pages
    // for tab completion
    // only need to do this if the ROC changes pftool::state.type_version
    auto defs = pflib::Compiler::get(pftool::state.type_version).defaults();
    for (const auto& page : defs) {
      page_names.push_back(page.first);
      for (const auto& param : page.second) {
        param_names[page.first].push_back(param.first);
      }
    }
  }
  if (cmd == "HARDRESET") {
    pft->hcal().hardResetROCs();
  }
  if (cmd == "SOFTRESET") {
    pft->hcal().softResetROC();
  }
  if (cmd == "IROC") {
    pftool::state.iroc = pftool::readline_int("Which ROC to manage: ", pftool::state.iroc);
    auto new_tv =
        pftool::readline("type_version of the HGCROC: ", pftool::state.type_version);
    if (new_tv != pftool::state.type_version) {
      // generate lists of page names and param names for those pages
      // for tab completion
      // only need to do this if the ROC changes type_version
      auto defs = pflib::Compiler::get(new_tv).defaults();
      for (const auto& page : defs) {
        page_names.push_back(page.first);
        for (const auto& param : page.second) {
          param_names[page.first].push_back(param.first);
        }
      }
    }
    pftool::state.type_version = new_tv;
  }
  pflib::ROC roc = pft->hcal().roc(pftool::state.iroc, pftool::state.type_version);
  if (cmd == "RUNMODE") {
    bool isRunMode = roc.isRunMode();
    isRunMode = pftool::readline_bool("Set ROC runmode: ", isRunMode);
    roc.setRunMode(isRunMode);
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
    // TODO filter out the readonly DA parameters on register 7
    auto name = pftool::readline("Direct Access Parameter to Set: ", options);
    bool val = pftool::readline_bool("On/Off: ", true);
    roc.setDirectAccess(name, val);
  }
  if (cmd == "PAGE") {
    int page = pftool::readline_int("Which page? ", 0);
    int len = pftool::readline_int("Length?", 8);
    std::vector<uint8_t> v = roc.readPage(page, len);
    for (int i = 0; i < int(v.size()); i++) printf("%02d : %02x\n", i, v[i]);
  }
  if (cmd == "PARAM_NAMES") {
    std::cout << "Select a page type from the following list:\n";
    for (const auto& page : page_names) {
      std::cout << " - " << page << "\n";
    }
    std::cout << std::endl;
    std::string p = pftool::readline("Page type? ", page_names);
    auto param_list_it = param_names.find(p);
    if (param_list_it == param_names.end()) {
      PFEXCEPTION_RAISE("BadPage", "Page name " + p + " not known.");
    }
    for (const std::string& pn : param_list_it->second) {
      std::cout << pn << "\n";
    }
    std::cout << std::endl;
  }
  if (cmd == "POKE_REG") {
    int page = pftool::readline_int("Which page? ", 0);
    int entry = pftool::readline_int("Offset: ", 0);
    int value = pftool::readline_int("New value: ");

    roc.setValue(page, entry, value);
  }
  if (cmd == "POKE" || cmd == "POKE_PARAM") {
    std::string page = pftool::readline("Page: ", page_names);
    if (param_names.find(page) == param_names.end()) {
      PFEXCEPTION_RAISE("BadPage", "Page name " + page + " not recognized.");
    }
    std::string param = pftool::readline("Parameter: ", param_names.at(page));
    int val = pftool::readline_int("New value: ");

    roc.applyParameter(page, param, val);
  }
  if (cmd == "LOAD_REG") {
    std::cout << "\n"
                 " --- This command expects a CSV file with columns "
                 "[page,offset,value].\n"
              << std::flush;
    std::string fname = pftool::readline("Filename: ");
    roc.loadRegisters(fname);
  }
  if (cmd == "LOAD" || cmd == "LOAD_PARAM") {
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
    std::string fname = pftool::readline_path("hgcroc_"+std::to_string(pftool::state.iroc)+"_settings");
    bool decompile =
        pftool::readline_bool("Decompile register values? ", true);
    if (decompile) {
      fname += ".yaml";
    } else {
      fname += ".csv";
    }
    roc.dumpSettings(fname, decompile);
  }
}


namespace {
auto menu_roc =
    pftool::menu("ROC", "Read-Out Chip Configuration", roc_render)
        ->line("HARDRESET", "Hard reset to all rocs", roc)
        ->line("SOFTRESET", "Soft reset to all rocs", roc)
        //->line("RESYNCLOAD","ResyncLoad to specified roc to help maintain link
        //stability", roc)
        ->line("DIRECT_ACCESS_PARAMETERS",
               "list the direct access parameters we know about", roc)
        ->line("GET_DIRECT_ACCESS", "print a direct access parameter", roc)
        ->line("SET_DIRECT_ACCESS", "set a direct access parameter", roc)
        ->line("IROC", "Change the active ROC number", roc)
        ->line("RUNMODE", "set/clear the run mode", roc)
        ->line("CHAN", "Dump link status", roc)
        ->line("PAGE", "Dump a page", roc)
        ->line("PARAM_NAMES", "Print a list of parameters on a page", roc)
        ->line("POKE_REG", "Change a single register value", roc)
        ->line("POKE_PARAM", "Change a single parameter value", roc)
        ->line("POKE", "Alias for POKE_PARAM", roc)
        //->line("POKE_ALL_ROCHALVES", "Like POKE_PARAM, but applies parameter
        //to both halves of all ROCs", roc)
        //->line("POKE_ALL_CHANNELS", "Like POKE_PARAM, but applies parameter to
        //all channels of the all ROCs", roc)
        ->line("LOAD_REG", "Load register values onto the chip from a CSV file",
               roc)
        ->line("LOAD_PARAM",
               "Load parameter values onto the chip from a YAML file", roc)
        ->line("LOAD", "Alias for LOAD_PARAM", roc)
        ->line("DUMP", "Dump hgcroc settings to a file", roc)
    //->line("DEFAULT_PARAMS", "Load default YAML files", roc)
    ;
}
