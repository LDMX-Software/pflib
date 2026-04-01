#include "noinv_vref_scan.h"

#include <nlohmann/json.hpp>

#include "../daq_run.h"

ENABLE_LOGGING();

void noinv_vref_scan(Target* tgt) {
  int nevents = pftool::readline_int("Number of events per point: ", 1);
  std::string output_filepath = pftool::readline_path("inv_vref_scan", ".csv");
  auto roc = tgt->roc(pftool::state.iroc);
  int noinv_vref = 0;
  int n_links = 2*tgt->nrocs();
  DecodeAndWriteToCSV writer{
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
      [&](std::ofstream& f, const pflib::packing::MultiSampleECONDEventPacket& ep) {
        f << noinv_vref;
        for (int ch{0}; ch < 72; ch++) {
          // TODO 348
          f << ',' << ep.samples[ep.i_soi].channel(ch / 36, ch % 36).adc();
        }
        f << '\n';
      },
      n_links};

  tgt->setup_run(1, Target::DaqFormat::ECOND_SW_HEADERS, 1);

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
