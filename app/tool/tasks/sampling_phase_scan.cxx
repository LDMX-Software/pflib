#include "sampling_phase_scan.h"

#include <nlohmann/json.hpp>

#include "../daq_run.h"

ENABLE_LOGGING();

void sampling_phase_scan(Target* tgt) {
  int nevents = pftool::readline_int("How many events per time point? ", 100);
  std::string fname = pftool::readline_path("sampling-phase-scan", ".csv");
  auto roc = tgt->roc(pftool::state.iroc);
  // TODO 348
  int link = 0;
  int i_ch = 0;
  int phase_ck = 0;
  int n_links = 2*tgt->nrocs();
  DecodeAndWriteToCSV writer{
      fname,  // output file name
      [&](std::ofstream& f) {
        nlohmann::json header;
        header["scan_type"] = "CH_#.PHASE_CK sweep";
        header["trigger"] = "PEDESTAL";
        header["nevents_per_point"] = nevents;
        f << "#" << header << "\n"
          << "PHASE_CK";
        for (int ch{0}; ch < 72; ch++) {
          f << "," << ch;
        }
        f << "\n";
      },
      [&](std::ofstream& f, const pflib::packing::MultiSampleECONDEventPacket& ep) {
        f << phase_ck;
        for (int ch{0}; ch < 72; ch++) {
          link = (ch / 36);
          i_ch = ch % 36;
          f << ',' << ep.samples[ep.i_soi].channel(link, i_ch).adc();
        }
        f << "\n";
      },
      n_links};

  tgt->setup_run(1, Target::DaqFormat::ECOND_SW_HEADERS, 1);

  // Loop over phases and do pedestals
  for (phase_ck = 0; phase_ck < 16; phase_ck++) {
    pflib_log(info) << "Scanning phase_ck = " << phase_ck;
    auto phase_test_handle =
        roc.testParameters().add("TOP", "PHASE_CK", phase_ck).apply();
    daq_run(tgt, "PEDESTAL", writer, nevents, pftool::state.daq_rate);
  }
}
