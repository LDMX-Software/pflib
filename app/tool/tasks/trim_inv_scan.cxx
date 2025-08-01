#include "trim_inv_scan.h"

#include "pflib/utility/json.h"
#include "pflib/DecodeAndWrite.h"

ENABLE_LOGGING();

void trim_inv_scan(Target* tgt) {
  int nevents = pftool::readline_int("Number of events per point: ", 1);

  std::string output_filepath = pftool::readline_path("trim_inv_scan", ".csv");

  auto roc = tgt->hcal().roc(pftool::state.iroc, pftool::state.type_version());

  int trim_inv =0;

  pflib::DecodeAndWriteToCSV writer{
    output_filepath,
    [&](std::ofstream& f) {
      boost::json::object header;
      header["scan_type"] = "CH_#.TRIM_INV sweep";
      header["trigger"] = "PEDESTAL";
      header["nevents_per_point"] = nevents;
      f << "# " << boost::json::serialize(header) << "\n"
        << "TRIM_INV";
      for (int ch{0}; ch < 72; ch++) {
        f << ',' << ch;
      }
      f << '\n';
    },
    [&](std::ofstream& f, const pflib::packing::SingleROCEventPacket &ep) {
      f << trim_inv;
      for (int ch{0}; ch < 72; ch++) {
        f << ',' << ep.channel(ch).adc();
      }
      f << '\n';
    }
  };

  tgt->setup_run(1 /* dummy */, Target::DaqFormat::SIMPLEROC, 1 /* dummy */);

  //take pedestal run on each parameter point
  for (trim_inv = 0; trim_inv < 64; trim_inv += 4) {
    pflib_log(info) << "Running TRIM_INV = " << trim_inv;
    auto trim_inv_test_builder = roc.testParameters();
    for (int ch{0}; ch < 72; ch++) {
      trim_inv_test_builder.add("CH_"+std::to_string(ch), "TRIM_INV", trim_inv);
    }
    auto trim_inv_test = trim_inv_test_builder.apply();
    tgt->daq_run("PEDESTAL", writer, nevents, pftool::state.daq_rate);
    
  }
}

