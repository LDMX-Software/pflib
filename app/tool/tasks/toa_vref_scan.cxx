#include "toa_vref_scan.h"

#include "pflib/utility/json.h"
#include "pflib/DecodeAndWrite.h"

ENABLE_LOGGING();

void toa_vref_scan(Target* tgt) {
  int nevents = pftool::readline_int("Number of events per point: ", 100);

  std::string output_filepath = pftool::readline_path("toa_vref_scan", ".csv");

  auto roc = tgt->hcal().roc(pftool::state.iroc, pftool::state.type_version());

  int toa_vref = 0;

  pflib::DecodeAndWriteToCSV writer{
    output_filepath,
    [&](std::ofstream& f) {
      boost::json::object header;
      header["scan_type"] = "CH_#.TOA_VREF sweep";
      header["trigger"] = "PEDESTAL";
      header["nevents_per_point"] = nevents;
      f << "# " << boost::json::serialize(header) << "\n"
        << "TOA_VREF";
      for (int ch{0}; ch < 72; ch++) {
        f << "," << ch;
      }
      f << "\n";
    },
    [&](std::ofstream& f, const pflib::packing::SingleROCEventPacket &ep) {
      f << toa_vref;
      for (int ch{0}; ch < 72; ch++) {
        f << "," << ep.channel(ch).toa();
      }
      f << "\n";
    }
  };

  tgt->setup_run(1 /* dummy */, Target::DaqFormat::SIMPLEROC, 1 /* dummy */);

  // Take a pedestal run on each parameter point
  // Toa_vref has a 10 b range, but we're not going to need all of that
  // since the pedestal variations are likely lower than ~300 adc.
  for (toa_vref = 0; toa_vref < 256; toa_vref++) {
    pflib_log(info) << "Running TOA_VREF = " << toa_vref;
    auto toa_vref_test = roc.testParameters()
      .add("REFERENCEVOLTAGE_0", "TOA_VREF", toa_vref)
      .add("REFERENCEVOLTAGE_1", "TOA_VREF", toa_vref)
      .apply();
    tgt->daq_run("PEDESTAL", writer, nevents, pftool::state.daq_rate);
  }
}
