/**
 * decoder for viewing raw data files in terminal
 */

#include <iostream>
#include <optional>

#include "pflib/logging/Logging.h"
#include "pflib/packing/FileReader.h"
#include "pflib/packing/Hex.h"
#include "pflib/packing/MultiSampleECONDEventPacket.h"
#include "pflib/version/Version.h"

static void usage() {
  std::cout << "\n"
               " USAGE:\n"
               "  econd-decoder [options] NLINKS input_file.{raw,data}\n"
               "\n"
               " OPTIONS:\n"
               "  -h,--help    : print this help and exit\n"
               "  -o,--output  : output CSV file to dump samples into "
               "(default is input file with extension changed)\n"
               "  -n,--nevents : provide maximum number of events (default is "
               "all events possible)\n"
               "  -l,--log     : logging level to printout (-1: trace up to 4: "
               "fatal)\n"
               "  --is[-not]-rogue : set if the input file was written by Rogue or "
               "not (default is to check extension)"
            << std::endl;
}

int main(int argc, char* argv[]) {
  pflib::logging::fixture f;

  auto the_log_{pflib::logging::get("econd-decoder")};

  pflib_log(warn) << "The ECOND decoding is not well tested. Inspect the "
                     "results carefully and please contribute!";

  if (argc == 1) {
    // can't do anything without any arguments
    usage();
    return 1;
  }

  bool trigger{false};
  std::optional<bool> is_rogue;

  int n_links{-1};
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
      } else if (arg == "--is-rogue") {
        is_rogue = true;
      } else if (arg == "--is-not-rogue") {
        is_rogue = false;
      } else {
        pflib_log(fatal) << "Unrecognized option " << arg;
        return 1;
      }
    } else {
      if (n_links <= 0) {
        try {
          n_links = std::stoi(arg);
        } catch (const std::invalid_argument& e) {
          pflib_log(fatal) << "The first positional argument needs to be the "
                              "number of links, but '"
                           << arg << "' is not an integer.";
          return 1;
        }
      } else if (not in_file.empty()) {
        pflib_log(fatal) << "Can only decode one file at a time.";
        return 1;
      } else {
        in_file = arg;
      }
    }
  }

  pflib_log(debug) << pflib::version::debug();

  if (in_file.empty()) {
    pflib_log(fatal) << "Need to provide a file to decode.";
    usage();
    return 1;
  }

  if (not is_rogue.has_value()) {
    pflib_log(debug) << "neither --is-rogue nor --is-not-rogue provided"
                     << " on command line, checking extension";
    auto ext = in_file.substr(in_file.find_last_of("."));
    if (ext == ".raw") {
      is_rogue = false;
    } else if (ext == ".dat") {
      is_rogue = true;
    } else {
      pflib_log(warn) << "unrecognized extension '" << ext
                      << "' from input file '" << in_file
                      << "'. Guessing NOT rogue.";
      is_rogue = false;
    }
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
    o << pflib::packing::ECONDEventPacket::to_csv_header << '\n';

    pflib::packing::MultiSampleECONDEventPacket ep(n_links);
    // count is NOT written into output file,
    // we use the event number from the links
    // this is just to allow users to limit the number of entries in
    // the output CSV if desired
    int count{0};

    uint32_t size, flags_err_ch, header;
    std::vector<uint32_t> frame;
    while (r) {
      pflib_log(info) << "popping " << count << " event from stream";
      if (is_rogue.value()) {
        using pflib::packing::mask;
        if(!r >> size >> flags_err_ch) {
          pflib_log(error) << "failure to readout header Rogue File Frame header";
          return 1;
        }
        long unsigned int ch{flags_err_ch & mask<8>};
        if (ch != 0) {
          pflib_log(debug) << "skipping frame not corresponding to data (channel = " << ch << ")";
          r.seek(size);
          continue;
        }
        // Rogue channel=0 (data), peak at next word to see if its the correct subsystem
        if (!(r >> header)) {
          pflib_log(error) << "partial frame transmitted, failure to readout LDMX RoR Header";
          return 1;
        }
        printf("%08x\n", header);
        long unsigned int subsys{((header >> 16) & mask<8>)};
        if (subsys != 5) {
          pflib_log(debug) << "skipping frame not corresponding to calorimeter subsystem (subsys = " << subsys << ")";
          r.seek(size - 4);
          continue;
        }
        frame.clear();
        frame.push_back(header);
        std::size_t n_words{(size - 4)/4}, offset{1};
        if(!r.Reader::read(frame, n_words, offset)) {
          pflib_log(error) << "partial frame transmitted, failure to read frame";
          return 1;
        }
        ep.from(frame, true);
      } else {
        // the non-rogue writer within pflib contains nothing
        // except a sequence of ECOND event packets
        r >> ep;
      }
      pflib_log(debug) << "r.eof(): " << std::boolalpha << r.eof()
                       << " and bool(r): " << bool(r);
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
