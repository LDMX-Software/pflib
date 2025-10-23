#include "align_phase_word.h"

#include "pflib/DecodeAndWrite.h"
#include "pflib/utility/json.h"

ENABLE_LOGGING();

void align_phase_word(Target* tgt) {
  auto roc = tgt->hcal().roc(pftool::state.iroc); //, pftool::state.type_version());
  auto econ = tgt->hcal().econ(pftool::state.iecon); //, pftool::state.type_version());


// Do I need something like this implemented?

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


// -------------------------------------------------------------------------------------------------------------------------------- //
// ----------------------------------------------------- PHASE ALIGNMENT ----------------------------------------------------- //
  int IDLE = 89478485;  // == 5555555. hardcode based on phase alignment script

  // Do I need one of these for the ECON? What does this do?
  tgt->setup_run(1 /* dummy */, Target::DaqFormat::ECOND_NO_ZS, 2 /* dummy */);
  tgt->setup_run(1 /* dummy */, Target::DaqFormat::SIMPLEROC, 1 /* dummy */);



  // ---------------------------------- SETTING ROC REGISTERS ---------------------------------- //
  // Is this how I set ROC parameters appropriately?
  // --ROC. do these test parameters correspond to e.g. before_state_roc.yaml?
  auto roc_setup_builder = roc.testParameters()
                           .add("DIGITALHALF_0", "IDLEFRAME", IDLE)
                           .add("DIGITALHALF_1", "IDLEFRAME", IDLE);
  auto roc_setup_test = roc_setup_builder.apply();

  // ---------------------------------- --------------------------------------------------------- //


  // ---------------------------------- SETTING ECON REGISTERS ---------------------------------- //
  // -- ECON do these test parameters correspond to e.g. before_state.yaml?
  auto econ_setup_builder = econ.testParameters()
                           .add("CLOCKSANDRESETS", "GLOBAL_PUSM_RUN", 0) // set run bit 0 while configuring
                          //  .add("", "", );
 
  // I can manually apply them, eg:
  // set run bit 0 while configuring
  // econ.testParameters().add("CLOCKSANDRESETS_GLOBAL", "PUSM_RUN", 0).apply();


  //Phase ON
                          .add("EPRXGRPTOP", "GLOBAL_TRACK_MODE", 1)  // corresponding to configs/train_erx_phase_ON_econ.yaml
                          .add("CHEPRXGRP", "0_TRAIN_CHANNEL", 1) // corresponding to configs/train_erx_phase_TRAIN_econ.yaml
                          .add("CHEPRXGRP", "1_TRAIN_CHANNEL", 1)  
                          .add("CHEPRXGRP", "2_TRAIN_CHANNEL", 1)  
                          .add("CHEPRXGRP", "3_TRAIN_CHANNEL", 1)  
                          .add("CHEPRXGRP", "4_TRAIN_CHANNEL", 1)  
                          .add("CHEPRXGRP", "5_TRAIN_CHANNEL", 1)  
                          .add("CHEPRXGRP", "6_TRAIN_CHANNEL", 1)  
                          .add("CHEPRXGRP", "7_TRAIN_CHANNEL", 1)  
                          .add("CHEPRXGRP", "8_TRAIN_CHANNEL", 1)  
                          .add("CHEPRXGRP", "9_TRAIN_CHANNEL", 1)  
                          .add("CHEPRXGRP", "10_TRAIN_CHANNEL", 1)  
                          .add("CHEPRXGRP", "11_TRAIN_CHANNEL", 1)  

  // Set run bit ON
                          .add("CLOCKSANDRESETS", "GLOBAL_PUSM_RUN", 1)

  // Set run bit OFF
                          .add("CLOCKSANDRESETS", "GLOBAL_PUSM_RUN", 0)

  // Set Training OFF
                          .add("CHEPRXGRP", "0_TRAIN_CHANNEL", 0) // corresponding to configs/train_erx_phase_OFF_econ.yaml
                          .add("CHEPRXGRP", "1_TRAIN_CHANNEL", 0)  
                          .add("CHEPRXGRP", "2_TRAIN_CHANNEL", 0)  
                          .add("CHEPRXGRP", "3_TRAIN_CHANNEL", 0)  
                          .add("CHEPRXGRP", "4_TRAIN_CHANNEL", 0)  
                          .add("CHEPRXGRP", "5_TRAIN_CHANNEL", 0)  
                          .add("CHEPRXGRP", "6_TRAIN_CHANNEL", 0)  
                          .add("CHEPRXGRP", "7_TRAIN_CHANNEL", 0)  
                          .add("CHEPRXGRP", "8_TRAIN_CHANNEL", 0)  
                          .add("CHEPRXGRP", "9_TRAIN_CHANNEL", 0)  
                          .add("CHEPRXGRP", "10_TRAIN_CHANNEL", 0)  
                          .add("CHEPRXGRP", "11_TRAIN_CHANNEL", 0) 

  // Set run bit ON
                          .add("CLOCKSANDRESETS", "GLOBAL_PUSM_RUN", 1)

  // Set Training OFF
                          .add("CHEPRXGRP", "0_CHANNEL_LOCKED", 1) // corresponding to configs/check_erx_current_channel_locked_econ$ECON.yaml
                          .add("CHEPRXGRP", "1_CHANNEL_LOCKED", 1)  
                          .add("CHEPRXGRP", "2_CHANNEL_LOCKED", 1)  
                          .add("CHEPRXGRP", "3_CHANNEL_LOCKED", 1)  
                          .add("CHEPRXGRP", "4_CHANNEL_LOCKED", 1)  
                          .add("CHEPRXGRP", "5_CHANNEL_LOCKED", 1)  
                          .add("CHEPRXGRP", "6_CHANNEL_LOCKED", 1)  
                          .add("CHEPRXGRP", "7_CHANNEL_LOCKED", 1)  
                          .add("CHEPRXGRP", "8_CHANNEL_LOCKED", 1)  
                          .add("CHEPRXGRP", "9_CHANNEL_LOCKED", 1)  
                          .add("CHEPRXGRP", "10_CHANNEL_LOCKED", 1)  
                          .add("CHEPRXGRP", "11_CHANNEL_LOCKED", 1)  
                          ;
  auto econ_setup_test = econ_setup_builder.apply();
  // -------------------------------------------------------------------------------------------- //
  

  // ---------------------------------- READING ECON REGISTERS ---------------------------------- //

  // Read PUSH
  // .add("CLOCKSANDRESETS_GLOBAL", "PUSM_STATE", 0)
  auto pusm_state = econ.dumpParameter("CLOCKSANDRESETS","GLOBAL_PUSM_STATE");   // dump Parameter I added to force this functionality to be able to assign to an output variable
  
  auto eprxgrptop = econ.dumpParameter("EPRXGRPTOP","GLOBAL_TRACK_MODE");  
  auto eprxgrptop = econ.dumpParameter("EPRXGRPTOP","GLOBAL_TRACK_MODE");  
  


  std::cout << "PUSM_STATE = " << pusm_state << std::endl ;
  //" (0x" << std::hex << pusm_state << std::dec << ")\n";

  // Check phase_select
  auto phase_sel_0 = econ.dumpParameter("CHEPRXGRP", "0_CHANNEL_LOCKED");     // can also readParameters()
  auto phase_sel_1 = econ.dumpParameter("CHEPRXGRP", "1_CHANNEL_LOCKED"); 
  auto phase_sel_2 = econ.dumpParameter("CHEPRXGRP", "2_CHANNEL_LOCKED"); 
  auto phase_sel_3 = econ.dumpParameter("CHEPRXGRP", "3_CHANNEL_LOCKED"); 
  auto phase_sel_4 = econ.dumpParameter("CHEPRXGRP", "4_CHANNEL_LOCKED"); 
  auto phase_sel_5 = econ.dumpParameter("CHEPRXGRP", "5_CHANNEL_LOCKED"); 

  std::cout << "phase select 0 = " << phase_sel_0 << std::endl ;
  std::cout << "phase select 1 = " << phase_sel_1 << std::endl ;
  std::cout << "phase select 2 = " << phase_sel_2 << std::endl ;
  std::cout << "phase select 3 = " << phase_sel_3 << std::endl ;
  std::cout << "phase select 4 = " << phase_sel_4 << std::endl ;
  std::cout << "phase select 5 = " << phase_sel_5 << std::endl ;
  
  // .add("CHEPRXGRP_00", "CHANNEL_LOCKED", 1) // corresponding to configs/check_erx_current_channel_locked_econ$ECON.yaml
  // .add("CHEPRXGRP_01", "CHANNEL_LOCKED", 1) 
  // .add("CHEPRXGRP_02", "CHANNEL_LOCKED", 1) 
  // .add("CHEPRXGRP_03", "CHANNEL_LOCKED", 1) 
  // .add("CHEPRXGRP_04", "CHANNEL_LOCKED", 1) 
  // .add("CHEPRXGRP_05", "CHANNEL_LOCKED", 1) 
// --------------------------------------------------------------------- //

// ----------------------------------------------------- END PHASE ALIGNMENT ----------------------------------------------------- //
// -------------------------------------------------------------------------------------------------------------------------------- //







// // -------------------------------------------------------------------------------------------------------------------------------- //
// // ----------------------------------------------------- WORD ALIGNMENT ----------------------------------------------------- //

// int INVERT_FCMD = 1; // Should this be 1 or 0? (See Econ align.sh)

//   // ---------------------------------- SETTING ROC REGISTERS to configure ROC ---------------------------------- //
//   roc_setup_builder = roc.testParameters()
//                            .add("DIGITALHALF_0", "IDLEFRAME", IDLE)
//                            .add("DIGITALHALF_1", "IDLEFRAME", IDLE)
//                           //  .add("TOP_0", "IN_INV_CMD_RX", INVERT_FCMD);   /// not needed
//   roc_setup_test = roc_setup_builder.apply();    
//       // $BASE_CMD_ROC -l trg${SIDE} -r roc${IROC}_${SIDE}${MOD} -w 1 -p DigitalHalf.0.IdleFrame -v $IDLE
//       //     $BASE_CMD_ROC -l trg${SIDE} -r roc${IROC}_${SIDE}${MOD} -w 1 -p DigitalHalf.1.IdleFrame -v $IDLE

//   // ------------------------------------------------------------------------------------------------------------ //




//   // ---------------------------------- READING ROC REGISTERS ---------------------------------- //
//   auto params = roc.getParameters("TOP_0"); // this uses the page and returns a mapping of all params therein
//   auto inv_fc_rx = params.find("IN_INV_CMD_RX")->second;  // second because its a key value pair mapping (See ROC.cxx)

//   params = roc.getParameters("TOP_0"); 
//   auto RunL = params.find("RUNL")->second;

//   std::cout << "inv_fc_rx = " << inv_fc_rx << std::endl;

//   // ---------------------------------- --------------------------------------------------------- //




//   // ---------------------------------- SETTING ECON REGISTERS ---------------------------------- //
// // e.g.   econ${ECON}_init_cpp.yaml (from econ_align.sh)
//   // ---------------------------------- --------------------------------------------------------- //


//   // ---------------------------------- READING ECON REGISTERS ---------------------------------- //
//       // read back econ_align.sh registers.
//       // read_snaphot.yaml
//       // check_econd_roc_alignment.yaml
//   // ---------------------------------- --------------------------------------------------------- //

// // see linkreset_rocs()
// // econ version exists, 

// // ----------------------------------------------------- END WORD ALIGNMENT ----------------------------------------------------- //
// // -------------------------------------------------------------------------------------------------------------------------------- //











  // // Do I need one of these for the ECON?
  // tgt->daq_run("CHARGE", writer, nevents, pftool::state.daq_rate);



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
