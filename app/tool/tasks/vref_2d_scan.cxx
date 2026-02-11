#include "charge_timescan.h"

#include <nlohmann/json.hpp>

#include "../daq_run.h"
#include "../econ_links.h"
#include "pflib/utility/string_format.h"

ENABLE_LOGGING();

template <class EventPacket>
static void vref_2d_scan_writer(Target* tgt, pflib::ROC& roc, size_t nevents,
                                   std::string fname, int stepsize) {

  int n_links = 2;
  if constexpr (std::is_same_v<EventPacket,
                               pflib::packing::MultiSampleECONDEventPacket>) {
    n_links = determine_n_links(tgt);
  }

  int ch = 0;
  int inv_vref = 0;
  int noinv_vref = 0;

  DecodeAndWriteToCSV<EventPacket> writer{
      fname,
      [&](std::ofstream& f) {
        nlohmann::json header;
        f << "noinv_vref,inv_vref,ch," << pflib::packing::Sample::to_csv_header << '\n';
      },
      [&](std::ofstream& f, const EventPacket& ep) {
        for (ch = 0; ch < 72; ch++) {
          int link = (ch / 36);
          f << noinv_vref << ',' << inv_vref << ',' << ch << ',';
          if constexpr (std::is_same_v<
                            EventPacket,
                            pflib::packing::MultiSampleECONDEventPacket>) {
            ep.samples[ep.i_soi].channel(link, ch).to_csv(f);
          } else if constexpr (std::is_same_v<
                                   EventPacket,
                                   pflib::packing::SingleROCEventPacket>) {
            ep.channel(ch).to_csv(f);
          } else {
            PFEXCEPTION_RAISE("BadConf",
                              "Unable to do all_channels_to_csv for the "
                              "currently configured format.");
          }
          f << '\n';
        }
      },
      n_links};

  tgt->setup_run(1 /* dummy - not stored */, pftool::state.daq_format_mode,
                 1 /* dummy */);

  for (noinv_vref = 0; noinv_vref < 1024; noinv_vref += stepsize) {
    auto noinv_param = roc.testParameters()
        .add("REFERENCEVOLTAGE_0", "NOINV_VREF", noinv_vref)
        .add("REFERENCEVOLTAGE_1", "NOINV_VREF", noinv_vref)
        .apply();
    for (inv_vref = 0; inv_vref < 1024; inv_vref += stepsize) {
      auto noinv_param = roc.testParameters()
          .add("REFERENCEVOLTAGE_0", "INV_VREF", inv_vref)
          .add("REFERENCEVOLTAGE_1", "INV_VREF", inv_vref)
          .apply();
      pflib_log(info) << "NOINV_VREF = " << noinv_vref << ", INV_VREF = " << inv_vref;
      daq_run(tgt, "PEDESTAL", writer, nevents, pftool::state.daq_rate);
    }
  }
}

void vref_2d_scan(Target* tgt) {
  int nevents = pftool::readline_int("How many events per time point? ", 100);
  int stepsize = pftool::readline_int("How big stepsize between vref values? ", 20);
  pflib::ROC roc{tgt->roc(pftool::state.iroc)};
  std::string fname;
  fname = pftool::readline_path("vref_2d_scan", ".csv");
  if (pftool::state.daq_format_mode == Target::DaqFormat::SIMPLEROC) {
    vref_2d_scan_writer<pflib::packing::SingleROCEventPacket>(
        tgt, roc, nevents, fname, stepsize);
  } else if (pftool::state.daq_format_mode ==
             Target::DaqFormat::ECOND_SW_HEADERS) {
    vref_2d_scan_writer<pflib::packing::MultiSampleECONDEventPacket>(
        tgt, roc, nevents, fname, stepsize);
  }
}
