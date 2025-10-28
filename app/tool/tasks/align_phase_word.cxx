#include "align_phase_word.h"

#include "pflib/DecodeAndWrite.h"
#include "pflib/utility/json.h"

ENABLE_LOGGING();

void align_phase_word(Target* tgt) {
  auto roc = tgt->hcal().roc(pftool::state.iroc);
  auto econ = tgt->hcal().econ(pftool::state.iecon);


// -------------------------------------------------------------------------------------------------------------------------------- //
// ----------------------------------------------------- PHASE ALIGNMENT ----------------------------------------------------- //
  int IDLE = 89478485;  // == 5555555. hardcode based on phase alignment script


// check PUSM state, run task only if state=8
  // Read PUSH
  auto pusm_run = econ.dumpParameter("CLOCKSANDRESETS","GLOBAL_PUSM_RUN");   // dump Parameter I added to force this functionality to be able to assign to an output variable
  auto pusm_state = econ.dumpParameter("CLOCKSANDRESETS","GLOBAL_PUSM_STATE");
    
  std::cout << "PUSM_STATE = " << pusm_state << std::endl ;
  std::cout << "PUSM_RUN = " << pusm_run << std::endl ;
  //" (0x" << std::hex << pusm_state << std::dec << ")\n";

  if(pusm_state==8){

    int poke_channels[] = {2,3,4,5,6,7};
  
    // ---------------------------------- SETTING ROC REGISTERS ---------------------------------- //
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
    
    // NOT setting run bit - commenting out
    // { // scope this
    //   std::cout << "1 test = " << std::endl ;
    //   auto econ_setup_builder = econ.testParameters().add("CLOCKSANDRESETS", "GLOBAL_PUSM_RUN", 0); // set run bit 0 while configuring
    //   auto econ_setup_test = econ_setup_builder.apply();
    //   auto pusm_run = econ.dumpParameter("CLOCKSANDRESETS","GLOBAL_PUSM_RUN");   // dump Parameter I added to force this functionality to be able to assign to an output variable
    //   auto pusm_state = econ.dumpParameter("CLOCKSANDRESETS","GLOBAL_PUSM_STATE"); 
    //   std::cout << "First PUSM_STATE = " << pusm_state << std::endl ;
    //   std::cout << "First PUSM_RUN = " << pusm_run << std::endl ;
    // }

    //Phase ON
    { // scope this
    auto econ_setup_builder = econ.testParameters()
                            .add("EPRXGRPTOP", "GLOBAL_TRACK_MODE", 1);  // corresponding to configs/train_erx_phase_ON_econ.yaml
    for(int channel : poke_channels){
      std::string var_name = std::to_string(channel) + "_TRAIN_CHANNEL";
      econ_setup_builder.add("CHEPRXGRP", var_name, 1);
      std::cout << "channel, varname = " << channel << ", " << var_name << std::endl;
    }



    
                              // .add("CHEPRXGRP", "0_TRAIN_CHANNEL", 1) // corresponding to configs/train_erx_phase_TRAIN_econ.yaml
                              // .add("CHEPRXGRP", "1_TRAIN_CHANNEL", 1)  
                              // .add("CHEPRXGRP", "2_TRAIN_CHANNEL", 1)  
                              // .add("CHEPRXGRP", "3_TRAIN_CHANNEL", 1)  
                              // .add("CHEPRXGRP", "4_TRAIN_CHANNEL", 1)  
                              // .add("CHEPRXGRP", "5_TRAIN_CHANNEL", 1)  
                              // .add("CHEPRXGRP", "6_TRAIN_CHANNEL", 1)  
                              // .add("CHEPRXGRP", "7_TRAIN_CHANNEL", 1)  
                              // .add("CHEPRXGRP", "8_TRAIN_CHANNEL", 1)  
                              // .add("CHEPRXGRP", "9_TRAIN_CHANNEL", 1)  
                              // .add("CHEPRXGRP", "10_TRAIN_CHANNEL", 1)  
                              // .add("CHEPRXGRP", "11_TRAIN_CHANNEL", 1);  
      auto econ_setup_test = econ_setup_builder.apply();
      auto eprxgrptop = econ.dumpParameter("EPRXGRPTOP","GLOBAL_TRACK_MODE");
      auto cheprxgrp5 = econ.dumpParameter("CHEPRXGRP","5_TRAIN_CHANNEL");
      std::cout << "EPRXGRPTOP = " << eprxgrptop << std::endl ; 
      std::cout << "CHEPRXGRP5 (Example train_channel 5) = " << cheprxgrp5 << std::endl ;   // arbitrarily using 5 as example
    }


    // { // scope this
    // NOT setting run bit - commenting out
    //   // Set run bit ON
    //   auto econ_setup_builder = econ.testParameters()
    //                           .add("CLOCKSANDRESETS", "GLOBAL_PUSM_RUN", 1);
    //   auto econ_setup_test = econ_setup_builder.apply();
    //   auto pusm_run = econ.dumpParameter("CLOCKSANDRESETS","GLOBAL_PUSM_RUN");
    //   auto pusm_state = econ.dumpParameter("CLOCKSANDRESETS","GLOBAL_PUSM_STATE");
    //   std::cout << "PUSM_STATE = " << pusm_state << std::endl ;
    //   std::cout << "PUSM_RUN = " << pusm_run << std::endl ;
    // }

    // { // scope this
    //   // Set run bit OFF
    //   auto econ_setup_builder = econ.testParameters()
    //                           .add("CLOCKSANDRESETS", "GLOBAL_PUSM_RUN", 0);
    //   auto econ_setup_test = econ_setup_builder.apply();
    //   auto pusm_run = econ.dumpParameter("CLOCKSANDRESETS","GLOBAL_PUSM_RUN");
    //   auto pusm_state = econ.dumpParameter("CLOCKSANDRESETS","GLOBAL_PUSM_STATE");
    //   std::cout << "PUSM_STATE = " << pusm_state << std::endl ;
    //   std::cout << "PUSM_RUN = " << pusm_run << std::endl ;
    // }


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
      std::cout << "CHEPRXGRP5 (Example train_channel 5) = " << cheprxgrp5 << std::endl ;
    }


    // { // scope this
    // // NOT setting run bit - commenting out
    //   // Set run bit ON
    //   auto econ_setup_builder = econ.testParameters()
    //                           .add("CLOCKSANDRESETS", "GLOBAL_PUSM_RUN", 1);
    //   auto econ_setup_test = econ_setup_builder.apply();
    //   auto pusm_run = econ.dumpParameter("CLOCKSANDRESETS","GLOBAL_PUSM_RUN");
    //   auto pusm_state = econ.dumpParameter("CLOCKSANDRESETS","GLOBAL_PUSM_STATE");
    //   std::cout << "PUSM_STATE = " << pusm_state << std::endl ;
    //   std::cout << "PUSM_RUN = " << pusm_run << std::endl ;
    // }

    { // scope this
      // Set Channel Lock ?
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
      std::cout << "CHEPRXGRP5 (Example lock_channel 5) = " << cheprxgrp5 << std::endl ;  
    };
    // -------------------------------------------------------------------------------------------- //
    

    // ---------------------------------- READING ECON REGISTERS ---------------------------------- //


    // Check channel lock
    auto ch_lock_0 = econ.dumpParameter("CHEPRXGRP", "0_CHANNEL_LOCKED");     // can also readParameters()
    auto ch_lock_1 = econ.dumpParameter("CHEPRXGRP", "1_CHANNEL_LOCKED"); 
    auto ch_lock_2 = econ.dumpParameter("CHEPRXGRP", "2_CHANNEL_LOCKED"); 
    auto ch_lock_3 = econ.dumpParameter("CHEPRXGRP", "3_CHANNEL_LOCKED"); 
    auto ch_lock_4 = econ.dumpParameter("CHEPRXGRP", "4_CHANNEL_LOCKED"); 
    auto ch_lock_5 = econ.dumpParameter("CHEPRXGRP", "5_CHANNEL_LOCKED"); 
    auto ch_lock_6 = econ.dumpParameter("CHEPRXGRP", "6_CHANNEL_LOCKED"); 
    auto ch_lock_7 = econ.dumpParameter("CHEPRXGRP", "7_CHANNEL_LOCKED"); 
    auto ch_lock_8 = econ.dumpParameter("CHEPRXGRP", "8_CHANNEL_LOCKED"); 
    auto ch_lock_9 = econ.dumpParameter("CHEPRXGRP", "9_CHANNEL_LOCKED"); 
    auto ch_lock_10 = econ.dumpParameter("CHEPRXGRP", "10_CHANNEL_LOCKED"); 
    auto ch_lock_11 = econ.dumpParameter("CHEPRXGRP", "11_CHANNEL_LOCKED"); 

    std::cout << "channel_locked 0 = " << ch_lock_0 << std::endl ;
    std::cout << "channel_locked 1 = " << ch_lock_1 << std::endl ;
    std::cout << "channel_locked 2 = " << ch_lock_2 << std::endl ;
    std::cout << "channel_locked 3 = " << ch_lock_3 << std::endl ;
    std::cout << "channel_locked 4 = " << ch_lock_4 << std::endl ;
    std::cout << "channel_locked 6 = " << ch_lock_6 << std::endl ;
    std::cout << "channel_locked 7 = " << ch_lock_7 << std::endl ;
    std::cout << "channel_locked 8 = " << ch_lock_8 << std::endl ;
    std::cout << "channel_locked 9 = " << ch_lock_9 << std::endl ;
    std::cout << "channel_locked 10 = " << ch_lock_10 << std::endl ;
    std::cout << "channel_locked 11 = " << ch_lock_11 << std::endl ;

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
      auto global_match_pattern_val = econ.dumpParameter("ALIGNER", "GLOBAL_MATCH_PATTERN_VAL"); 
      std::cout << "GLOBAL_MATCH_PATTERN_VAL test: " << global_match_pattern_val << std::endl ;
    }

    // sets when snapshot is going to be taken
    { // scope this
      auto econ_setup_builder = econ.testParameters()
                              .add("ALIGNER", "GLOBAL_ORBSYN_CNT_SNAPSHOT", 0);
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



  //   // ---------------------------------------------- FAST CONTROL -------------------------------------------- //
  // // see linkreset_rocs()

  //LINK_RESET
    // tgt->fc().linkreset_rocs();     // is this sufficient? Do I need to set a value e.g. ./uhal_backend_v3.py -b Housekeeping-FastCommands-fastcontrol-axi-0 --node bx_link_reset_roc${ECON} --val 3516

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
    auto ch_snap_6 = econ.dumpParameter("CHALIGNER", "6_PER_CH_ALIGN_EN"); 
    auto ch_snap_7 = econ.dumpParameter("CHALIGNER", "7_PER_CH_ALIGN_EN"); 
    auto ch_snap_8 = econ.dumpParameter("CHALIGNER", "8_PER_CH_ALIGN_EN"); 
    auto ch_snap_9 = econ.dumpParameter("CHALIGNER", "9_PER_CH_ALIGN_EN"); 
    auto ch_snap_10 = econ.dumpParameter("CHALIGNER", "10_PER_CH_ALIGN_EN"); 
    auto ch_snap_11 = econ.dumpParameter("CHALIGNER", "11_PER_CH_ALIGN_EN"); 

  std::cout << "chAligner snapshot 0 = " << ch_snap_0 << std::endl ;
  std::cout << "chAligner snapshot 1 = " << ch_snap_1 << std::endl ;
  std::cout << "chAligner snapshot 2 = " << ch_snap_2 << std::endl ;
  std::cout << "chAligner snapshot 3 = " << ch_snap_3 << std::endl ;
  std::cout << "chAligner snapshot 4 = " << ch_snap_4 << std::endl ;
  std::cout << "chAligner snapshot 5 = " << ch_snap_5 << std::endl ;
  std::cout << "chAligner snapshot 6 = " << ch_snap_6 << std::endl ;
  std::cout << "chAligner snapshot 7 = " << ch_snap_7 << std::endl ;
  std::cout << "chAligner snapshot 8 = " << ch_snap_8 << std::endl ;
  std::cout << "chAligner snapshot 9 = " << ch_snap_9 << std::endl ;
  std::cout << "chAligner snapshot 10 = " << ch_snap_10 << std::endl ;
  std::cout << "chAligner snapshot 11 = " << ch_snap_11 << std::endl ;

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
    auto ch_pm_6 = econ.dumpParameter("CHALIGNER", "6_PATTERN_MATCH"); 
    auto ch_snap_dv_6 = econ.dumpParameter("CHALIGNER", "6_SNAPSHOT_DV"); 
    auto ch_select_6 = econ.dumpParameter("CHALIGNER", "6_SELECT"); 
    auto ch_pm_7 = econ.dumpParameter("CHALIGNER", "7_PATTERN_MATCH"); 
    auto ch_snap_dv_7 = econ.dumpParameter("CHALIGNER", "7_SNAPSHOT_DV"); 
    auto ch_select_7 = econ.dumpParameter("CHALIGNER", "7_SELECT"); 
    auto ch_pm_8 = econ.dumpParameter("CHALIGNER", "8_PATTERN_MATCH"); 
    auto ch_snap_dv_8 = econ.dumpParameter("CHALIGNER", "8_SNAPSHOT_DV"); 
    auto ch_select_8 = econ.dumpParameter("CHALIGNER", "8_SELECT"); 
    auto ch_pm_9 = econ.dumpParameter("CHALIGNER", "9_PATTERN_MATCH"); 
    auto ch_snap_dv_9 = econ.dumpParameter("CHALIGNER", "9_SNAPSHOT_DV"); 
    auto ch_select_9 = econ.dumpParameter("CHALIGNER", "9_SELECT"); 
    auto ch_pm_10 = econ.dumpParameter("CHALIGNER", "10_PATTERN_MATCH"); 
    auto ch_snap_dv_10 = econ.dumpParameter("CHALIGNER", "10_SNAPSHOT_DV"); 
    auto ch_select_10 = econ.dumpParameter("CHALIGNER", "10_SELECT"); 
    auto ch_pm_11 = econ.dumpParameter("CHALIGNER", "11_PATTERN_MATCH"); 
    auto ch_snap_dv_11 = econ.dumpParameter("CHALIGNER", "11_SNAPSHOT_DV"); 
    auto ch_select_11 = econ.dumpParameter("CHALIGNER", "11_SELECT"); 

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
  std::cout << "chAligner pattern_match 6 = " << ch_pm_6 << std::endl ;
  std::cout << "chAligner snapshot_dv 6 = " << ch_snap_dv_6 << std::endl ;
  std::cout << "chAligner select 6 = " << ch_select_6 << std::endl ;
  std::cout << "chAligner pattern_match 7 = " << ch_pm_7 << std::endl ;
  std::cout << "chAligner snapshot_dv 7 = " << ch_snap_dv_7 << std::endl ;
  std::cout << "chAligner select 7 = " << ch_select_7 << std::endl ;
  std::cout << "chAligner pattern_match 8 = " << ch_pm_8 << std::endl ;
  std::cout << "chAligner snapshot_dv 8 = " << ch_snap_dv_8 << std::endl ;
  std::cout << "chAligner select 8 = " << ch_select_9 << std::endl ;
  std::cout << "chAligner pattern_match 9 = " << ch_pm_9 << std::endl ;
  std::cout << "chAligner snapshot_dv 9 = " << ch_snap_dv_9 << std::endl ;
  std::cout << "chAligner select 9 = " << ch_select_10 << std::endl ;
  std::cout << "chAligner pattern_match 10 = " << ch_pm_10 << std::endl ;
  std::cout << "chAligner snapshot_dv 10 = " << ch_snap_dv_10 << std::endl ;
  std::cout << "chAligner select 10 = " << ch_select_10 << std::endl ;
  std::cout << "chAligner pattern_match 11 = " << ch_pm_11 << std::endl ;
  std::cout << "chAligner snapshot_dv 11 = " << ch_snap_dv_11 << std::endl ;
  std::cout << "chAligner select 11 = " << ch_select_11 << std::endl ;
  //   // ---------------------------------- --------------------------------------------------------- //



  // // ----------------------------------------------------- END WORD ALIGNMENT ----------------------------------------------------- //
  // // -------------------------------------------------------------------------------------------------------------------------------- //

  } // end of scope checking if pusmstate=8
  else{
      std::cout << "PUSM_STATE / runbit does not equal 8. Not running phase aligment task." << std::endl ;
  }


}
