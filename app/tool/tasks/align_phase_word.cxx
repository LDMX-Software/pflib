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
  { // scope this
    auto roc_setup_builder = roc.testParameters()
                            .add("DIGITALHALF_0", "IDLEFRAME", IDLE)
                            .add("DIGITALHALF_1", "IDLEFRAME", IDLE);
    auto params = roc.getParameters("DIGITALHALF_0"); // this uses the page and returns a mapping of all params therein
    auto idle_0 = params.find("IDLEFRAME")->second;  // second because its a key value pair mapping (See ROC.cxx)

    std::cout << "idle_0 = " << idle_0 << std::endl;
  }

  // ---------------------------------- --------------------------------------------------------- //


  // ---------------------------------- SETTING ECON REGISTERS ---------------------------------- //
  // -- ECON do these test parameters correspond to e.g. before_state.yaml?
  { // scope this
    std::cout << "1 test = " << std::endl ;
    auto econ_setup_builder = econ.testParameters().add("CLOCKSANDRESETS", "GLOBAL_PUSM_RUN", 0); // set run bit 0 while configuring
    auto econ_setup_test = econ_setup_builder.apply();
    auto pusm_run = econ.dumpParameter("CLOCKSANDRESETS","GLOBAL_PUSM_RUN");   // dump Parameter I added to force this functionality to be able to assign to an output variable
    auto pusm_state = econ.dumpParameter("CLOCKSANDRESETS","GLOBAL_PUSM_STATE"); 
    std::cout << "PUSM_STATE 1 = " << pusm_state << std::endl ;
    std::cout << "PUSM_RUN 1 = " << pusm_run << std::endl ;
  }


  // I can manually apply them, eg:
  // set run bit 0 while configuring
  // econ.testParameters().add("CLOCKSANDRESETS_GLOBAL", "PUSM_RUN", 0).apply();


  //Phase ON
  { // scope this
  auto econ_setup_builder = econ.testParameters()
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
                            .add("CHEPRXGRP", "11_TRAIN_CHANNEL", 1);  
    auto econ_setup_test = econ_setup_builder.apply();
    auto eprxgrptop = econ.dumpParameter("EPRXGRPTOP","GLOBAL_TRACK_MODE");
    auto cheprxgrp5 = econ.dumpParameter("CHEPRXGRP","5_TRAIN_CHANNEL");
    std::cout << "EPRXGRPTOP = " << eprxgrptop << std::endl ; 
    std::cout << "CHEPRXGRP5 = " << cheprxgrp5 << std::endl ;   // arbitrarily using 5
  }


  { // scope this
    // Set run bit ON
    auto econ_setup_builder = econ.testParameters()
                            .add("CLOCKSANDRESETS", "GLOBAL_PUSM_RUN", 1);
    auto econ_setup_test = econ_setup_builder.apply();
    auto pusm_run = econ.dumpParameter("CLOCKSANDRESETS","GLOBAL_PUSM_RUN");
    auto pusm_state = econ.dumpParameter("CLOCKSANDRESETS","GLOBAL_PUSM_STATE");
    std::cout << "PUSM_STATE = " << pusm_state << std::endl ;
    std::cout << "PUSM_RUN = " << pusm_run << std::endl ;
  }

  { // scope this
    // Set run bit OFF
    auto econ_setup_builder = econ.testParameters()
                            .add("CLOCKSANDRESETS", "GLOBAL_PUSM_RUN", 0);
    auto econ_setup_test = econ_setup_builder.apply();
    auto pusm_run = econ.dumpParameter("CLOCKSANDRESETS","GLOBAL_PUSM_RUN");
    auto pusm_state = econ.dumpParameter("CLOCKSANDRESETS","GLOBAL_PUSM_STATE");
    std::cout << "PUSM_STATE = " << pusm_state << std::endl ;
    std::cout << "PUSM_RUN = " << pusm_run << std::endl ;
  }


  { // scope this
    // Set Training OFF
    auto econ_setup_builder = econ.testParameters()
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
                            .add("CHEPRXGRP", "11_TRAIN_CHANNEL", 0); 
    auto econ_setup_test = econ_setup_builder.apply();  
    auto cheprxgrp5 = econ.dumpParameter("CHEPRXGRP","5_TRAIN_CHANNEL");
    std::cout << "CHEPRXGRP5 = " << cheprxgrp5 << std::endl ;
  }


  { // scope this
    // Set run bit ON
    auto econ_setup_builder = econ.testParameters()
                            .add("CLOCKSANDRESETS", "GLOBAL_PUSM_RUN", 1);
    auto econ_setup_test = econ_setup_builder.apply();
    auto pusm_run = econ.dumpParameter("CLOCKSANDRESETS","GLOBAL_PUSM_RUN");
    auto pusm_state = econ.dumpParameter("CLOCKSANDRESETS","GLOBAL_PUSM_STATE");
    std::cout << "PUSM_STATE = " << pusm_state << std::endl ;
    std::cout << "PUSM_RUN = " << pusm_run << std::endl ;
  }

  { // scope this
    // Set Training OFF
    auto econ_setup_builder = econ.testParameters()
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
                            .add("CHEPRXGRP", "11_CHANNEL_LOCKED", 1);
    auto econ_setup_test = econ_setup_builder.apply();  
    auto cheprxgrp5 = econ.dumpParameter("CHEPRXGRP","5_CHANNEL_LOCKED");
    std::cout << "CHEPRXGRP5 = " << cheprxgrp5 << std::endl ;  
  }
                          ;
  // auto econ_setup_test = econ_setup_builder.apply();
  // -------------------------------------------------------------------------------------------- //
  

  // ---------------------------------- READING ECON REGISTERS ---------------------------------- //

  // Read PUSH
  // .add("CLOCKSANDRESETS_GLOBAL", "PUSM_STATE", 0)
  auto pusm_run = econ.dumpParameter("CLOCKSANDRESETS","GLOBAL_PUSM_STATE");   // dump Parameter I added to force this functionality to be able to assign to an output variable
  auto pusm_state = econ.dumpParameter("CLOCKSANDRESETS","GLOBAL_PUSM_STATE");
    
  // auto eprxgrptop = econ.dumpParameter("EPRXGRPTOP","GLOBAL_TRACK_MODE");  
  std::cout << "PUSM_STATE = " << pusm_state << std::endl ;
  std::cout << "PUSM_RUN = " << pusm_run << std::endl ;
  //" (0x" << std::hex << pusm_state << std::dec << ")\n";

  // Check phase_select
  auto phase_sel_0 = econ.dumpParameter("CHEPRXGRP", "0_CHANNEL_LOCKED");     // can also readParameters()
  auto phase_sel_1 = econ.dumpParameter("CHEPRXGRP", "1_CHANNEL_LOCKED"); 
  auto phase_sel_2 = econ.dumpParameter("CHEPRXGRP", "2_CHANNEL_LOCKED"); 
  auto phase_sel_3 = econ.dumpParameter("CHEPRXGRP", "3_CHANNEL_LOCKED"); 
  auto phase_sel_4 = econ.dumpParameter("CHEPRXGRP", "4_CHANNEL_LOCKED"); 
  auto phase_sel_5 = econ.dumpParameter("CHEPRXGRP", "5_CHANNEL_LOCKED"); 
  auto phase_sel_6 = econ.dumpParameter("CHEPRXGRP", "6_CHANNEL_LOCKED"); 
  auto phase_sel_7 = econ.dumpParameter("CHEPRXGRP", "7_CHANNEL_LOCKED"); 
  auto phase_sel_8 = econ.dumpParameter("CHEPRXGRP", "8_CHANNEL_LOCKED"); 
  auto phase_sel_9 = econ.dumpParameter("CHEPRXGRP", "9_CHANNEL_LOCKED"); 
  auto phase_sel_10 = econ.dumpParameter("CHEPRXGRP", "10_CHANNEL_LOCKED"); 
  auto phase_sel_11 = econ.dumpParameter("CHEPRXGRP", "11_CHANNEL_LOCKED"); 

  std::cout << "phase select 0 = " << phase_sel_0 << std::endl ;
  std::cout << "phase select 1 = " << phase_sel_1 << std::endl ;
  std::cout << "phase select 2 = " << phase_sel_2 << std::endl ;
  std::cout << "phase select 3 = " << phase_sel_3 << std::endl ;
  std::cout << "phase select 4 = " << phase_sel_4 << std::endl ;
  std::cout << "phase select 6 = " << phase_sel_5 << std::endl ;
  std::cout << "phase select 7 = " << phase_sel_5 << std::endl ;
  std::cout << "phase select 8 = " << phase_sel_5 << std::endl ;
  std::cout << "phase select 9 = " << phase_sel_5 << std::endl ;
  std::cout << "phase select 10 = " << phase_sel_5 << std::endl ;
  std::cout << "phase select 11 = " << phase_sel_5 << std::endl ;

// --------------------------------------------------------------------- //

// ----------------------------------------------------- END PHASE ALIGNMENT ----------------------------------------------------- //
// -------------------------------------------------------------------------------------------------------------------------------- //







// // -------------------------------------------------------------------------------------------------------------------------------- //
// // ----------------------------------------------------- WORD ALIGNMENT ----------------------------------------------------- //

int INVERT_FCMD = 1; // Should this be 1 or 0? (See Econ align.sh)

//   // ---------------------------------- SETTING ROC REGISTERS to configure ROC ---------------------------------- //
{ // scope this
    auto roc_setup_builder = roc.testParameters()
                            .add("DIGITALHALF_0", "IDLEFRAME", IDLE)
                            .add("DIGITALHALF_1", "IDLEFRAME", IDLE);
    auto params = roc.getParameters("DIGITALHALF_0"); // this uses the page and returns a mapping of all params therein
    auto idle_0 = params.find("IDLEFRAME")->second;  // second because its a key value pair mapping (See ROC.cxx)

    std::cout << "idle_0 = " << idle_0 << std::endl;
}

//   // ------------------------------------------------------------------------------------------------------------ //




//   // ---------------------------------- READING ROC REGISTERS ---------------------------------- //
{ // scope this
  auto params = roc.getParameters("TOP"); // this uses the page and returns a mapping of all params therein
  auto inv_fc_rx = params.find("IN_INV_CMD_RX")->second;  // second because its a key value pair mapping (See ROC.cxx)
  std::cout << "inv_fc_rx = " << inv_fc_rx << std::endl;

  params = roc.getParameters("TOP"); 
  auto RunL = params.find("RUNL")->second;
  std::cout << "RunL = " << RunL << std::endl;

  params = roc.getParameters("TOP"); 
  auto RunR = params.find("RUNR")->second;
  std::cout << "RunR = " << RunR << std::endl;

  params = roc.getParameters("DIGITALHALF_0"); 
  auto idle_0 = params.find("IDLEFRAME")->second;
  params = roc.getParameters("DIGITALHALF_1"); 
  auto idle_1 = params.find("IDLEFRAME")->second;
  std::cout << "Idle_0 = " << idle_0 << std::endl;
  std::cout << "Idle_1 = " << idle_1 << std::endl;

  params = roc.getParameters("DIGITALHALF_0"); 
  auto bx_0 = params.find("BX_OFFSET")->second;
  params = roc.getParameters("DIGITALHALF_1"); 
  auto bx_1 = params.find("BX_OFFSET")->second;
  std::cout << "bxoffset_0 = " << bx_0 << std::endl;
  std::cout << "bxoffset_1 = " << bx_1 << std::endl;

  params = roc.getParameters("DIGITALHALF_0"); 
  auto bxtrig_0 = params.find("BX_TRIGGER")->second;
  params = roc.getParameters("DIGITALHALF_1"); 
  auto bxtrig_1 = params.find("BX_TRIGGER")->second;
  std::cout << "bxtrigger_0 = " << bxtrig_0 << std::endl;
  std::cout << "bxtrigger_1 = " << bxtrig_1 << std::endl;

}
//   // ---------------------------------- --------------------------------------------------------- //




//   // ---------------------------------- SETTING ECON REGISTERS ---------------------------------- //
  //Configure ECOND for Alignment (from econd_init_cpp.yaml)
  { // scope this
    auto econ_setup_builder = econ.testParameters()
                            .add("ROCDAQCTRL", "GLOBAL_HGCROC_HDR_MARKER", 15)
                            .add("ROCDAQCTRL", "GLOBAL_SYNC_HEADER", 1) 
                            .add("ROCDAQCTRL", "GLOBAL_SYNC_BODY", 89478485)  
                            .add("ROCDAQCTRL", "GLOBAL_ACTIVE_ERXS", 7)  
                            .add("ROCDAQCTRL", "GLOBAL_PASS_THRU_MODE", 1)  
                            .add("ROCDAQCTRL", "GLOBAL_MATCH_THRESHOLD", 2)  
                            .add("ROCDAQCTRL", "GLOBAL_SIMPLE_MODE", 1) 

                            .add("ALIGNER", "GLOBAL_ORBSYN_CNT_LOAD_VAL", 1)  
                            .add("ALIGNER", "GLOBAL_ORBSYN_CNT_SNAPSHOT", 3080)  
                            .add("ALIGNER", "GLOBAL_MATCH_PATTERN_VAL", 2505397589)       // IS THIS CORRECT (and those below _00 _01)
                            // .add("ALIGNER", "GLOBAL_MATCH_PATTERN_VAL_01", 2773833045)  
                            .add("ALIGNER", "GLOBAL_MATCH_MASK_VAL", 00)                    // is this set like this?  
                            .add("ALIGNER", "GLOBAL_MATCH_MASK_VAL", 01)  
                            .add("ALIGNER", "GLOBAL_I2C_SNAPSHOT_EN", 0)  
                            .add("ALIGNER", "GLOBAL_SNAPSHOT_EN", 1)  
                            .add("ALIGNER", "GLOBAL_ORBSYN_CNT_MAX_VAL", 3563)  
                            .add("ALIGNER", "GLOBAL_FREEZE_OUTPUT_ENABLE", 0)  
                            .add("ALIGNER", "GLOBAL_FREEZE_OUTPUT_ENABLE_ALL_CHANNELS_LOCKED", 0)

                            .add("CHALIGNER", "0_PER_CH_ALIGN_EN", 1)
                            .add("CHALIGNER", "1_PER_CH_ALIGN_EN", 1)
                            .add("CHALIGNER", "2_PER_CH_ALIGN_EN", 1)
                            .add("CHALIGNER", "3_PER_CH_ALIGN_EN", 1)
                            .add("CHALIGNER", "4_PER_CH_ALIGN_EN", 1)
                            .add("CHALIGNER", "5_PER_CH_ALIGN_EN", 1)

                            .add("ERX", "0_ENABLE", 1)
                            .add("ERX", "1_ENABLE", 1)
                            .add("ERX", "2_ENABLE", 1)
                            .add("ERX", "3_ENABLE", 1)
                            .add("ERX", "4_ENABLE", 1)
                            .add("ERX", "5_ENABLE", 1)

                            .add("ELINKPROCESSORS", "GLOBAL_VETO_PASS_FAIL", 65535)
                            .add("ELINKPROCESSORS", "GLOBAL_RECON_MODE_RESULT", 3) 
                            .add("ELINKPROCESSORS", "GLOBAL_RECON_MODE_CHOICE", 0)  
                            .add("ELINKPROCESSORS", "GLOBAL_V_RECONSTRUCT_THRESH", 2)  
                            .add("ELINKPROCESSORS", "GLOBAL_ERX_MASK_EBO", 4095)  
                            .add("ELINKPROCESSORS", "GLOBAL_ERX_MASK_CRC", 4095)  
                            .add("ELINKPROCESSORS", "GLOBAL_ERX_MASK_HT", 4095) ;  
    auto econ_setup_test = econ_setup_builder.apply();
    // auto eprxgrptop = econ.dumpParameter("EPRXGRPTOP","GLOBAL_TRACK_MODE");
    // auto cheprxgrp5 = econ.dumpParameter("CHEPRXGRP","5_TRAIN_CHANNEL");
    // std::cout << "EPRXGRPTOP = " << eprxgrptop << std::endl ; 
    // std::cout << "CHEPRXGRP5 = " << cheprxgrp5 << std::endl ;  
  }

  // sets when snapshot is going to be taken
  { // scope this
    auto econ_setup_builder = econ.testParameters()
                            .add("ROCDAQCTRL", "GLOBAL_ORBSYN_CNT_SNAPSHOT", 0);           // UNSURE what this parameter should be. "snapshot location BX" or "ORBSYNCSNAPSHOT"
    auto econ_setup_test = econ_setup_builder.apply();
  }
//   // ---------------------------------- --------------------------------------------------------- //


//   // ---------------------------------- READING ECON REGISTERS ---------------------------------- //
auto cnt_load_val = econ.dumpParameter("ALIGNER","GLOBAL_ORBSYN_CNT_LOAD_VAL"); 

std::cout << "Orbsyn_cnt_load_val = " << cnt_load_val << std::endl ;

//       // read back econ_align.sh registers.
//       // read_snaphot.yaml
//       // check_econd_roc_alignment.yaml
//   // ---------------------------------- --------------------------------------------------------- //

  
          // DO I need a second round of alignment settings?     //Configure ECOND for Alignment (from econd_init_cpp.yaml)



//   // ---------------------------------------------- FAST CONTROL -------------------------------------------- //
// // see linkreset_rocs()

//LINK_RESET
  tgt->fc().linkreset_rocs();     // is this sufficient? Do I need to set a value e.g. ./uhal_backend_v3.py -b Housekeeping-FastCommands-fastcontrol-axi-0 --node bx_link_reset_roc${ECON} --val 3516

  // // econ version exists, 
// // ------------------------------------------------------------------------------------------------------------ //




//   // ---------------------------------- READING ECON REGISTERS ---------------------------------- //
//READ SNAPSHOT
  // Check ROC D pattern in each eRx   (from read_snapshot.yaml)       /// "ROC-D pattern"?
  auto ch_snap_0 = econ.dumpParameter("CHALIGNER", "0_PER_CH_ALIGN_EN"); 
  auto ch_snap_1 = econ.dumpParameter("CHALIGNER", "1_PER_CH_ALIGN_EN"); 
  auto ch_snap_2 = econ.dumpParameter("CHALIGNER", "2_PER_CH_ALIGN_EN"); 
  auto ch_snap_3 = econ.dumpParameter("CHALIGNER", "3_PER_CH_ALIGN_EN"); 
  auto ch_snap_4 = econ.dumpParameter("CHALIGNER", "4_PER_CH_ALIGN_EN"); 
  auto ch_snap_5 = econ.dumpParameter("CHALIGNER", "5_PER_CH_ALIGN_EN"); 

std::cout << "chAligner snapshot 0 = " << ch_snap_0 << std::endl ;
std::cout << "chAligner snapshot 1 = " << ch_snap_1 << std::endl ;
std::cout << "chAligner snapshot 2 = " << ch_snap_2 << std::endl ;
std::cout << "chAligner snapshot 3 = " << ch_snap_3 << std::endl ;
std::cout << "chAligner snapshot 4 = " << ch_snap_4 << std::endl ;
std::cout << "chAligner snapshot 5 = " << ch_snap_5 << std::endl ;

//READ ALIGNMENT STATUS  (from check_econd_roc_alignment.yaml)
  auto ch_pm_0 = econ.dumpParameter("CHALIGNER", "0_PATTERN_MATCH"); 
  auto ch_snap_dv_0 = econ.dumpParameter("CHALIGNER", "0_SNAPSHOT_DV"); 
  auto ch_select_0 = econ.dumpParameter("CHALIGNER", "0_SELECT"); 
  auto ch_pm_1 = econ.dumpParameter("CHALIGNER", "1_PATTERN_MATCH"); 
  auto ch_snap_dv_1 = econ.dumpParameter("CHALIGNER", "1_SNAPSHOT_DV"); 
  auto ch_select_1 = econ.dumpParameter("CHALIGNER", "1_SELECT"); 
  auto ch_pm_2 = econ.dumpParameter("CHALIGNER", "2_PATTERN_MATCH"); 
  auto ch_snap_dv_2 = econ.dumpParameter("CHALIGNER", "2_SNAPSHOT_DV"); 
  auto ch_select_2 = econ.dumpParameter("CHALIGNER", "2_SELECT"); 
  auto ch_pm_3 = econ.dumpParameter("CHALIGNER", "3_PATTERN_MATCH"); 
  auto ch_snap_dv_3 = econ.dumpParameter("CHALIGNER", "3_SNAPSHOT_DV"); 
  auto ch_select_3 = econ.dumpParameter("CHALIGNER", "3_SELECT"); 
  auto ch_pm_4 = econ.dumpParameter("CHALIGNER", "4_PATTERN_MATCH"); 
  auto ch_snap_dv_4 = econ.dumpParameter("CHALIGNER", "4_SNAPSHOT_DV"); 
  auto ch_select_4 = econ.dumpParameter("CHALIGNER", "4_SELECT"); 
  auto ch_pm_5 = econ.dumpParameter("CHALIGNER", "5_PATTERN_MATCH"); 
  auto ch_snap_dv_5 = econ.dumpParameter("CHALIGNER", "5_SNAPSHOT_DV"); 
  auto ch_select_5 = econ.dumpParameter("CHALIGNER", "5_SELECT"); 

std::cout << "chAligner pattern_match 0 = " << ch_pm_0 << std::endl ;
std::cout << "chAligner snapshot_dv 0 = " << ch_snap_dv_0 << std::endl ;
std::cout << "chAligner select 0 = " << ch_select_0 << std::endl ;
std::cout << "chAligner pattern_match 1 = " << ch_pm_1 << std::endl ;
std::cout << "chAligner snapshot_dv 1 = " << ch_snap_dv_1 << std::endl ;
std::cout << "chAligner select 1 = " << ch_select_1 << std::endl ;
std::cout << "chAligner pattern_match 2 = " << ch_pm_2 << std::endl ;
std::cout << "chAligner snapshot_dv 2 = " << ch_snap_dv_2 << std::endl ;
std::cout << "chAligner select 2 = " << ch_select_2 << std::endl ;
std::cout << "chAligner pattern_match 3 = " << ch_pm_3 << std::endl ;
std::cout << "chAligner snapshot_dv 3 = " << ch_snap_dv_3 << std::endl ;
std::cout << "chAligner select 3 = " << ch_select_3 << std::endl ;
std::cout << "chAligner pattern_match 4 = " << ch_pm_4 << std::endl ;
std::cout << "chAligner snapshot_dv 4 = " << ch_snap_dv_4 << std::endl ;
std::cout << "chAligner select 4 = " << ch_select_4 << std::endl ;
std::cout << "chAligner pattern_match 5 = " << ch_pm_5 << std::endl ;
std::cout << "chAligner snapshot_dv 5 = " << ch_snap_dv_5 << std::endl ;
std::cout << "chAligner select 5 = " << ch_select_5 << std::endl ;

//       // read back econ_align.sh registers.
//       // check_econd_roc_alignment.yaml
//   // ---------------------------------- --------------------------------------------------------- //



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
