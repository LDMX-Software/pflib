/**
 * @file main.cxx File defining the pftool entrypoint
 *
 * The commands are written into files corresponding to the menu's name.
 */

#include <filesystem>
#include <fstream>

#include "pflib/Compile.h"
#include "pflib/menu/Rcfile.h"
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

void pftool::State::init(Target* tgt) {
  /// copy over page and param names for tab completion
  std::vector<int> roc_ids{tgt->hcal().roc_ids()};
  for (int id : roc_ids) {
    auto defs = tgt->hcal().roc(id).defaults();
    for (const auto& page : defs) {
      roc_page_names_[id].push_back(page.first);
      for (const auto& param : page.second) {
        roc_param_names_[id][page.first].push_back(param.first);
      }
    }
  }
  for (int id : tgt->hcal().econ_ids()) {
    auto type = tgt->hcal().econ(id).type();
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
 * List the options that could be provided within the RC file
 *
 * @param[in] rcfile RC file prepare
 */
void prepareOpts(pflib::menu::Rcfile& rcfile) {
  rcfile.declareVBool("roclinks",
                      "Vector Bool[8] indicating which roc links are active");
  rcfile.declareString("ipbus_map_path",
                       "Full path to directory containgin IP-bus mapping. Only "
                       "required for uHal comm.");
  rcfile.declareString("default_hostname",
                       "Hostname of polarfire to connect to if none are given "
                       "on the command line");
  rcfile.declareString(
      "default_output_directory",
      "Path to default output directory for creating default output filepaths");
  rcfile.declareString("timestamp_format",
                       "C format string for creating timestamp appended to "
                       "default output filepaths");
  rcfile.declareString(
      "runnumber_file",
      "Full path to a file which contains the last run number");
  rcfile.declareInt("log_level",
                    "Level (and above) to print logging for (trace=-1 up to "
                    "fatal=4, default info=2)");
}

/**
 * This is the main executable for the pftool.
 *
 * The options are prepared first so their help information
 * is available for the usage information.
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

  pflib::menu::Rcfile options;
  prepareOpts(options);

  int boardmask = 0xF;
  int ilink = 0;

  // print help before attempting to load RC file incase the RC file is broken
  if (argc == 2 and (!strcmp(argv[1], "-h") or !strcmp(argv[1], "--help"))) {
    printf("\nUSAGE: (HCal HGCROC fiberless mode)\n");
    printf("   %s OPTIONS\n\n", argv[0]);
    printf("OPTIONS:\n");
    printf("  -z : required for fiberless (no-polarfire, zcu102-based) mode\n");
    printf("  -z0 : fiber-based ZCU, backplane 0 (SFP0/SFP1)\n");
    printf("  -z1 : fiber-based ZCU, backplane 1 (SFP2/SFP3)\n");
    printf("  -s : pass a script of commands to run through pftool\n");
    printf("  -h|--help : print this help and exit\n");
    printf(
        "  -d|--dump : print out the entire pftool menu and submenus with "
        "their command descriptions\n");
    printf("\n");

    printf("CONFIG:\n");
    printf(
        " Reading RC files from ${PFTOOLRC}, ${CWD}/pftoolrc, "
        "${HOME}/.pftoolrc with priority in this order.\n");
    options.help();

    printf("\n");
    return 1;
  }

  /*****************************************************************************
   * Load RC File
   ****************************************************************************/
  std::string home = getenv("HOME");
  if (getenv("PFTOOLRC")) {
    if (std::filesystem::exists(getenv("PFTOOLRC"))) {
      pflib_log(info) << "Loading " << getenv("PFTOOLRC");
      options.load(getenv("PFTOOLRC"));
    } else {
      pflib_log(warn) << "PFTOOLRC=" << getenv("PFTOOLRC")
                      << " is not loaded because it doesn't exist.";
    }
  }
  if (std::filesystem::exists("pftoolrc")) {
    pflib_log(info) << "Loading ${CWD}/pftoolrc";
    options.load("pftoolrc");
  }
  if (std::filesystem::exists(home + "/.pftoolrc")) {
    pflib_log(info) << "Loading ~/.pftoolrc";
    options.load(home + "/.pftoolrc");
  }

  if (options.contents().has_key("log_level")) {
    int lvl = options.contents().getInt("log_level");
    pflib::logging::set(pflib::logging::convert(lvl));
  }

  if (options.contents().has_key("default_output_directory")) {
    pftool::output_directory =
        options.contents().getString("default_output_directory");
  }

  if (options.contents().has_key("timestamp_format")) {
    pftool::timestamp_format = options.contents().getString("timestamp_format");
  }

  /*****************************************************************************
   * Parse Command Line Parameters
   ****************************************************************************/
  enum RunMode { Unknown = 0, Fiberless, UIO_ZCU, Rogue } mode = Unknown;

  std::vector<std::string> hostnames;
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
    } else if (arg == "-z") {
      mode = Fiberless;
      if (not is_fw_active(FW_SHORTNAME_FIBERLESS)) {
        pflib_log(fatal) << "'" << FW_SHORTNAME_FIBERLESS
                         << "' firmware is not active on ZCU.";
        pflib_log(fatal) << "Connection will likely fail.";
      }
    } else if (arg == "-z0") {
      mode = UIO_ZCU;
      ilink = 0;
      if (not is_fw_active(FW_SHORTNAME_UIO_ZCU)) {
        pflib_log(fatal) << "'" << FW_SHORTNAME_UIO_ZCU
                         << "' firmware is not active on ZCU.";
        pflib_log(fatal) << "Connection will likely fail.";
      }
    } else if (arg == "-z1") {
      mode = UIO_ZCU;
      ilink = 1;
      if (not is_fw_active(FW_SHORTNAME_UIO_ZCU)) {
        pflib_log(fatal) << "'" << FW_SHORTNAME_UIO_ZCU
                         << "' firmware is not active on ZCU.";
        pflib_log(fatal) << "Connection will likely fail.";
      }
    } else if (arg == "-d" or arg == "--dump") {
      // dump out the entire menu to stdout
      pftool::root()->print(std::cout);
      std::cout << std::flush;
      return 0;
    } else {
      // positional argument -> hostname
      hostnames.push_back(arg);
    }
  }

  if (mode == Unknown) {
    pflib_log(fatal) << "No running mode selected.";
  }

  if (hostnames.size() == 0 && mode == Rogue) {
    std::string hn = options.contents().getString("default_hostname");
    if (hn.empty()) {
      pflib_log(fatal) << "No hostnames to connect to provided on the command "
                          "line or in RC file";
      return 3;
    } else {
      hostnames.push_back(hn);
    }
  }

  /*****************************************************************************
   * Run tool
   ****************************************************************************/
  try {
    int i_host{-1};
    bool continue_running = true;  // used if multiple hosts
    do {
      if (hostnames.size() > 1) {
        while (true) {
          std::cout << "ID - Hostname" << std::endl;
          for (std::size_t k{0}; k < hostnames.size(); k++) {
            std::cout << std::setw(2) << k << " - " << hostnames.at(k)
                      << std::endl;
          }
          i_host = pftool::readline_int(
              " ID of Polarfire Hostname (-1 to exit) : ", i_host);
          if (i_host == -1 or (i_host >= 0 and i_host < hostnames.size())) {
            // valid choice, let's leave
            break;
          } else {
            std::cerr << "\n " << i_host << " is not a valid choice."
                      << std::endl;
          }
        }
        // if user chooses to leave menu
        if (i_host == -1) break;
      } else {
        i_host = 0;
      }

      // initialize connection
      std::unique_ptr<Target> p_pft;
      try {
        switch (mode) {
          case Fiberless:
            pflib_log(info) << "connecting from ZCU in Fiberless mode";
            p_pft.reset(pflib::makeTargetFiberless());
            pftool::root()->drop({"OPTO"});
            break;
          case Rogue:
            PFEXCEPTION_RAISE("BadComm",
                              "Rogue communication mode not implemented");
            break;
          case UIO_ZCU:
            p_pft.reset(pflib::makeTargetHcalBackplaneZCU(ilink, boardmask));
            break;
          default:
            PFEXCEPTION_RAISE(
                "BadComm",
                "Unknown RunMode configured, not able to connect to HGCROC");
            break;
        }
      } catch (const pflib::Exception& e) {
        pflib_log(fatal) << "Init Error [" << e.name() << "] : " << e.message();
        return 3;
      }

      if (options.contents().has_key("runnumber_file")) {
        pftool::state.last_run_file =
            options.contents().getString("runnumber_file");
      }

      pftool::state.init(p_pft.get());

      if (p_pft) {
        // prepare the links
        pftool::set_history_filepath("~/.pftool-history");
        status(p_pft.get());
        pftool::run(p_pft.get());
      } else {
        pflib_log(fatal) << "No Polarfire Target available to connect with. "
                            "Not sure how we got here.";
        return 126;
      }

      if (hostnames.size() > 1) {
        // menu for that target has been exited, check if user wants to choose
        // another host
        continue_running = pftool::readline_bool(
            " Choose a new card/host to connect to ? ", true);
      } else {
        // no other hosts, leave
        continue_running = false;
      }
    } while (continue_running);
  } catch (const std::exception& e) {
    pflib_log(fatal) << "Unrecognized Exception : " << e.what();
    return 127;
  }
  return 0;
}
