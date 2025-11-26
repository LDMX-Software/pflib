#include "noinv_vref_scan.h"

#include <nlohmann/json.hpp>

#include "../daq_run.h"

ENABLE_LOGGING();

// helper function to facilitate EventPacket dependent behaviour
template <class EventPacket>
static void noinv_vref_scan_writer(Target* tgt, pflib::ROC& roc, size_t nevents,
                                   std::string& output_filepath) {
  int link = 0;
  int i_ch = 0;  // 0–35
  int noinv_vref = 0;

  DecodeAndWriteToCSV<EventPacket> writer{
      output_filepath,
      [&](std::ofstream& f) {
        nlohmann::json header;
        header["scan_type"] = "CH_#.NOINV_VREF sweep";
        header["trigger"] = "PEDESTAL";
        header["nevents_per_point"] = nevents;
        f << "# " << header << "\n"
          << "NOINV_VREF";
        for (int ch{0}; ch < 72; ch++) {
          f << ',' << ch;
        }
        f << '\n';
      },
      [&](std::ofstream& f, const EventPacket& ep) {
        f << noinv_vref;
        for (int ch{0}; ch < 72; ch++) {
          link = (ch / 36);
          i_ch = ch % 36;
          if constexpr (std::is_same_v<EventPacket,
                                       pflib::packing::SingleROCEventPacket>) {
            f << ',' << ep.channel(ch).adc();
          } else if constexpr (
              std::is_same_v<EventPacket,
                             pflib::packing::MultiSampleECONDEventPacket>) {
            f << ',' << ep.samples[ep.i_soi].channel(link, i_ch).adc();
          }
        }
        f << '\n';
      }};

  tgt->setup_run(1 /* dummy - not stored */, pftool::state.daq_format_mode,
                 1 /* dummy */);

  // increment inv_vref in increments of 20. 10 bit value but only scanning to
  // 600
  for (noinv_vref = 0; noinv_vref <= 600; noinv_vref += 20) {
    pflib_log(info) << "Running INV_VREF = " << noinv_vref;
    // set noinv_vref simultaneously for both links
    auto test_param = roc.testParameters()
                          .add("REFERENCEVOLTAGE_0", "INV_VREF", noinv_vref)
                          .add("REFERENCEVOLTAGE_1", "INV_VREF", noinv_vref)
                          .apply();
    // store current scan state in header for writer access
    daq_run(tgt, "PEDESTAL", writer, nevents, pftool::state.daq_rate);
  }
}

void noinv_vref_scan(Target* tgt) {
  int nevents = pftool::readline_int("Number of events per point: ", 1);
  std::string output_filepath = pftool::readline_path("inv_vref_scan", ".csv");
  auto roc = tgt->roc(pftool::state.iroc);

  if (pftool::state.daq_format_mode == Target::DaqFormat::SIMPLEROC) {
    noinv_vref_scan_writer<pflib::packing::SingleROCEventPacket>(
        tgt, roc, nevents, output_filepath);
  } else if (pftool::state.daq_format_mode ==
             Target::DaqFormat::ECOND_SW_HEADERS) {
    noinv_vref_scan_writer<pflib::packing::MultiSampleECONDEventPacket>(
        tgt, roc, nevents, output_filepath);
  }

  // DecodeAndWriteToCSV writer{
  //     output_filepath,
  //     [&](std::ofstream& f) {
  //       nlohmann::json header;
  //       header["scan_type"] = "CH_#.NOINV_VREF sweep";
  //       header["trigger"] = "PEDESTAL";
  //       header["nevents_per_point"] = nevents;
  //       f << "# " << header << "\n"
  //         << "NOINV_VREF";
  //       for (int ch{0}; ch < 72; ch++) {
  //         f << ',' << ch;
  //       }
  //       f << '\n';
  //     },
  //     [&](std::ofstream& f, const pflib::packing::SingleROCEventPacket& ep) {
  //       f << noinv_vref;
  //       for (int ch{0}; ch < 72; ch++) {
  //         f << ',' << ep.channel(ch).adc();
  //       }
  //       f << '\n';
  //     }};

  // tgt->setup_run(1 /* dummy */, Target::DaqFormat::SIMPLEROC, 1 /* dummy */);

  // // set global params HZ_noinv on each link (arbitrary channel on each)
  // auto test_param = roc.testParameters()
  //                       .add("CH_17", "HZ_INV", 1)
  //                       .add("CH_53", "HZ_INV", 1)
  //                       .apply();

  // // increment inv_vref in increments of 20. 10 bit value but only scanning
  // to
  // // 600
  // for (noinv_vref = 0; noinv_vref <= 600; noinv_vref += 20) {
  //   pflib_log(info) << "Running NOINV_VREF = " << noinv_vref;
  //   // set noinv_vref simultaneously for both links
  //   auto test_param = roc.testParameters()
  //                         .add("REFERENCEVOLTAGE_0", "NOINV_VREF",
  //                         noinv_vref) .add("REFERENCEVOLTAGE_1",
  //                         "NOINV_VREF", noinv_vref) .apply();
  //   // store current scan state in header for writer access
  //   daq_run(tgt, "PEDESTAL", writer, nevents, pftool::state.daq_rate);
  // }
}
