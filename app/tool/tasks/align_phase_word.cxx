#include "align_phase_word.h"

#include "pflib/DecodeAndWrite.h"
#include "pflib/utility/json.h"

ENABLE_LOGGING();

void align_phase_word(Target* tgt) {
  auto roc = tgt->hcal().roc(pftool::state.iroc, pftool::state.type_version());
  auto econ = tgt->hcal().econ(pftool::state.iecon, pftool::state.type_version())


  // int calib = 0;

  // pflib::DecodeAndWriteToCSV writer{
  //     output_filepath,
  //     [&](std::ofstream& f) {
  //       boost::json::object header;
  //       header["scan_type"] = "CH_#.TOA sweep";
  //       header["trigger"] = "CHARGE";
  //       header["nevents_per_point"] = nevents;
  //       f << "# " << boost::json::serialize(header) << "\n"
  //         << "CALIB";
  //       for (int ch{0}; ch < 72; ch++) {
  //         f << "," << ch;
  //       }
  //       f << "\n";
  //     },
  //     [&](std::ofstream& f, const pflib::packing::SingleROCEventPacket& ep) {
  //       f << calib;
  //       // Write the TOA values for each channel
  //       for (int ch{0}; ch < 72; ch++) {
  //         f << "," << ep.channel(ch).toa();
  //       }
  //       f << "\n";
  //     }};



  int IDLE = 89478485;  // hardcode based on phase alignment script

  // Do I need one of these for the ECON?
  tgt->setup_run(1 /* dummy */, Target::DaqFormat::SIMPLEROC, 1 /* dummy */);


  // Is this how I set parameters appropriately?
  auto roc_setup_builder = roc.testParameters()
                           .add("DIGITALHALF_0", "IDLEFRAME", IDLE);

  auto roc_setup_test = roc_setup_builder.apply();

  auto econ_setup_builder = econ.testParameters()
                           .add("DIGITALHALF_0", "IDLEFRAME", IDLE);

  auto econ_setup_test = econ_setup_builder.apply();

  // Do I need one of these for the ECON?
  tgt->daq_run("CHARGE", writer, nevents, pftool::state.daq_rate);



  // for (int ch{0}; ch < 72; ch++) {
  //   setup_builder.add("CH_" + std::to_string(ch), "LOWRANGE", 1);
  // }
  // auto setup_test = setup_builder.apply();

  // for (calib = 0; calib < 800; calib += 4) {
  //   pflib_log(info) << "Running CALIB = " << calib;
  //   // Set the CALIB parameters for both halves
  //   auto calib_test = roc.testParameters()
  //                         .add("REFERENCEVOLTAGE_0", "CALIB", calib)
  //                         .add("REFERENCEVOLTAGE_1", "CALIB", calib)
  //                         .apply();
  //   tgt->daq_run("CHARGE", writer, nevents, pftool::state.daq_rate);
  // }


}
