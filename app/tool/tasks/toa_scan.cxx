#include "toa_scan.h"

#include <nlohmann/json.hpp>

#include "../daq_run.h"
#include "../econ_links.h"

ENABLE_LOGGING();

template <class EventPacket>
static void toa_scan_writer(Target* tgt, pflib::ROC& roc, size_t nevents,
                            std::string& output_filepath) {
  int calib = 0;
  int n_links = 2;
  if constexpr (std::is_same_v<
                    EventPacket,
                    pflib::packing::MultiSampleECONDEventPacket>) {
    n_links = determine_n_links(tgt);
  }
  DecodeAndWriteToCSV<EventPacket> writer{
      output_filepath,
      [&](std::ofstream& f) {
        nlohmann::json header;
        header["scan_type"] = "CH_#.TOA sweep";
        header["trigger"] = "CHARGE";
        header["nevents_per_point"] = nevents;
        f << "# " << header << "\n"
          << "CALIB";
        for (int ch{0}; ch < 72; ch++) {
          f << "," << ch;
        }
        f << "\n";
      },
      [&](std::ofstream& f, const EventPacket& ep) {
        f << calib;
        // Write the TOA values for each channel
        for (int ch{0}; ch < 72; ch++) {
          f << "," << ep.channel(ch).toa();
        }
        f << "\n";
      },
      n_links};

  tgt->setup_run(1 /* dummy - not stored */, pftool::state.daq_format_mode,
                 1 /* dummy */);

  // Take a charge injection run.
  auto setup_builder = roc.testParameters()
                           .add("REFERENCEVOLTAGE_0", "CALIB", calib)
                           .add("REFERENCEVOLTAGE_1", "CALIB", calib)
                           .add("REFERENCEVOLTAGE_0", "INTCTEST", 1)
                           .add("REFERENCEVOLTAGE_1", "INTCTEST", 1);
  for (int ch{0}; ch < 72; ch++) {
    setup_builder.add("CH_" + std::to_string(ch), "LOWRANGE", 1);
  }
  auto setup_test = setup_builder.apply();

  for (calib = 0; calib < 800; calib += 4) {
    pflib_log(info) << "Running CALIB = " << calib;
    // Set the CALIB parameters for both halves
    auto calib_test = roc.testParameters()
                          .add("REFERENCEVOLTAGE_0", "CALIB", calib)
                          .add("REFERENCEVOLTAGE_1", "CALIB", calib)
                          .apply();
    daq_run(tgt, "CHARGE", writer, nevents, pftool::state.daq_rate);
  }
}

void toa_scan(Target* tgt) {
  int nevents = pftool::readline_int("Number of events per point: ", 100);

  std::string output_filepath = pftool::readline_path("toa_scan", ".csv");

  auto roc = tgt->roc(pftool::state.iroc);

  // DecodeAndWriteToCSV writer{
  //     output_filepath,
  //     [&](std::ofstream& f) {
  //       nlohmann::json header;
  //       header["scan_type"] = "CH_#.TOA sweep";
  //       header["trigger"] = "CHARGE";
  //       header["nevents_per_point"] = nevents;
  //       f << "# " << header << "\n"
  //         << "CALIB";
  //       for (int ch{0}; ch < 72; ch++) {
  //         f << "," << ch;
  //       }
  //       f << "\n";
  //     },
  //     [&](std::ofstream& f, const pflib::packing::SingleROCEventPacket& ep) {
  //       f << calib;
  //       // Write the TOA values for each channel
  //       for (int ch{0}; ch < 72; ch++) {
  //         f << "," << ep.channel(ch).toa();
  //       }
  //       f << "\n";
  //     }};

  // tgt->setup_run(1 /* dummy */, Target::DaqFormat::SIMPLEROC, 1 /* dummy */);

  // // Take a charge injection run.
  // auto setup_builder = roc.testParameters()
  //                          .add("REFERENCEVOLTAGE_0", "CALIB", calib)
  //                          .add("REFERENCEVOLTAGE_1", "CALIB", calib)
  //                          .add("REFERENCEVOLTAGE_0", "INTCTEST", 1)
  //                          .add("REFERENCEVOLTAGE_1", "INTCTEST", 1);
  // for (int ch{0}; ch < 72; ch++) {
  //   setup_builder.add("CH_" + std::to_string(ch), "LOWRANGE", 1);
  // }
  // auto setup_test = setup_builder.apply();

  // for (calib = 0; calib < 800; calib += 4) {
  //   pflib_log(info) << "Running CALIB = " << calib;
  //   // Set the CALIB parameters for both halves
  //   auto calib_test = roc.testParameters()
  //                         .add("REFERENCEVOLTAGE_0", "CALIB", calib)
  //                         .add("REFERENCEVOLTAGE_1", "CALIB", calib)
  //                         .apply();
  //   daq_run(tgt, "CHARGE", writer, nevents, pftool::state.daq_rate);
  // }
}
