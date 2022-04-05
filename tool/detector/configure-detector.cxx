/**
 * configure entire HCal detector from a single YAML
 */

#include <iostream>
#include <iomanip>
#include <fstream>

#include "pflib/detector/DetectorConfiguration.h"
#include "pflib/Exception.h"

static void usage() {
  std::cout <<
    "\n"
    " USAGE:\n"
    "  configure-detector [options] config.yaml\n"
    "\n"
    " OPTIONS:\n"
    "  -h,--help    : Print this help and exit\n"
    "  --test       : Print final settings without actually applying them to the chips\n"
    "  --no-confirm : Just apply the settings, don't check with user\n"
    << std::endl;
}

int main(int argc, char *argv[]) {
  if (argc == 1) {
    usage();
    return 1;
  }

  bool confirm = true;
  bool test  = false;
  std::string config;
  for (int i_arg{1}; i_arg < argc; i_arg++) {
    std::string arg{argv[i_arg]};
    if (arg[0] == '-') {
      // option
      if (arg == "--test") {
        test = true;
      } else if (arg == "--no-confirm") {
        confirm = false;
      } else if (arg == "--help" or arg == "-h") {
        usage();
        return 0;
      } else {
        std::cerr << "ERROR: " << arg << " not a recognized argument." << std::endl;
        return 1;
      }
    } else {
      // positional ==> settings file
      if (not config.empty()) {
        std::cerr << "ERROR: Only a single config file can be provided." << std::endl;
        return 2;
      }
      config = arg;
    }
  }

  if (config.empty()) {
    std::cerr << "ERROR: We a configuration to apply." << std::endl;
    return 3;
  }

  try {
    pflib::DetectorConfiguration dc(config);   
    if (test) {
      std::cout << dc << std::endl;
    } else {
      if (confirm) {
        std::cout << dc << std::endl;
        char a;
        std::cout << "Apply these settings to the detector? [Y/N] " << std::flush;
        std::cin >> a;
        if (a != 'y' and a != 'Y') {
          std::cout << "  Not applying settings. Exiting..." << std::endl;
          return 0;
        }
      }
      std::cout << "  Applying settings..." << std::endl;
      dc.apply();
      std::cout << "  done" << std::endl;
    }
  } catch (const pflib::Exception& e) {
    std::cerr << "ERROR: " << "[" << e.name() << "] "
      << e.message() << std::endl;
    return -1;
  }

  return 0;
}
