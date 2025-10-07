#include "sampling_phase_scan.h"

#include "pflib/DecodeAndWrite.h"
#include "pflib/utility/json.h"

ENABLE_LOGGING();

void sampling_phase_scan(Target* tgt) {
  int nevents = pftool::readline_int("How many events per time point? ", 100);

  std::string fname = pftool::readline_path("sampling-phase-scan", ".csv");

  auto roc = tgt->hcal().roc(pftool::state.iroc, pftool::state.type_version());

  int phase_ck = 0;

  pflib::DecodeAndWriteToCSV writer{
      fname,  // output file name
      [&](std::ofstream& f) {
        boost::json::object header;
        header["scan_type"] = "CH_#.PHASE_CK sweep";
        header["trigger"] = "PEDESTAL";
        header["nevents_per_point"] = nevents;
        f << "#" << boost::json::serialize(header) << "\n"
          << "PHASE_CK";
        for (int ch{0}; ch < 72; ch++) {
          f << "," << ch;
        }
        f << "\n";
      },
      [&](std::ofstream& f, const pflib::packing::SingleROCEventPacket& ep) {
        f << phase_ck;
        for (int ch{0}; ch < 72; ch++) {
          f << "," << ep.channel(ch).adc();
        }
        f << "\n";
      }};

  tgt->setup_run(1 /* dummy - not stored */, Target::DaqFormat::SIMPLEROC,
                 1 /* dummy */);

  // Loop over phases and do pedestals
  for (phase_ck = 0; phase_ck < 16; phase_ck++) {
    pflib_log(info) << "Scanning phase_ck = " << phase_ck;
    auto phase_test_handle =
        roc.testParameters().add("TOP", "PHASE_CK", phase_ck).apply();
    tgt->daq_run("PEDESTAL", writer, nevents, pftool::state.daq_rate);
  }
}
