/**
 * configure entire HCal detector from a single YAML
 */

#include <iostream>
#include <iomanip>
#include <fstream>

#include "pflib/Exception.h"
#include "pflib/DetectorConfiguration.h"

static void usage() {
  std::cout <<
    "\n"
    " USAGE:\n"
    "  pfconfigure [options] config.yaml\n"
    "\n"
    " OPTIONS:\n"
    "  -h,--help  : Print this help and exit\n"
    "  --test     : Print final settings without actually applying them to the chips\n"
    "\n"
    << std::endl;
}

int main(int argc, char *argv[]) {
  if (argc == 1) {
    usage();
    return 1;
  }

  bool prepend_defaults = false;
  bool test  = false;
  std::string config;
  for (int i_arg{1}; i_arg < argc; i_arg++) {
    std::string arg{argv[i_arg]};
    if (arg[0] == '-') {
      // option
      if (arg == "--defaults") {
        prepend_defaults = true;
      } else if (arg == "--test") {
        test = true;
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
    if (test) std::cout << dc << std::endl;
    else dc.apply();
  } catch (const pflib::Exception& e) {
    std::cerr << "ERROR: " << "[" << e.name() << "] "
      << e.message() << std::endl;
    return -1;
  }

  return 0;
}
