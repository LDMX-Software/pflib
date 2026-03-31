#include <nlohmann/json.hpp>

#include "../daq_run.h"
#include "charge_timescan.h"
#include "pflib/utility/string_format.h"

ENABLE_LOGGING();

void vref_2d_scan(Target* tgt) {
  int nevents = pftool::readline_int("How many events per time point? ", 100);
  int stepsize =
      pftool::readline_int("How big stepsize between vref values? ", 20);
  pflib::ROC roc{tgt->roc(pftool::state.iroc)};
  std::string fname;
  fname = pftool::readline_path("vref_2d_scan", ".csv");
  int n_links = 2*tgt->nrocs();

  int ch = 0;
  int inv_vref = 0;
  int noinv_vref = 0;

  DecodeAndWriteToCSV writer{
      fname,
      [&](std::ofstream& f) {
        nlohmann::json header;
        f << "noinv_vref,inv_vref,ch," << pflib::packing::Sample::to_csv_header
          << '\n';
      },
      [&](std::ofstream& f, const pflib::packing::MultiSampleECONDEventPacket& ep) {
        for (ch = 0; ch < 72; ch++) {
          // TODO 348
          int link = (ch / 36);
          f << noinv_vref << ',' << inv_vref << ',' << ch << ',';
          ep.samples[ep.i_soi].channel(link, ch).to_csv(f);
          f << '\n';
        }
      },
      n_links};

  tgt->setup_run(1, Target::DaqFormat::ECOND_SW_HEADERS, 1);

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
      pflib_log(info) << "NOINV_VREF = " << noinv_vref
                      << ", INV_VREF = " << inv_vref;
      daq_run(tgt, "PEDESTAL", writer, nevents, pftool::state.daq_rate);
    }
  }
}
