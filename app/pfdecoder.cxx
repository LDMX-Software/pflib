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
    "  -h,--help    : print this help and exit\n"
    "  -o,--output  : output CSV file to dump samples into (default is input file with extension changed)\n"
    "  -n,--nevents : provide maximum number of events (default is all events possible)\n"
    << std::endl;
}

int main(int argc, char* argv[]) {
  if (argc == 1) {
    // can't do anything without any arguments
    usage();
    return 1;
  }
   
  int nevents{-1};
  std::string in_file, out_file;
  for (int i_arg{1}; i_arg < argc; i_arg++) {
    std::string arg{argv[i_arg]};
    if (arg[0] == '-') {
      // option
      if (arg == "-h" or arg == "--help") {
        usage();
        return 0;
      } else if (arg == "-o" or arg == "--output") {
        if (i_arg+1 == argc or argv[i_arg+1][0] == '-') {
          std::cerr << "ERROR: The " << arg << " parameter requires are argument after it." << std::endl;
          return 1;
        }
        i_arg++;
        out_file = argv[i_arg];
      } else if (arg == "-n" or arg == "--nevents") {
        if (i_arg+1 == argc or argv[i_arg+1][0] == '-') {
          std::cerr << "ERROR: The " << arg << " parameter requires are argument after it." << std::endl;
          return 1;
        }
        i_arg++;
        nevents = std::stoi(argv[i_arg]);
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

  if (out_file.empty()) {
    out_file = in_file.substr(0, in_file.find_last_of("."))+".csv";
  }

  pflib::packing::FileReader r{in_file};
  if (not r) {
    std::cerr << "Unable to open file '" << in_file << "'." << std::endl;
    return 1;
  }

  std::ofstream o{out_file};
  if (not o) {
    std::cerr << "Unable to open file '" + out_file << "'." << std::endl;
    return 1;
  }

  o << std::boolalpha;
  o << "link,bx,event,orbit,channel,Tp,Tc,adc_tm1,adc,tot,toa\n";

  pflib::packing::SingleROCEventPacket ep;
  // count is NOT written into output file,
  // we use the event number from the links
  // this is just to allow users to limit the number of entries in
  // the output CSV if desired
  int count{0};

  while (r) {
    if (!(r >> ep)) break;
    for (std::size_t i_link{0}; i_link < 2; i_link++) {
      const auto& daq_link{ep.daq_links[i_link]};
      o << i_link << ','
        << daq_link.bx << ','
        << daq_link.event << ','
        << daq_link.orbit << ','
        << "calib,"
        << daq_link.calib.Tp() << ','
        << daq_link.calib.Tc() << ','
        << daq_link.calib.adc_tm1() << ','
        << daq_link.calib.adc() << ','
        << daq_link.calib.tot() << ','
        << daq_link.calib.toa() << '\n';
      for (std::size_t i_ch{0}; i_ch < 36; i_ch++) {
        o << i_link << ','
          << daq_link.bx << ','
          << daq_link.event << ','
          << daq_link.orbit << ','
          << i_ch << ','
          << daq_link.channels[i_ch].Tp() << ','
          << daq_link.channels[i_ch].Tc() << ','
          << daq_link.channels[i_ch].adc_tm1() << ','
          << daq_link.channels[i_ch].adc() << ','
          << daq_link.channels[i_ch].tot() << ','
          << daq_link.channels[i_ch].toa() << '\n';
      }
    }
    if (nevents > 0 and ++count > nevents) {
      break;
    }
  }

  return 0;
}
