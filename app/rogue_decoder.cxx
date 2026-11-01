/**
 * decoder for data files written by rogue
 */

#include <iostream>
#include <memory>
#include <sstream>

#include "pflib/EcalModule.h"
#include "pflib/Exception.h"
#include "pflib/HcalBackplane.h"
#include "pflib/logging/Logging.h"
#include "pflib/packing/Hex.h"
#include "pflib/packing/MultiSampleECONDEventPacket.h"
#include "pflib/packing/SingleECONDRocErxMapping.h"
#include "pflib/version/Version.h"
#ifdef NEED_ROGUE_NUMPY
#include "rogue/numpy.h"
#endif
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
  int only_econd_{-1};
  bool with_cm_{false};
  std::unique_ptr<pflib::packing::SingleECONDRocErxMapping> mapping_;
  pflib::packing::MultiSampleECONDEventPacket ep_;

 public:
  CaloCSVWriter(
      const std::string& filepath, int nlinks, int contrib_id,
      int only_econd = -1, bool with_cm = false,
      std::unique_ptr<pflib::packing::SingleECONDRocErxMapping> mapping = {})
      : output_{filepath},
        contrib_id_{contrib_id},
        only_econd_{only_econd},
        with_cm_{with_cm},
        mapping_{std::move(mapping)},
        ep_{nlinks} {
    if (not output_) {
      PFEXCEPTION_RAISE("NoOpen", "Unable to open file '" + filepath + "'.");
    }

    output_ << (mapping_ ? pflib::packing::MultiSampleECONDEventPacket::
                               to_csv_header_with_mapping
                         : pflib::packing::MultiSampleECONDEventPacket::
                               to_csv_header)
            << '\n';
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

    auto frame_it = frame->begin();
    uint8_t version{0}, subsystem_id{0}, contrib_id{0}, sentinel{0};
    rogue::interfaces::stream::fromFrame(frame_it, 1, &version);
    rogue::interfaces::stream::fromFrame(frame_it, 1, &subsystem_id);
    rogue::interfaces::stream::fromFrame(frame_it, 1, &contrib_id);
    rogue::interfaces::stream::fromFrame(frame_it, 1, &sentinel);

    // 5 = ECal, 7 = HCal in the 2026 merger config
    if (subsystem_id != 5 and subsystem_id != 7) {
      pflib_log(debug) << "Frame belongs to non-calo subsystem "
                       << subsystem_id;
      return;
    }

    if (contrib_id_ != 0 and contrib_id != contrib_id_) {
      pflib_log(debug) << "Frame belongs to non-selected calo contributor "
                       << contrib_id;
      return;
    }

    // reset since we peaked at first word already
    frame_it = frame->begin();
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

    ep_.from(words, true, only_econd_);
    ep_.to_csv(output_, mapping_.get(), with_cm_);
  }
};

static void usage() {
  std::cout << "\n"
               " USAGE:\n"
               "  rogue-decoder [options] input_file.dat\n"
               "\n"
               " OPTIONS:\n"
               "  -h,--help    : print this help and exit\n"
               "  -o,--output  : output CSV file to dump samples into "
               "(default is input file with extension changed)\n"
               "  --n-links    : number of active links connected to the"
               " ECON-D (default 2)\n"
               "  -l,--log     : logging level to printout (-1: trace up to 4: "
               "fatal)\n"
               "  -c,--contrib : which contributor to focus on (hcal or ecal)\n"
               "  --econ       : only decode sample packets from this ECON-D ID"
               " (e.g. one layer of a multi-layer ECal frame)\n"
               "  --rocs       : comma-separated active ROCs, mapped with the"
               " ECal module or HCal backplane eRx map (from -c); sets --n-links"
               " and writes i_roc and channel 0-71\n"
               "  --cm         : also write the common-mode words as channels"
               " cm0 and cm1\n"
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

  int n_links{2};
  int contrib_id{0};
  int only_econd{-1};
  bool with_cm{false};
  std::vector<int> rocs;
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
      } else if (arg == "--n-links") {
        if (i_arg + 1 == argc or argv[i_arg + 1][0] == '-') {
          pflib_log(fatal) << "The " << arg
                           << " parameter requires an argument after it.";
          return 1;
        }
        i_arg++;
        try {
          n_links = std::stoi(argv[i_arg]);
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
          contrib_id = 20;
        } else if (contrib_name == "ecal") {
          contrib_id = 40;
        } else {
          pflib_log(fatal) << "unrecognized contributor name " << contrib_name
                           << " should be 'hcal' or 'ecal'";
          return 1;
        }
      } else if (arg == "--econ" or arg == "--rocs") {
        if (i_arg + 1 == argc or argv[i_arg + 1][0] == '-') {
          pflib_log(fatal) << "The " << arg
                           << " parameter requires an argument after it.";
          return 1;
        }
        i_arg++;
        try {
          if (arg == "--econ") {
            only_econd = std::stoi(argv[i_arg]);
          } else {
            std::stringstream ss{argv[i_arg]};
            std::string roc;
            while (std::getline(ss, roc, ',')) {
              rocs.push_back(std::stoi(roc));
            }
          }
        } catch (const std::invalid_argument& e) {
          pflib_log(fatal) << "The argument to " << arg << " '" << argv[i_arg]
                           << "' is not an integer (list).";
          return 1;
        }
      } else if (arg == "--cm") {
        with_cm = true;
      } else {
        pflib_log(fatal) << "Unrecognized option " << arg;
        return 1;
      }
    } else {
      if (not in_file.empty()) {
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
    std::unique_ptr<pflib::packing::SingleECONDRocErxMapping> mapping;
    if (not rocs.empty()) {
      if (contrib_id != 20 and contrib_id != 40) {
        pflib_log(fatal) << "--rocs needs -c ecal or -c hcal for the eRx map";
        return 1;
      }
      mapping = std::make_unique<pflib::packing::SingleECONDRocErxMapping>(
          contrib_id == 20 ? pflib::HcalBackplane::ROC_ERX_MAPPING_DAQ
                           : pflib::EcalModule::ROC_ERX_MAPPING_DAQ,
          rocs);
      n_links = 2 * rocs.size();
    }
    auto writer = std::make_shared<CaloCSVWriter>(
        out_file, n_links, contrib_id, only_econd, with_cm, std::move(mapping));
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
