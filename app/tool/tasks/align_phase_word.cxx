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



  int IDLE = 89478485;  // == 5555555. hardcode based on phase alignment script

  // Do I need one of these for the ECON?
  tgt->setup_run(1 /* dummy */, Target::DaqFormat::ECOND_NO_ZS, 2 /* dummy */);
  tgt->setup_run(1 /* dummy */, Target::DaqFormat::SIMPLEROC, 1 /* dummy */);


  // How do I set parameters appropriately?
  // --ROC. do these test parameters correspond to e.g. before_state_roc.yaml?
  auto roc_setup_builder = roc.testParameters()
                           .add("DIGITALHALF_0", "IDLEFRAME", IDLE)
                           .add("DIGITALHALF_1", "IDLEFRAME", IDLE);
  auto roc_setup_test = roc_setup_builder.apply();


  // --ECON do these test parameters correspond to e.g. before_state.yaml?
  auto econ_setup_builder = econ.testParameters()
                           .add("CLOCKSANDRESETS_GLOBAL", "PUSM_RUN", 0) // set run bit 0 while configuring
                           .add("", "", );
 

  // -- SETTING REGISTERS -- //
  //I dont know how to set these:

  //Phase ON
  // .add("EPRXGRPTOP_GLOBAL", "TRACK_MODE", 1)  // corresponding to configs/train_erx_phase_ON_econ.yaml
  // .add("CHEPRXGRP_00", "TRAIN_CHANNEL", 1) // corresponding to configs/train_erx_phase_TRAIN_econ.yaml
  // .add("CHEPRXGRP_01", "TRAIN_CHANNEL", 1)  
  // .add("CHEPRXGRP_02", "TRAIN_CHANNEL", 1)  
  // .add("CHEPRXGRP_03", "TRAIN_CHANNEL", 1)  
  // .add("CHEPRXGRP_04", "TRAIN_CHANNEL", 1)  
  // .add("CHEPRXGRP_05", "TRAIN_CHANNEL", 1)  
  // .add("CHEPRXGRP_06", "TRAIN_CHANNEL", 1)  
  // .add("CHEPRXGRP_07", "TRAIN_CHANNEL", 1)  
  // .add("CHEPRXGRP_08", "TRAIN_CHANNEL", 1)  
  // .add("CHEPRXGRP_09", "TRAIN_CHANNEL", 1)  
  // .add("CHEPRXGRP_10", "TRAIN_CHANNEL", 1)  
  // .add("CHEPRXGRP_11", "TRAIN_CHANNEL", 1)  

  // Set run bit ON
  // .add("CLOCKSANDRESETS_GLOBAL", "PUSM_RUN", 1)

  // Set run bit OFF
  // .add("CLOCKSANDRESETS_GLOBAL", "PUSM_RUN", 0)

  // Set Training OFF
  // .add("CHEPRXGRP_00", "TRAIN_CHANNEL", 0) // corresponding to configs/train_erx_phase_OFF_econ.yaml
  // .add("CHEPRXGRP_01", "TRAIN_CHANNEL", 0) 
  // .add("CHEPRXGRP_02", "TRAIN_CHANNEL", 0) 
  // .add("CHEPRXGRP_03", "TRAIN_CHANNEL", 0) 
  // .add("CHEPRXGRP_04", "TRAIN_CHANNEL", 0) 
  // .add("CHEPRXGRP_05", "TRAIN_CHANNEL", 0) 
  // .add("CHEPRXGRP_06", "TRAIN_CHANNEL", 0) 
  // .add("CHEPRXGRP_07", "TRAIN_CHANNEL", 0) 
  // .add("CHEPRXGRP_08", "TRAIN_CHANNEL", 0) 
  // .add("CHEPRXGRP_09", "TRAIN_CHANNEL", 0) 
  // .add("CHEPRXGRP_10", "TRAIN_CHANNEL", 0) 
  // .add("CHEPRXGRP_11", "TRAIN_CHANNEL", 0) 

  // Set run bit ON
  // .add("CLOCKSANDRESETS_GLOBAL", "PUSM_RUN", 1)

  // Set Training OFF
  // .add("CHEPRXGRP_00", "CHANNEL_LOCKED", 1) // corresponding to configs/check_erx_current_channel_locked_econ$ECON.yaml
  // .add("CHEPRXGRP_01", "CHANNEL_LOCKED", 1) 
  // .add("CHEPRXGRP_02", "CHANNEL_LOCKED", 1) 
  // .add("CHEPRXGRP_03", "CHANNEL_LOCKED", 1) 
  // .add("CHEPRXGRP_04", "CHANNEL_LOCKED", 1) 
  // .add("CHEPRXGRP_05", "CHANNEL_LOCKED", 1) 
  // .add("CHEPRXGRP_06", "CHANNEL_LOCKED", 1) 
  // .add("CHEPRXGRP_07", "CHANNEL_LOCKED", 1) 
  // .add("CHEPRXGRP_08", "CHANNEL_LOCKED", 1) 
  // .add("CHEPRXGRP_09", "CHANNEL_LOCKED", 1) 
  // .add("CHEPRXGRP_10", "CHANNEL_LOCKED", 1) 
  // .add("CHEPRXGRP_11", "CHANNEL_LOCKED", 1) 
// --------------------------------------------------------------------- //

  // -- READING REGISTERS -- //

  // Read PUSH
  // .add("CLOCKSANDRESETS_GLOBAL", "PUSM_STATE", 0)

  // Check phase_select
  // .add("CHEPRXGRP_00", "CHANNEL_LOCKED", 1) // corresponding to configs/check_erx_current_channel_locked_econ$ECON.yaml
  // .add("CHEPRXGRP_01", "CHANNEL_LOCKED", 1) 
  // .add("CHEPRXGRP_02", "CHANNEL_LOCKED", 1) 
  // .add("CHEPRXGRP_03", "CHANNEL_LOCKED", 1) 
  // .add("CHEPRXGRP_04", "CHANNEL_LOCKED", 1) 
  // .add("CHEPRXGRP_05", "CHANNEL_LOCKED", 1) 
// --------------------------------------------------------------------- //






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
