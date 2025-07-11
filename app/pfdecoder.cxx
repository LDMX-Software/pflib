/**
 * decoder for viewing raw data files in terminal
 */

#include <iostream>

#include "pflib/Logging.h"
#include "pflib/version/Version.h"
#include "pflib/packing/FileReader.h"
#include "pflib/packing/Hex.h"
#include "pflib/packing/SingleROCEventPacket.h"

static void usage() {
  std::cout << "\n"
               " USAGE:\n"
               "  pfdecoder [options] input_file.raw\n"
               "\n"
               " OPTIONS:\n"
               "  -h,--help    : print this help and exit\n"
               "  -o,--output  : output CSV file to dump samples into (default "
               "is input file with extension changed)\n"
               "  -n,--nevents : provide maximum number of events (default is "
               "all events possible)\n"
               "  -l,--log     : logging level to printout (-1: trace up to 4: "
               "fatal)\n"
               "  --headers    : print header words and decoded value to term\n"
               "  --trigger    : write out trigger links to term\n"
            << std::endl;
}

int main(int argc, char* argv[]) {
  pflib::logging::fixture f;
  if (argc == 1) {
    // can't do anything without any arguments
    usage();
    return 1;
  }

  auto the_log_{pflib::logging::get("pfdecoder")};

  bool trigger{false};
  bool headers{false};
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
        if (i_arg + 1 == argc or argv[i_arg + 1][0] == '-') {
          pflib_log(fatal) << "The " << arg
                           << " parameter requires an argument after it.";
          return 1;
        }
        i_arg++;
        out_file = argv[i_arg];
      } else if (arg == "-n" or arg == "--nevents") {
        if (i_arg + 1 == argc or argv[i_arg + 1][0] == '-') {
          pflib_log(fatal) << "The " << arg
                           << " parameter requires an argument after it.";
          return 1;
        }
        i_arg++;
        try {
          nevents = std::stoi(argv[i_arg]);
        } catch (const std::invalid_argument& e) {
          pflib_log(fatal) << "The argument to " << arg << " '" << argv[i_arg]
                           << "' is not an integer.";
          return 1;
        }
      } else if (arg == "-l" or arg == "--log") {
        if (i_arg + 1 == argc) {
          pflib_log(fatal) << "The " << arg
                           << " parameter requires an argument after it.";
          return 1;
        }
        std::string arg_p1{argv[i_arg + 1]};
        if (arg_p1[0] == '-' and arg_p1 != "-1") {
          pflib_log(fatal) << "The " << arg
                           << " parameter requires an argument after it.";
          return 1;
        }
        i_arg++;
        try {
          pflib::logging::set(pflib::logging::convert(std::stoi(argv[i_arg])));
        } catch (const std::invalid_argument& e) {
          pflib_log(fatal) << "The argument to " << arg << " '" << argv[i_arg]
                           << "' is not an integer.";
          return 1;
        }
      } else if (arg == "--headers") {
        headers = true;
      } else if (arg == "--trigger") {
        trigger = true;
      } else {
        pflib_log(fatal) << "Unrecognized option " << arg;
        return 1;
      }
    } else {
      if (not in_file.empty()) {
        pflib_log(fatal) << "Can only decode one file at a time.";
        return 1;
      }
      in_file = arg;
    }
  }

  pflib_log(debug) << pflib::version::debug();

  if (in_file.empty()) {
    pflib_log(fatal) << "Need to provide a file to decode.";
    usage();
    return 1;
  }

  if (out_file.empty()) {
    out_file = in_file.substr(0, in_file.find_last_of(".")) + ".csv";
  }

  pflib::packing::FileReader r{in_file};
  if (not r) {
    pflib_log(fatal) << "Unable to open file '" << in_file << "'.";
    return 1;
  }

  std::ofstream o{out_file};
  if (not o) {
    pflib_log(fatal) << "Unable to open file '" << out_file << "'.";
    return 1;
  }

  try {
    o << std::boolalpha;
    o << pflib::packing::SingleROCEventPacket::to_csv_header << '\n';
  
    pflib::packing::SingleROCEventPacket ep;
    // count is NOT written into output file,
    // we use the event number from the links
    // this is just to allow users to limit the number of entries in
    // the output CSV if desired
    int count{0};
  
    while (r) {
      pflib_log(info) << "popping " << count << " event from stream";
      r >> ep;
      pflib_log(debug) << "r.eof(): " << std::boolalpha << r.eof()
                       << " and bool(r): " << bool(r);
      if (headers) {
        for (std::size_t i_link{0}; i_link < 2; i_link++) {
          const auto& daq_link{ep.daq_links[i_link]};
          std::cout << "Link " << i_link << " : " << "BX = " << daq_link.bx
                    << " Event = " << daq_link.event
                    << " Orbit = " << daq_link.orbit << std::endl;
        }
      }

      if (trigger) {
        for (std::size_t i_link{0}; i_link < 4; i_link++) {
          for (std::size_t i_sum{0}; i_sum < 4; i_sum++) {
            std::cout << "TC" << i_link << "_" << i_sum
              << " = " << ep.trigsum(i_link, i_sum) << std::endl;
          }
        }
      }

      ep.to_csv(o);
      count++;
      if (nevents > 0 and count >= nevents) {
        break;
      }
    }
  } catch (const std::runtime_error& e) {
    pflib_log(fatal) << e.what();
    return 1;
  }

  return 0;
}
