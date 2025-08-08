#include "pflib/algorithm/trim_toa_scan.h"

#include "pflib/utility/string_format.h"
#include "pflib/utility/efficiency.h"

#include "pflib/DecodeAndBuffer.h"

/**
 */
void trim_toa_scan(Target* tgt) {
  int nevents = pftool::readline_int("Number of events per point: ", 100);

  std::string output_filepath = pftool::readline_path("trim_toa_scan", ".csv");

  auto roc = tgt->hcal().roc(pftool::state.iroc, pftool::state.type_version());

  int trim_toa = 0;
  int calib = 0;

  pflib::DecodeAndWriteToCSV writer{
    output_filepath,
    [&](std::ofstream& f) {
      boost::json::object header;
      header["scan_type"] = "CH_#.TRIM_TOA sweep";
      header["trigger"] = "CHARGE";
      header["nevents_per_point"] = nevents;
      f << "# " << boost::json::serialize(header) << "\n"
        << "TRIM_TOA" << "," << "CALIB";
      for (int ch{0}; ch < 72; ch++) {
        f << "," << ch;
      }
      f << "\n";
    },
    [&](std::ofstream& f, const pflib::packing::SingleROCEventPacket &ep) {
      f << trim_toa << "," << calib;
      // Write the TOA values for each channel
      for (int ch{0}; ch < 72; ch++) {
        f << "," << ep.channel(ch).toa();
      }
      f << "\n";
    }
  };

  tgt->setup_run(1 /* dummy */, Target::DaqFormat::SIMPLEROC, 1 /* dummy */);

  // Take a charge injection run at each trim_toa. Do post-CC lowrange
  // and test all channels at a same time. Trim_toa has a range of 6 b.
  // And vary calib, from 0 to 800. Want to see how toa_efficiency
  // changes with trim_toa and calib. More details in the ana script.
  auto setup_builder = roc.testParameters()
    .add("REFERENCEVOLTAGE_0", "CALIB", calib)
    .add("REFERENCEVOLTAGE_1", "CALIB", calib)
    .add("REFERENCEVOLTAGE_0", "INTCTEST", 1)
    .add("REFERENCEVOLTAGE_1", "INTCTEST", 1);
  for (int ch{0}; ch < 72; ch++) {
    setup_builder.add("CH_"+std::to_string(ch), "LOWRANGE", 1);
  }
  auto setup_test = setup_builder.apply();

  for (trim_toa = 0; trim_toa < 32; trim_toa += 4) {
    pflib_log(info) << "Running TRIM_TOA = " << trim_toa;
    auto trim_toa_test_builder = roc.testParameters();
    for (int ch{0}; ch < 72; ch++) {
      trim_toa_test_builder.add("CH_"+std::to_string(ch), "TRIM_TOA", trim_toa);
    }
    // set TRIM_TOA for each channel
    auto trim_toa_test = trim_toa_test_builder.apply();
    for (calib = 0; calib < 800; calib += 4) {
      pflib_log(info) << "Running CALIB = " << calib;
      // Set the CALIB parameters for both halves
      auto calib_test = roc.testParameters()
        .add("REFERENCEVOLTAGE_0", "CALIB", calib)
        .add("REFERENCEVOLTAGE_1", "CALIB", calib)
        .apply();
      tgt->daq_run("CHARGE", writer, nevents, pftool::state.daq_rate);
    }
  }
}
