/**
 * decoder for viewing raw data files in terminal
 */

#include <iostream>

#include "pflib/packing/FileReader.h"
#include "pflib/packing/Hex.h"
#include "pflib/packing/SingleROCEventPacket.h"

static void usage() {
  std::cout <<
    "\n"
    " USAGE:\n"
    "  pfdecoder [options] input_file.raw\n"
    "\n"
    " OPTIONS:\n"
    "  -h,--help : print this help and exit\n"
    "  -d,--data : show data words (default is only header words)\n"
    //"  -v level  : define how much printing\n"
  << std::endl;
}

int main(int argc, char* argv[]) {
  std::string in_file;
  bool showdata{false};

  if (argc == 1) {
    // can't do anything without any arguments
    usage();
    return 1;
  }
   
  for (int i_arg{1}; i_arg < argc; i_arg++) {
    std::string arg{argv[i_arg]};
    if (arg[0] == '-') {
      // option
      if (arg == "-h" or arg == "--help") {
        usage();
        return 0;
      } else if (arg == "-d" or arg == "--data") {
        showdata = true;
      } else {
        std::cerr << "ERROR: Unrecognized option " << arg << std::endl;
        return 1;
      }
    } else {
      if (not in_file.empty()) {
        std::cerr << "ERROR: Can only decode one file at a time." << std::endl;
        return 1;
      }
      in_file = arg;
    }
  }

  if (in_file.empty()) {
    std::cerr << "ERROR: Need to provide a file to decode." << std::endl;
    usage();
    return 1;
  }

  pflib::packing::FileReader r{in_file};
  if (not r) {
    std::cerr << "Unable to open file '" << in_file << "'.";
    return 1;
  }

  pflib::packing::SingleROCEventPacket ep;
  int count{0};
  while (r) {
    r >> ep;
    break;
    //if (++count > 2) break;
  }

  return 0;
}
