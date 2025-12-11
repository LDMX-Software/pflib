/**
 * decoder for data files written by rogue
 */

#include <iostream>

#include "pflib/Exception.h"
#include "pflib/logging/Logging.h"
#include "pflib/packing/Hex.h"
#include "pflib/packing/MultiSampleECONDEventPacket.h"
#include "pflib/version/Version.h"
#include "rogue/GeneralError.h"
#include "rogue/Helpers.h"
#include "rogue/interfaces/stream/Frame.h"
#include "rogue/interfaces/stream/FrameIterator.h"
#include "rogue/interfaces/stream/Slave.h"
#include "rogue/utilities/fileio/StreamReader.h"

/**
 * Accept frames from the rogue stream reader, filtering out non-Ecal stuff
 * and then write out the ECOND event packet to the CSV
 */
class CaloCSVWriter : public rogue::interfaces::stream::Slave {
  mutable pflib::logging::logger the_log_{pflib::logging::get("CaloCSVWriter")};
  std::ofstream output_;
  int contrib_id_{0};
  pflib::packing::MultiSampleECONDEventPacket ep_;

 public:
  CaloCSVWriter(const std::string& filepath, int nlinks, int contrib_id)
      : output_{filepath}, ep_{nlinks}, contrib_id_{contrib_id} {
    if (not output_) {
      PFEXCEPTION_RAISE("NoOpen", "Unable to open file '" + filepath + "'.");
    }

    output_ << pflib::packing::MultiSampleECONDEventPacket::to_csv_header;
  }
  void acceptFrame(
      std::shared_ptr<rogue::interfaces::stream::Frame> frame) override {
    if (frame->getError()) {
      pflib_log(debug) << "Rogue header signaled error present";
      return;
    }

    if (frame->getChannel() != 0) {
      pflib_log(debug) << "Frame belongs to non-data channel "
                       << frame->getChannel();
      return;
    }

    // skip first byte which is meaningless i guess?
    auto frame_it{frame->begin() + 1};
    // get subsystem id
    uint8_t subsystem_id{0}, contrib_id{0}, burn_count{0};
    rogue::interfaces::stream::fromFrame(frame_it, 1, &subsystem_id);
    rogue::interfaces::stream::fromFrame(frame_it, 1, &contrib_id);
    rogue::interfaces::stream::fromFrame(frame_it, 1, &burn_count);

    if (subsystem_id != 5) {
      pflib_log(debug) << "Frame belongs to non-calo subsystem "
                       << subsystem_id;
      return;
    }

    if (contrib_id_ != 0 and contrib_id != contrib_id_) {
      pflib_log(debug) << "Frame belongs to non-selected calo contributor "
                       << contrib_id;
      return;
    }

    uint32_t meaningless{0};
    uint64_t timestamp{0};
    rogue::interfaces::stream::fromFrame(frame_it, 4, &meaningless);
    rogue::interfaces::stream::fromFrame(frame_it, 8, &timestamp);

    int frame_size{frame->end() - frame_it};
    pflib_log(debug) << "Frame size: " << frame_size;
    if (frame_size % 4 != 0) {
      pflib_log(error) << "Frame size " << frame_size
                       << " is not multiple of 4 bytes";
    }

    std::vector<uint32_t> words(frame_size / 4);
    for (int i_word{0}; i_word < words.size(); i_word++) {
      rogue::interfaces::stream::fromFrame(frame_it, 4, &(words[i_word]));
    }

    ep_.from(words);
    ep_.to_csv(output_);
  }
};

static void usage() {
  std::cout << "\n"
               " USAGE:\n"
               "  rogue-decoder [options] NLINKS input_file.dat\n"
               "\n"
               " OPTIONS:\n"
               "  -h,--help    : print this help and exit\n"
               "  -o,--output  : output CSV file to dump samples into "
               "(default is input file with extension changed)\n"
               "  -n,--nevents : provide maximum number of events (default is "
               "all events possible)\n"
               "  -l,--log     : logging level to printout (-1: trace up to 4: "
               "fatal)\n"
               "  -c,--contrib : which contributor to focus on (hcal or ecal)\n"
            << std::endl;
}

int main(int argc, char* argv[]) {
  pflib::logging::fixture f;

  auto the_log_{pflib::logging::get("rogue-decoder")};

  pflib_log(warn) << "The ECOND decoding is not well tested. Inspect the "
                     "results carefully and please contribute!";

  if (argc == 1) {
    // can't do anything without any arguments
    usage();
    return 1;
  }

  int n_links{-1};
  int contrib_id{0};
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
      } else if (arg == "-c" or arg == "--contrib") {
        if (i_arg + 1 == argc or argv[i_arg + 1][0] == '-') {
          pflib_log(fatal) << "The " << arg
                           << " parameter requires an argument after it.";
          return 1;
        }
        i_arg++;
        std::string contrib_name{argv[i_arg]};
        if (contrib_name == "hcal") {
          contrib_id = 1;
        } else if (contrib_name == "ecal") {
          contrib_id = 2;
        } else {
          pflib_log(fatal) << "unrecognized contributor name " << contrib_name
                           << " should be 'hcal' or 'ecal'";
          return 1;
        }
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

  if (out_file.empty()) {
    out_file = in_file.substr(0, in_file.find_last_of(".")) + ".csv";
  }

  try {
    // setup stream
    auto reader = std::make_shared<rogue::utilities::fileio::StreamReader>();
    auto writer =
        std::make_shared<CaloCSVWriter>(out_file, n_links, contrib_id);
    reader->addSlave(writer);

    // run
    reader->open(in_file);
    reader->closeWait();
  } catch (const pflib::Exception& e) {
    pflib_log(fatal) << "pflib [" << e.name() << "]: " << e.message();
    return 1;
  } catch (const rogue::GeneralError& e) {
    pflib_log(fatal) << "Rogue Error: " << e.what();
    return 1;
  } catch (const std::runtime_error& e) {
    pflib_log(fatal) << e.what();
    return 1;
  }

  return 0;
}
