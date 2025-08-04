#include "trim_inv_dacb_scan.h"

#include "pflib/utility/json.h"
#include "pflib/DecodeAndWrite.h"

ENABLE_LOGGING();

void trim_inv_dacb_scan(Target* tgt) {
  int nevents = pftool::readline_int("Number of events per point: ", 1);

  std::string output_filepath = pftool::readline_path("trim_inv_dacb_scan", ".csv");

  auto roc = tgt->hcal().roc(pftool::state.iroc, pftool::state.type_version());

  int trim_inv =0;
  int dacb = 0;

  pflib::DecodeAndWriteToCSV writer{
    output_filepath,
    [&](std::ofstream& f) {
      boost::json::object header;
      header["scan_type"] = "CH_#.TRIM_INV & CH_#.DACB sweep";
      header["trigger"] = "PEDESTAL";
      header["nevents_per_point"] = nevents;
      f << "# " << boost::json::serialize(header) << "\n"
        << "TRIM_INV,DACB";
      for (int ch{0}; ch < 72; ch++) {
        f << ',' << ch;
      }
      f << '\n';
    },
    [&](std::ofstream& f, const pflib::packing::SingleROCEventPacket &ep) {
      f << trim_inv << ',' << dacb;
      for (int ch{0}; ch < 72; ch++) {
        f << ',' << ep.channel(ch).adc();
      }
      f << '\n';
    }
  };

  tgt->setup_run(1 /* dummy */, Target::DaqFormat::SIMPLEROC, 1 /* dummy */);

  //take pedestal run for dacb parameter points (trim_inv = 0)
  auto sign_dac_builder = roc.testParameters();
  for (int ch{0}; ch < 72; ch++){
    sign_dac_builder.add("CH_" + std::to_string(ch), "SIGN_DAC", 1);
  }
  auto sign_dac_test = sign_dac_builder.apply();

  for (dacb = 0; dacb < 64; dacb += 4){
    auto dacb_test_builder = roc.testParameters();
    for (int ch{0}; ch < 72; ch++){
      dacb_test_builder.add("CH_" + std::to_string(ch), "DACB", dacb);
    }
    auto dacb_test = dacb_test_builder.apply();

    pflib_log(info) << "Running DACB = " << dacb;
    tgt->daq_run("PEDESTAL", writer, nevents, pftool::state.daq_rate);

  }

  //reset dacb to 0
  auto dacb_reset_builder = roc.testParameters();
  dacb = 0;
  for (int ch{0}; ch < 72; ch++){
      dacb_reset_builder.add("CH_" + std::to_string(ch), "DACB", dacb)
                .add("CH_" + std::to_string(ch), "SIGN_DAC", 0);
    }
  auto dacb_reset = dacb_reset_builder.apply();

  //take pedestal run for trim_inv parameter points
  for (trim_inv = 4; trim_inv < 64; trim_inv += 4) {
      pflib_log(info) << "Running TRIM_INV = " << trim_inv;
      auto trim_inv_test_builder = roc.testParameters();
      for (int ch{0}; ch < 72; ch++) {
        trim_inv_test_builder.add("CH_"+std::to_string(ch), "TRIM_INV", trim_inv);
      }
      auto trim_inv_test = trim_inv_test_builder.apply();
      tgt->daq_run("PEDESTAL", writer, nevents, pftool::state.daq_rate);
      
    }
}

