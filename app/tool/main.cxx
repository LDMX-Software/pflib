/**
 * @file main.cxx File defining the pftool entrypoint
 *
 * The commands are written into files corresponding to the menu's name.
 */

#include <filesystem>
#include <fstream>

#include "pflib/Compile.h"
#include "pflib/Parameters.h"
#include "pflib/version/Version.h"
#include "pftool.h"

pflib::logging::logger get_by_file(const std::string& filepath) {
  static const std::filesystem::path this_file{__FILE__};
  static const std::filesystem::path this_parent{this_file.parent_path()};
  std::filesystem::path fp{filepath};
  std::string relative{std::filesystem::relative(fp, this_parent)};
  std::replace(relative.begin(), relative.end(), '/', '.');
  return pflib::logging::get("pftool." + relative);
}

void pftool::State::init(Target* tgt, int cfg) {
  cfg_ = cfg;
  /// copy over page and param names for tab completion
  std::vector<int> roc_ids{tgt->roc_ids()};
  for (int id : roc_ids) {
    auto defs = tgt->roc(id).defaults();
    for (const auto& page : defs) {
      roc_page_names_[id].push_back(page.first);
      for (const auto& param : page.second) {
        roc_param_names_[id][page.first].push_back(param.first);
      }
    }
  }
  for (int id : tgt->econ_ids()) {
    auto type = tgt->econ(id).type();
    if (econ_page_names_.find(type) == econ_page_names_.end()) {
      auto compiler = pflib::Compiler::get(type);
      for (const auto& page : compiler.defaults()) {
        econ_page_names_[type].push_back(page.first);
        for (const auto& param : page.second) {
          econ_param_names_[type][page.first].push_back(param.first);
        }
      }
    }
  }
}

const std::vector<std::string>& pftool::State::roc_page_names() const {
  return roc_page_names_.at(iroc);
}

const std::vector<std::string>& pftool::State::roc_param_names(
    const std::string& page) const {
  auto PAGE{pflib::upper_cp(page)};

  auto param_list_it = roc_param_names_.at(iroc).find(PAGE);
  if (param_list_it == roc_param_names_.at(iroc).end()) {
    PFEXCEPTION_RAISE("BadPage", "Page name " + page + " not a known page.");
  }
  return param_list_it->second;
}

const std::vector<std::string>& pftool::State::econ_page_names(
    pflib::ECON econ) const {
  return econ_page_names_.at(econ.type());
}

const std::vector<std::string>& pftool::State::econ_param_names(
    pflib::ECON econ, const std::string& page) const {
  auto PAGE{pflib::upper_cp(page)};
  auto param_list_it = econ_param_names_.at(econ.type()).find(PAGE);
  if (param_list_it == econ_param_names_.at(econ.type()).end()) {
    PFEXCEPTION_RAISE("BadPage", "Page name " + page + " not a known page.");
  }
  return param_list_it->second;
}

/// initial definition of the menu state
pftool::State pftool::state{};

ENABLE_LOGGING();

/**
 * Execute a command and capture its output into a string
 *
 * The maximum output is 128 characters.
 *
 * @param[in] cmd command C-string to run
 * @return up to 128 characters of output by cmd
 */
std::string exec(const char* cmd) {
  /**
   * shamelessly taken from https://stackoverflow.com/a/478960
   */
  std::array<char, 128> buffer;
  std::string result;
  std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
  if (!pipe) {
    PFEXCEPTION_RAISE("POpen",
                      "popen() failed to run the command given to exec");
  }
  while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe.get()) !=
         nullptr) {
    result += buffer.data();
  }
  return result;
}

/// firmware name as it appears as a directory on disk
const std::string FW_SHORTNAME_FIBERLESS = "hcal-zcu102";
const std::string FW_SHORTNAME_UIO_ZCU = "dualtarget-zcu102";

/**
 * Check if the firmware supporting the HGCROC is active
 *
 * @return true if correct firmware is active
 */
bool is_fw_active(const std::string& name) {
  static const std::filesystem::path active_fw{"/opt/ldmx-firmware/active"};
  auto resolved{std::filesystem::read_symlink(active_fw).stem()};
  return (resolved == name);
}

/**
 * Get the current firmware version
 *
 * This takes a moment because we simply retrieve the version
 * using an rpm query and the exec function.
 *
 * @return string holding the full firmware version
 */
std::string fw_version() {
  static const std::filesystem::path active_fw{"/opt/ldmx-firmware/active"};
  auto resolved{std::filesystem::read_symlink(active_fw).stem()};
  static const std::string QUERY_CMD =
      "rpm -qa '*" + std::string(resolved) + "*'";
  auto output = exec(QUERY_CMD.c_str());
  output.erase(std::remove(output.begin(), output.end(), '\n'), output.cend());
  return output;
}

/**
 * Main status of menu
 *
 * Prints the firmware and software versions
 *
 * @param[in] pft pointer to active target
 */
static void status(Target* pft) {
  pflib_log(info) << "pflib version: " << pflib::version::debug();
  /*
  if (is_fw_active()) {
    pflib_log(debug) << "fw is active";
  } else {
    pflib_log(fatal) << "fw is not active!";
  }
  */
  pflib_log(info) << "fw version   : " << fw_version();
}

namespace {

auto log_line = pftool::root()->line(
    "LOG", "Configure logging levels", [](pftool::TargetHandle _tgt) {
      std::cout << "  Levels range from trace (-1) up to fatal (4)\n"
                << "  The default logging level is info (2)\n"
                << "  You can select a specific channel to apply a level to "
                   "(e.g. decoding)\n"
                << std::endl;

      int new_level = pftool::readline_int("Level (and above) to print: ", 2);
      std::string only = pftool::readline(
          "Only apply to this channel (empty applies to all): ");
      pflib::logging::set(pflib::logging::convert(new_level), only);
    });

}  // namespace

/**
 * This is the main executable for the pftool.
 *
 * If '-h' or '--help' is the only parameter provided,
 * the usage information is printed and we exit; otherwise,
 * we continue.
 *
 * The RC files  are read from ${PFTOOLRC}, pftoolrc, and ~/.pftoolrc
 * with priority in that order.
 * Then the command line parameters are parsed; if not hostname(s) is(are)
 * provided on the command line, then the 'default_hostname' RC file option
 * is necessary.
 *
 * Initialization scripts (provided with `-s`) are parsed such that white space
 * and lines which start with `#` are ignored. The keystrokes in those "sripts"
 * are run upon launching of the menu and then the user gets control of the tool
 * back after the script is done.
 */
int main(int argc, char* argv[]) {
  pflib::logging::fixture f;
  auto the_log_{pflib::logging::get("pftool")};

  /*****************************************************************************
   * Parse Command Line Parameters
   ****************************************************************************/
  if (argc < 2 or (!strcmp(argv[1], "-h") or !strcmp(argv[1], "--help"))) {
    printf("\nUSAGE:\n");
    printf("   %s [-s SCRIPT] [-h | --help] [-d | --dump] config.yaml\n\n",
           argv[0]);
    printf("OPTIONS:\n");
    printf("  -s : pass a script of commands to run through pftool\n");
    printf("  -h|--help : print this help and exit\n");
    printf(
        "  -d|--dump : print out the entire pftool menu and submenus with "
        "their command descriptions\n");
    printf("\n");
    return 1;
  }

  std::vector<std::string> configs;
  for (int i = 1; i < argc; i++) {
    std::string arg(argv[i]);
    if (arg == "-s") {
      if (i + 1 == argc or argv[i + 1][0] == '-') {
        pflib_log(fatal) << "Argument " << arg << " requires a file after it.";
        return 2;
      }
      i++;
      std::fstream sFile(argv[i]);
      if (!sFile.is_open()) {
        pflib_log(fatal) << "Unable to open script file " << argv[i];
        return 2;
      }

      std::string line;
      while (getline(sFile, line)) {
        // erase whitespace at beginning of line
        while (!line.empty() && isspace(line[0])) line.erase(line.begin());
        // skip empty lines or ones whose first character is #
        if (!line.empty() && line[0] == '#') continue;
        // add to command queue
        pftool::add_to_command_queue(line);
      }
      sFile.close();
    } else if (arg == "-d" or arg == "--dump") {
      // dump out the entire menu to stdout
      pftool::root()->print(std::cout);
      std::cout << std::flush;
      return 0;
    } else {
      // positional argument -> configuration files
      configs.push_back(arg);
    }
  }

  if (configs.size() > 1) {
    std::cerr << "Only one config.yaml at a time rightnow." << std::endl;
    return 2;
  }

  /**
   * Load the configuration YAML
   */
  pflib::Parameters configuration;
  configuration.from_yaml(configs[0], true);

  auto pftool_params{configuration.get<pflib::Parameters>("pftool", {})};

  if (pftool_params.exists("log_level")) {
    pflib::logging::set(
        pflib::logging::convert(pftool_params.get<int>("log_level")));
  }

  if (pftool_params.exists("default_output_directory")) {
    pftool::output_directory =
        pftool_params.get<std::string>("default_output_directory");
  }

  if (pftool_params.exists("timestamp_format")) {
    pftool::timestamp_format =
        pftool_params.get<std::string>("timestamp_format");
  }

  if (pftool_params.exists("runnumber_file")) {
    pftool::state.last_run_file =
        pftool_params.get<std::string>("runnumber_file");
  }

  if (not configuration.exists("target")) {
    std::cerr << "Need to define a 'target' in the configuration." << std::endl;
    return 3;
  }

  auto target{configuration.get<pflib::Parameters>("target")};

  if (not target.exists("type")) {
    std::cerr << "Need to define target's 'type'." << std::endl;
    return 4;
  }

  auto target_type{target.get<std::string>("type")};
  std::unique_ptr<Target> tgt;
  int readout_cfg = -1;
  try {
    if (target_type == "Fiberless" or target_type == "HcalFMC") {
      if (not is_fw_active(FW_SHORTNAME_FIBERLESS)) {
        pflib_log(fatal) << "'" << FW_SHORTNAME_FIBERLESS
                         << "' firmware is not active on ZCU.";
        pflib_log(fatal) << "Connection will likely fail.";
      }

      pflib_log(info) << "connecting from ZCU in Fiberless mode";
      tgt.reset(pflib::makeTargetFiberless());
      readout_cfg = pftool::State::CFG_HCALFMC;
      pftool::root()->drop({"OPTO", "ECON"});
    } else if (target_type == "HcalBackplaneZCU") {
      if (not is_fw_active(FW_SHORTNAME_UIO_ZCU)) {
        pflib_log(fatal) << "'" << FW_SHORTNAME_UIO_ZCU
                         << "' firmware is not active on ZCU.";
        pflib_log(fatal) << "Connection will likely fail.";
      }
      // need ilink to be in configuration
      auto ilink = target.get<int>("ilink");
      if (ilink < 0 or ilink > 1) {
        PFEXCEPTION_RAISE("BadLink",
                          "ZCU HcalBackplance ilink can only be 0 or 1");
      }
      auto boardmask = target.get<int>("boardmask", 0xff);
      tgt.reset(pflib::makeTargetHcalBackplaneZCU(ilink, boardmask));
      readout_cfg = pftool::State::CFG_HCALOPTO;
    } else if (target_type == "HcalBackplaneBittware") {
    } else {
      pflib_log(fatal) << "Target type '" << target_type << "' not recognized.";
      return 1;
    }
  } catch (const pflib::Exception& e) {
    pflib_log(fatal) << "Init Error [" << e.name() << "] : " << e.message();
    return 3;
  }

  /*****************************************************************************
   * Run tool
   ****************************************************************************/
  try {
    pftool::state.init(tgt.get(), readout_cfg);
    pftool::set_history_filepath("~/.pftool-history");
    status(tgt.get());
    pftool::run(tgt.get());
  } catch (const std::exception& e) {
    pflib_log(fatal) << "Unrecognized Exception : " << e.what();
    return 127;
  }
  return 0;
}
