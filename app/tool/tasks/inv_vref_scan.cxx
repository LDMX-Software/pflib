#include "inv_vref_scan.h"

#include <nlohmann/json.hpp>

#include "../daq_run.h"
#include "../econ_links.h"

ENABLE_LOGGING();

// helper function to facilitate EventPacket dependent behaviour
template <class EventPacket>
static void inv_vref_scan_writer(Target* tgt, pflib::ROC& roc, size_t nevents,
                                 std::string& output_filepath,
                                 std::array<int, 2>& channels, int& inv_vref) {
  int link = 0;
  int i_ch = 0;  // 0–35
  int n_links = 2;
  if constexpr (std::is_same_v<EventPacket,
                               pflib::packing::MultiSampleECONDEventPacket>) {
    n_links = determine_n_links(tgt);
  }
  DecodeAndWriteToCSV<EventPacket> writer{
      output_filepath,
      [&](std::ofstream& f) {
        nlohmann::json header;
        header["scan_type"] = "CH_#.INV_VREF sweep";
        header["trigger"] = "PEDESTAL";
        header["nevents_per_point"] = nevents;
        f << "# " << header << "\n"
          << "INV_VREF";
        for (int ch : channels) {
          f << ',' << ch;
        }
        f << '\n';
      },
      [&](std::ofstream& f, const EventPacket& ep) {
        f << inv_vref;
        for (int ch : channels) {
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
      },
      n_links};

  tgt->setup_run(1 /* dummy - not stored */, pftool::state.daq_format_mode,
                 1 /* dummy */);

  // increment inv_vref in increments of 20. 10 bit value but only scanning to
  // 600
  for (inv_vref = 0; inv_vref <= 1024; inv_vref += 20) {
    pflib_log(info) << "Running INV_VREF = " << inv_vref;
    // set inv_vref simultaneously for both links
    auto test_param = roc.testParameters()
                          .add("REFERENCEVOLTAGE_0", "INV_VREF", inv_vref)
                          .add("REFERENCEVOLTAGE_1", "INV_VREF", inv_vref)
                          .apply();
    // store current scan state in header for writer access
    daq_run(tgt, "PEDESTAL", writer, nevents, pftool::state.daq_rate);
  }
}

void inv_vref_scan(Target* tgt) {
  int nevents = pftool::readline_int("Number of events per point: ", 1);
  int inv_vref = 0;

  std::string output_filepath = pftool::readline_path("inv_vref_scan", ".csv");
  int ch_link0 = pftool::readline_int("Channel to scan on link 0", 17);
  int ch_link1 = pftool::readline_int("Channel to scan on link 1", 51);
  std::array<int, 2> channels = {ch_link0, ch_link1};

  auto roc = tgt->roc(pftool::state.iroc);

  if (pftool::state.daq_format_mode == Target::DaqFormat::SIMPLEROC) {
    inv_vref_scan_writer<pflib::packing::SingleROCEventPacket>(
        tgt, roc, nevents, output_filepath, channels, inv_vref);
  } else if (pftool::state.daq_format_mode ==
             Target::DaqFormat::ECOND_SW_HEADERS) {
    inv_vref_scan_writer<pflib::packing::MultiSampleECONDEventPacket>(
        tgt, roc, nevents, output_filepath, channels, inv_vref);
  }

  // DecodeAndWriteToCSV writer{
  //     output_filepath,
  //     [&](std::ofstream& f) {
  //       nlohmann::json header;
  //       header["scan_type"] = "CH_#.INV_VREF sweep";
  //       header["trigger"] = "PEDESTAL";
  //       header["nevents_per_point"] = nevents;
  //       f << "# " << header << "\n"
  //         << "INV_VREF";
  //       for (int ch : channels) {
  //         f << ',' << ch;
  //       }
  //       f << '\n';
  //     },
  //     [&](std::ofstream& f, const pflib::packing::SingleROCEventPacket& ep) {
  //       f << inv_vref;
  //       for (int ch : channels) {
  //         f << ',' << ep.channel(ch).adc();
  //       }
  //       f << '\n';
  //     }};

  // tgt->setup_run(1 /* dummy */, Target::DaqFormat::SIMPLEROC, 1 /* dummy */);

  // // increment inv_vref in increments of 20. 10 bit value but only scanning
  // to
  // // 600
  // for (inv_vref = 0; inv_vref <= 600; inv_vref += 20) {
  //   pflib_log(info) << "Running INV_VREF = " << inv_vref;
  //   // set inv_vref simultaneously for both links
  //   auto test_param = roc.testParameters()
  //                         .add("REFERENCEVOLTAGE_0", "INV_VREF", inv_vref)
  //                         .add("REFERENCEVOLTAGE_1", "INV_VREF", inv_vref)
  //                         .apply();
  //   // store current scan state in header for writer access
  //   daq_run(tgt, "PEDESTAL", writer, nevents, pftool::state.daq_rate);
  // }
}
