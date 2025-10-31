#include "align_phase_word.h"

#include "pflib/DecodeAndWrite.h"
#include "pflib/utility/json.h"

ENABLE_LOGGING();

void align_phase_word(Target* tgt) {
  auto roc = tgt->hcal().roc(pftool::state.iroc);
  auto econ = tgt->hcal().econ(pftool::state.iecon);

  // --------------------------------------------------------------------------------------------------------------------------------
  // //
  // ----------------------------------------------------- PHASE ALIGNMENT
  // ----------------------------------------------------- //
  int IDLE = 89478485;  // == 5555555. hardcode based on phase alignment script

  // check PUSM state, run task only if state=8
  // SET and Read PUSH runbit; also ensure GLOBAL_INVERT_COMMAND_RX is set to
  // get ECON to lock to FCMDs; otherwise run bit will = 3.
  // Scope 2 builders to make this happen in sequence
  
  // {  // set inversion value
    auto econ_setup_builder = econ.testParameters().add(
        "FCTRL", "GLOBAL_INVERT_COMMAND_RX",
        1);  // set fctrl inversion so that ECON can Lock.
    auto econ_setup_test = econ_setup_builder.apply();
  // }

    // usleep(1000);

    // // {  // set run value
    // auto econ_setup_builder =
    //     econ.testParameters().add("CLOCKSANDRESETS", "GLOBAL_PUSM_RUN",
    //                               1);  // set run bit = 1 AFTER inversion bit.
    // auto econ_setup_test = econ_setup_builder.apply();
    // // }

    auto pusm_run = econ.dumpParameter(
        "CLOCKSANDRESETS",
        "GLOBAL_PUSM_RUN");  // dump Parameter I added to force this
                             // functionality to be able to assign to an output
                             // variable
    auto pusm_state =
        econ.dumpParameter("CLOCKSANDRESETS", "GLOBAL_PUSM_STATE");

    std::cout << "PUSM_STATE = " << pusm_state << ", 0x" << std::hex
              << pusm_state << std::dec << std::endl;
    std::cout << "PUSM_RUN = " << pusm_run << ", 0x" << std::hex << pusm_run
              << std::dec << std::endl;

    if (pusm_state == 8) {
      int list_channels[] = {6, 7};  // {2,3,4,5,6,7};
      int binary_channels = 0;

      for (int i = 0; i < std::size(list_channels); i++) {
        binary_channels =
            binary_channels |
            (1 << list_channels[i]);  // bit wise OR comparison between e.g. 6
                                      // and 7, and shifting the '1' bit in the
                                      // lowest sig bit, with << operator by the
                                      // amount of the channel #.
      }

      std::cout << "Channels to be configured: " << std::endl;
      for (int channel : list_channels) {
        std::cout << channel << "  " << std::endl;
      }
      std::cout << "Decimal value of channels: " << binary_channels
                << std::endl;

      // ---------------------------------- SETTING ROC REGISTERS
      // ---------------------------------- //
      {  // scope this
        // set run modes
        roc.setRunMode(1);

        auto roc_setup_builder = roc.testParameters()
                                     .add("DIGITALHALF_0", "IDLEFRAME", IDLE)
                                     .add("DIGITALHALF_1", "IDLEFRAME", IDLE);
        auto params = roc.getParameters(
            "DIGITALHALF_0");  // this uses the page and returns a mapping of
                               // all params therein
        auto idle_0 =
            params.find("IDLEFRAME")->second;  // second because its a key value
                                               // pair mapping (See ROC.cxx)

        std::cout << "idle_0 = " << idle_0 << ", 0x" << std::hex << idle_0
                  << std::dec << std::endl;
      }

      // ----------------------------------
      // --------------------------------------------------------- //

      // ---------------------------------- SETTING ECON REGISTERS
      // ---------------------------------- //

      {  // scope this
        // Set Phase Training ON
        auto econ_setup_builder = econ.testParameters().add(
            "EPRXGRPTOP", "GLOBAL_TRACK_MODE",
            1);  // corresponding to configs/train_erx_phase_ON_econ.yaml
        for (int channel : list_channels) {
          std::string var_name = std::to_string(channel) + "_TRAIN_CHANNEL";
          econ_setup_builder.add("CHEPRXGRP", var_name, 1);
          // std::cout << "channel, varname = " << channel << ", " << var_name
          // << std::endl;
        }
        auto econ_setup_test = econ_setup_builder.apply();
        auto eprxgrptop = econ.dumpParameter("EPRXGRPTOP", "GLOBAL_TRACK_MODE");
        auto cheprxgrp5 = econ.dumpParameter("CHEPRXGRP", "5_TRAIN_CHANNEL");
        std::cout << "EPRXGRPTOP = " << eprxgrptop << ", 0x" << std::hex
                  << eprxgrptop << std::dec << std::endl;
        std::cout << "CHEPRXGRP5 (Example train_channel 5) = " << cheprxgrp5
                  << ", 0x" << std::hex << cheprxgrp5 << std::dec
                  << std::endl;  // arbitrarily using 5 as example
      }

      {  // scope this
        // Set Training OFF
        for (int channel : list_channels) {
          std::string var_name =
              std::to_string(channel) +
              "_TRAIN_CHANNEL";  // corresponding to
                                 // configs/train_erx_phase_OFF_econ.yaml
          auto econ_setup_builder =
              econ.testParameters().add("CHEPRXGRP", var_name, 0);
          auto econ_setup_test = econ_setup_builder.apply();
        }
        auto cheprxgrp5 = econ.dumpParameter("CHEPRXGRP", "5_TRAIN_CHANNEL");
        std::cout << "CHEPRXGRP5 (Example train_channel 5) = " << cheprxgrp5
                  << ", 0x" << std::hex << cheprxgrp5 << std::dec << std::endl;
      }

      // --------------------------------------------------------------------------------------------
      // //

      // ---------------------------------- READING ECON REGISTERS
      // ---------------------------------- //
      {  // scope
        // Check channel lock
        std::map<int, int> ch_lock_values;  // create map for storing channel -
                                            // value. Note this is not used, but
                                            // here just in case its needed.
        for (int channel : list_channels) {
          std::string var_name = std::to_string(channel) + "_CHANNEL_LOCKED";
          auto ch_lock = econ.dumpParameter("CHEPRXGRP", var_name);
          ch_lock_values[channel] = ch_lock;
          std::cout << "channel_locked " << channel << " = " << ch_lock
                    << ", 0x" << std::hex << ch_lock << std::dec << std::endl;
        }
      }
      // ---------------------------------------------------------------------
      // //

      // ----------------------------------------------------- END PHASE
      // ALIGNMENT
      // ----------------------------------------------------- //
      // --------------------------------------------------------------------------------------------------------------------------------
      // //

      // //
      // --------------------------------------------------------------------------------------------------------------------------------
      // //
      // // ----------------------------------------------------- WORD ALIGNMENT
      // ----------------------------------------------------- //

      //   // ---------------------------------- READING ROC REGISTERS
      //   ---------------------------------- //
      {  // scope this
        auto params =
            roc.getParameters("TOP");  // this uses the page and returns a
                                       // mapping of all params therein
        auto inv_fc_rx = params.find("IN_INV_CMD_RX")
                             ->second;  // second because its a key value pair
                                        // mapping (See ROC.cxx)
        std::cout << "inv_fc_rx = " << inv_fc_rx << ", 0x" << std::hex
                  << inv_fc_rx << std::dec << std::endl;

        params = roc.getParameters("TOP");
        auto RunL = params.find("RUNL")->second;
        std::cout << "RunL = " << RunL << ", 0x" << std::hex << RunL << std::dec
                  << std::endl;

        params = roc.getParameters("TOP");
        auto RunR = params.find("RUNR")->second;
        std::cout << "RunR = " << RunR << ", 0x" << std::hex << RunR << std::dec
                  << std::endl;

        params = roc.getParameters("DIGITALHALF_0");
        auto idle_0 = params.find("IDLEFRAME")->second;
        params = roc.getParameters("DIGITALHALF_1");
        auto idle_1 = params.find("IDLEFRAME")->second;
        std::cout << "Idle_0 = " << idle_0 << ", 0x" << std::hex << idle_0
                  << std::dec << std::endl;
        std::cout << "Idle_1 = " << idle_1 << ", 0x" << std::hex << idle_1
                  << std::dec << std::endl;

        params = roc.getParameters("DIGITALHALF_0");
        auto bx_0 = params.find("BX_OFFSET")->second;
        params = roc.getParameters("DIGITALHALF_1");
        auto bx_1 = params.find("BX_OFFSET")->second;
        std::cout << "bxoffset_0 = " << bx_0 << ", 0x" << std::hex << bx_0
                  << std::dec << std::endl;
        std::cout << "bxoffset_1 = " << bx_1 << ", 0x" << std::hex << bx_1
                  << std::dec << std::endl;

        params = roc.getParameters("DIGITALHALF_0");
        auto bxtrig_0 = params.find("BX_TRIGGER")->second;
        params = roc.getParameters("DIGITALHALF_1");
        auto bxtrig_1 = params.find("BX_TRIGGER")->second;
        std::cout << "bxtrigger_0 = " << bxtrig_0 << ", 0x" << std::hex
                  << bxtrig_0 << std::dec << std::endl;
        std::cout << "bxtrigger_1 = " << bxtrig_1 << ", 0x" << std::hex
                  << bxtrig_1 << std::dec << std::endl;
      }
      //   // ----------------------------------
      //   --------------------------------------------------------- //

      //   // ---------------------------------- SETTING ECON REGISTERS
      //   ---------------------------------- //
      // Configure ECOND for Alignment (from econd_init_cpp.yaml)
      {  // scope this
        auto econ_setup_builder =
            econ.testParameters()
                .add("ROCDAQCTRL", "GLOBAL_HGCROC_HDR_MARKER", 15)
                .add("ROCDAQCTRL", "GLOBAL_SYNC_HEADER", 1)
                .add("ROCDAQCTRL", "GLOBAL_SYNC_BODY", 89478485)
                .add("ROCDAQCTRL", "GLOBAL_ACTIVE_ERXS", binary_channels)
                .add("ROCDAQCTRL", "GLOBAL_PASS_THRU_MODE", 1)
                .add("ROCDAQCTRL", "GLOBAL_MATCH_THRESHOLD", 2)
                .add("ROCDAQCTRL", "GLOBAL_SIMPLE_MODE", 1)

                .add("ALIGNER", "GLOBAL_ORBSYN_CNT_LOAD_VAL", 1)
                .add("ALIGNER", "GLOBAL_ORBSYN_CNT_SNAPSHOT", 3080)
                .add("ALIGNER", "GLOBAL_MATCH_PATTERN_VAL",
                     2505397589)  // IS THIS CORRECT (and those below _00 _01)
                // .add("ALIGNER", "GLOBAL_MATCH_PATTERN_VAL_01", 2773833045)
                .add("ALIGNER", "GLOBAL_MATCH_MASK_VAL",
                     00)  // is this set like this?
                .add("ALIGNER", "GLOBAL_MATCH_MASK_VAL", 01)
                .add("ALIGNER", "GLOBAL_I2C_SNAPSHOT_EN", 0)
                .add("ALIGNER", "GLOBAL_SNAPSHOT_EN", 1)
                .add("ALIGNER", "GLOBAL_ORBSYN_CNT_MAX_VAL", 3563)
                .add("ALIGNER", "GLOBAL_FREEZE_OUTPUT_ENABLE", 0)
                .add("ALIGNER",
                     "GLOBAL_FREEZE_OUTPUT_ENABLE_ALL_CHANNELS_LOCKED", 0)

                .add("ELINKPROCESSORS", "GLOBAL_VETO_PASS_FAIL", 65535)
                .add("ELINKPROCESSORS", "GLOBAL_RECON_MODE_RESULT", 3)
                .add("ELINKPROCESSORS", "GLOBAL_RECON_MODE_CHOICE", 0)
                .add("ELINKPROCESSORS", "GLOBAL_V_RECONSTRUCT_THRESH", 2)
                .add("ELINKPROCESSORS", "GLOBAL_ERX_MASK_EBO", 4095)
                .add("ELINKPROCESSORS", "GLOBAL_ERX_MASK_CRC", 4095)
                .add("ELINKPROCESSORS", "GLOBAL_ERX_MASK_HT", 4095);
        auto econ_setup_test = econ_setup_builder.apply();
        auto global_match_pattern_val =
            econ.dumpParameter("ALIGNER", "GLOBAL_MATCH_PATTERN_VAL");
        std::cout << "GLOBAL_MATCH_PATTERN_VAL test: "
                  << global_match_pattern_val << ", 0x" << std::hex
                  << global_match_pattern_val << std::dec << std::endl;
      }

      {
        for (int channel : list_channels) {
          std::string var_name_align =
              std::to_string(channel) + "_PER_CH_ALIGN_EN";
          std::string var_name_erx = std::to_string(channel) + "_ENABLE";
          auto econ_setup_builder = econ.testParameters()
                                        .add("CHALIGNER", var_name_align, 1)
                                        .add("ERX", var_name_erx, 1);
          auto econ_setup_test = econ_setup_builder.apply();
          // std::cout << "channel, varname_align/erx = " << channel << ", " <<
          // var_name_align << ", " << var_name_erx << std::endl;  // DEBUG line
        }
      }

      // sets when snapshot is going to be taken
      {  // scope this
        auto econ_setup_builder = econ.testParameters().add(
            "ALIGNER", "GLOBAL_ORBSYN_CNT_SNAPSHOT", 0);
        auto econ_setup_test = econ_setup_builder.apply();
      }
      //   // ----------------------------------
      //   --------------------------------------------------------- //

      //   // ---------------------------------- READING ECON REGISTERS
      //   ---------------------------------- //
      auto cnt_load_val =
          econ.dumpParameter("ALIGNER", "GLOBAL_ORBSYN_CNT_LOAD_VAL");

      std::cout << "Orbsyn_cnt_load_val = " << cnt_load_val << ", 0x"
                << std::hex << cnt_load_val << std::dec << std::endl;
      //   // ----------------------------------
      //   --------------------------------------------------------- //

      //   // ---------------------------------------------- FAST CONTROL
      //   -------------------------------------------- //
      // // see linkreset_rocs()

      // LINK_RESET
      tgt->fc().standard_setup();
      tgt->fc().linkreset_rocs();
      auto cmdcounters = tgt->fc().getCmdCounters();
      for (uint32_t i : cmdcounters) {
        std::cout << "i: " << i << ", ";
      }
      std::cout << std::endl;

      // Custom BX value
      int bx_new = 3000;
      int bx_addr = 3;
      int bx_mask = 0xfff000;
      tgt->fc().bx_custom(bx_addr, bx_mask, bx_new);

      // //
      // ------------------------------------------------------------------------------------------------------------
      // //

      //   // ---------------------------------- READING ECON REGISTERS
      //   ---------------------------------- //
      // READ SNAPSHOT
      {  // scope
        // Check ROC D pattern in each eRx   (from read_snapshot.yaml)
        for (int channel : list_channels) {
          std::string var_name_align =
              std::to_string(channel) + "_PER_CH_ALIGN_EN";
          std::string var_name_pm = std::to_string(channel) + "_PATTERN_MATCH";
          std::string var_name_snap_dv =
              std::to_string(channel) + "_SNAPSHOT_DV";
          std::string var_name_select = std::to_string(channel) + "_SELECT";
          auto ch_snap = econ.dumpParameter("CHALIGNER", var_name_align);
          auto ch_pm = econ.dumpParameter("CHALIGNER", var_name_pm);
          auto ch_snap_dv = econ.dumpParameter("CHALIGNER", var_name_snap_dv);
          auto ch_select = econ.dumpParameter("CHALIGNER", var_name_select);
          std::cout << "channel_snap " << channel << " = " << ch_snap << ", 0x"
                    << std::hex << ch_snap << std::dec << std::endl;
          std::cout << "chAligner pattern_match = " << ch_pm << ", 0x"
                    << std::hex << ch_pm << std::dec << std::endl;
          std::cout << "chAligner snapshot_dv = " << ch_snap_dv << ", 0x"
                    << std::hex << ch_snap_dv << std::dec << std::endl;
          std::cout << "chAligner select " << channel << " = " << ch_select
                    << ", 0x" << std::hex << ch_select << std::dec << std::endl;
        }
      }

      //   // ----------------------------------
      //   --------------------------------------------------------- //

      // // ----------------------------------------------------- END WORD
      // ALIGNMENT ----------------------------------------------------- //
      // //
      // --------------------------------------------------------------------------------------------------------------------------------
      // //

    }  // end of scope checking if pusmstate=8
    else {
      std::cout << "PUSM_STATE / runbit does not equal 8. Not running phase "
                   "aligment task."
                << std::endl;
    }
  }
