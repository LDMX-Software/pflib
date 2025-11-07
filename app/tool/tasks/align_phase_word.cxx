#include "align_phase_word.h"

#include "pflib/DecodeAndWrite.h"
#include "pflib/utility/json.h"

ENABLE_LOGGING();

void align_phase_word(Target* tgt) {
  auto roc = tgt->hcal().roc(pftool::state.iroc);
  auto econ = tgt->hcal().econ(pftool::state.iecon);

  // Hardcode list from fastcontrol_axi of counter node headings
  std::vector<std::string> counterNames = {"errors",
                                           "l1a_suppressed",
                                           "bx_suppressed",
                                           "l1a",
                                           "l1a_nzs",
                                           "orbit_sync",
                                           "orbit_count_reset",
                                           "internal_calibration_pulse",
                                           "external_calibration_pulse",
                                           "chipsync",
                                           "ecr",
                                           "ebr",
                                           "link_reset_roct",
                                           "link_reset_rocd",
                                           "link_reset_econt",
                                           "link_reset_econd",
                                           "spare0",
                                           "spare1",
                                           "spare2",
                                           "spare3",
                                           "spare4",
                                           "spare5",
                                           "spare6",
                                           "spare7",
                                           "unassigned"};

  int IDLE = 89478485;  // == 0x5555555

  int list_channels[] = {6, 7};
  int binary_channels = 0;

  for (int i = 0; i < std::size(list_channels); i++) {
    binary_channels = binary_channels | (1 << list_channels[i]);
  }

  // print outs
  std::cout << "Channels to be configured: " << std::endl;
  for (int channel : list_channels) {
    std::cout << channel << "  " << std::endl;
  }
  std::cout << "Decimal value of channels: " << binary_channels << std::endl;

  // ----- PHASE ALIGNMENT SCOPE ----- //
  {
    std::map<std::string, std::map<std::string, uint64_t>> parameters = {};

    // set inversion bit on ECON
    int edgesel = 0;
    int invertfcmd = 1;
    parameters = {{"FCTRL", {{"GLOBAL_INVERT_COMMAND_RX", invertfcmd}}}};
    auto econ_inversion_runbit_currentvals = econ.applyParameters(parameters);

    // set econ run mode
    econ.setRunMode(1, edgesel, invertfcmd);

    auto pusm_run = econ.dumpParameter("CLOCKSANDRESETS", "GLOBAL_PUSM_RUN");
    auto pusm_state =
        econ.dumpParameter("CLOCKSANDRESETS", "GLOBAL_PUSM_STATE");

    // print outs
    std::cout << "PUSM_STATE = " << pusm_state << ", 0x" << std::hex
              << pusm_state << std::dec << std::endl;
    std::cout << "PUSM_RUN = " << pusm_run << ", 0x" << std::hex << pusm_run
              << std::dec << std::endl;

    if (pusm_state == 8) {
      // ----- SETTING ROC REGISTERS ----- //
      auto roc_setup_builder = roc.testParameters()
                                   .add("DIGITALHALF_0", "IDLEFRAME", IDLE)
                                   .add("DIGITALHALF_1", "IDLEFRAME", IDLE);
      auto roc_setup_test = roc_setup_builder.apply();
      auto params = roc.getParameters("DIGITALHALF_0");
      auto idle_0 = params.find("IDLEFRAME")->second;

      std::cout << "roc idle_0,1 = " << idle_0 << ", 0x" << std::hex << idle_0
                << std::dec << std::endl;
      // -------------------------------- //

      // ---- SETTING ECON REGISTERS ---- //
      // Set track mode ON
      parameters["EPRXGRPTOP"]["GLOBAL_TRACK_MODE"] = 1;
      auto econ_phase_align_currentvals = econ.applyParameters(parameters);

      // Set Phase Training OFF, channel enable ON
      for (int channel : list_channels) {
        std::string var_name = std::to_string(channel) + "_TRAIN_CHANNEL";
        std::string var_name_erx = std::to_string(channel) + "_ENABLE";
        parameters["ERX"][var_name_erx] = 1;
        parameters["CHEPRXGRP"][var_name] = 0;
      }
      econ.applyParameters(parameters);

      // Toggle Phase Training ON
      for (int channel : list_channels) {
        std::string var_name = std::to_string(channel) + "_TRAIN_CHANNEL";
        parameters["CHEPRXGRP"][var_name] = 1;
      }
      econ.applyParameters(parameters);

      // Toggle Phase Training Off
      for (int channel : list_channels) {
        std::string var_name = std::to_string(channel) + "_TRAIN_CHANNEL";
        parameters["CHEPRXGRP"][var_name] = 0;
      }
      econ.applyParameters(parameters);

      // Print outs
      auto eprxgrptop = econ.dumpParameter("EPRXGRPTOP", "GLOBAL_TRACK_MODE");
      auto cheprxgrp5 = econ.dumpParameter("CHEPRXGRP", "5_TRAIN_CHANNEL");
      auto cheprxgrp6 = econ.dumpParameter("CHEPRXGRP", "6_TRAIN_CHANNEL");
      auto cheprxgrp7 = econ.dumpParameter("CHEPRXGRP", "7_TRAIN_CHANNEL");
      std::cout << "EPRXGRPTOP = " << eprxgrptop << ",0x" << std::hex
                << eprxgrptop << std::dec << std::endl;
      std::cout << "CHEPRXGRP5 (Example train_channel 5) = " << cheprxgrp5
                << ", 0x" << std::hex << cheprxgrp5 << std::dec << std::endl;
      std::cout << "CHEPRXGRP6 (Example train_channel 6) = " << cheprxgrp6
                << ", 0x" << std::hex << cheprxgrp6 << std::dec << std::endl;
      std::cout << "CHEPRXGRP7 (Example train_channel 7) = " << cheprxgrp7
                << ", 0x" << std::hex << cheprxgrp7 << std::dec << std::endl;
      // -------------------------------- //

      // ---- READING ECON REGISTERS ---- //
      // Check channel locks
      std::map<int, int> ch_lock_values;
      for (int channel : list_channels) {
        std::string var_name = std::to_string(channel) + "_CHANNEL_LOCKED";
        auto ch_lock = econ.dumpParameter("CHEPRXGRP", var_name);
        ch_lock_values[channel] = ch_lock;
        std::cout << "channel_locked " << channel << " = " << ch_lock << ", 0x"
                  << std::hex << ch_lock << std::dec << std::endl;
      }
      // ----------------------------- //

    } else {
      std::cout << "PUSM_STATE / runbit does not equal 8. Not running phase "
                   "aligment task."
                << std::endl;
    }
  }
  // ------ END PHASE ALIGNMENT scope ------ //

  // ------------ WORD ALIGNMENT scope ----------- //
  {
    // re-set inversion value and runbit
    std::map<std::string, std::map<std::string, uint64_t>> parameters = {};
    int edgesel = 0;
    int invertfcmd = 1;
    econ.setRunMode(1, edgesel, invertfcmd);

    auto pusm_run = econ.dumpParameter("CLOCKSANDRESETS", "GLOBAL_PUSM_RUN");
    auto pusm_state =
        econ.dumpParameter("CLOCKSANDRESETS", "GLOBAL_PUSM_STATE");

    if (pusm_state == 8) {
      // ---- RE SETTING ROC REGISTERS ---- //
      auto roc_setup_builder = roc.testParameters()
                                   .add("DIGITALHALF_0", "IDLEFRAME", IDLE)
                                   .add("DIGITALHALF_1", "IDLEFRAME", IDLE);
      auto roc_setup_test = roc_setup_builder.apply();

      // auto params = roc.getParameters("DIGITALHALF_0");

      // ----- READING ROC REGISTERS ----- //
      auto params = roc.getParameters("TOP");
      auto inv_fc_rx = params.find("IN_INV_CMD_RX")->second;

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
      // ---------------------------------- //

      // ---- SETTING ECON REGISTERS ---- //
      parameters = {};
      parameters["ROCDAQCTRL"]["GLOBAL_HGCROC_HDR_MARKER"] = 15;  // 0xf
      parameters["ROCDAQCTRL"]["GLOBAL_SYNC_HEADER"] = 1;
      parameters["ROCDAQCTRL"]["GLOBAL_SYNC_BODY"] = 89478485;  // 0x5555555
      parameters["ROCDAQCTRL"]["GLOBAL_ACTIVE_ERXS"] = binary_channels;
      parameters["ROCDAQCTRL"]["GLOBAL_PASS_THRU_MODE"] = 1;
      parameters["ROCDAQCTRL"]["GLOBAL_MATCH_THRESHOLD"] = 2;
      parameters["ROCDAQCTRL"]["GLOBAL_SIMPLE_MODE"] = 1;

      // BX value econ resets to when it recieves BCR (linkreset)
      // Overall phase marker between ROC and ECON
      parameters["ALIGNER"]["GLOBAL_ORBSYN_CNT_LOAD_VAL"] = 3514;
      // 0xdba

      // // // BX value econ takes snapshot
      // parameters["ALIGNER"]["GLOBAL_ORBSYN_CNT_SNAPSHOT"] = 3532;
      // // // 0xdcc

      // parameters["ALIGNER"]["GLOBAL_MATCH_PATTERN_VAL"] = 2505397589;
      // // 0x95555555

      parameters["ALIGNER"]["GLOBAL_MATCH_MASK_VAL"] = 0;
      parameters["ALIGNER"]["GLOBAL_I2C_SNAPSHOT_EN"] = 0;
      parameters["ALIGNER"]["GLOBAL_SNAPSHOT_EN"] = 1;
      parameters["ALIGNER"]["GLOBAL_ORBSYN_CNT_MAX_VAL"] = 3563;  // 0xdeb

      // Channel settings
      for (int channel : list_channels) {
        std::string var_name_align =
            std::to_string(channel) + "_PER_CH_ALIGN_EN";
        std::string var_name_erx = std::to_string(channel) + "_ENABLE";
        std::string var_name_erxINV = std::to_string(channel) + "_INVERT_DATA";

        parameters["CHALIGNER"][var_name_align] = 1;
        parameters["ERX"][var_name_erx] = 1;
        parameters["ERX"][var_name_erxINV] = 0;
      }
      auto econ_word_align_currentvals_check = econ.applyParameters(parameters);
      econ.setValue(0x0381, 0x95555555a5555555, 8); 

      //   ----- READING ECON REGISTERS ----- //
      std::map<int, int> ch_lock_values;

      // Channel Locking print outs
      for (int channel : list_channels) {
        std::string var_name = std::to_string(channel) + "_CHANNEL_LOCKED";
        auto ch_lock = econ.dumpParameter("CHEPRXGRP", var_name);
        ch_lock_values[channel] = ch_lock;
        std::cout << "channel_locked " << channel << " = " << ch_lock << ", 0x"
                  << std::hex << ch_lock << std::dec << std::endl;
      };

      // Snapshot match value print out
      for (int channel : list_channels) {
        auto global_match_pattern_val =
            econ.dumpParameter("ALIGNER", "GLOBAL_MATCH_PATTERN_VAL");
        std::cout << "GLOBAL_MATCH_PATTERN_VAL test: "
                  << global_match_pattern_val << ", 0x" << std::hex
                  << global_match_pattern_val << std::dec << std::endl;
      }

      auto cnt_load_val =
          econ.dumpParameter("ALIGNER", "GLOBAL_ORBSYN_CNT_LOAD_VAL");
      auto snapshot_arm = econ.dumpParameter("ALIGNER", "GLOBAL_SNAPSHOT_ARM");

      std::cout << "Orbsyn_cnt_load_val = " << cnt_load_val << ", 0x"
                << std::hex << cnt_load_val << std::dec << std::endl;
      std::cout << "Global snapshot ARM = " << snapshot_arm << ", 0x"
                << std::hex << snapshot_arm << std::dec << std::endl;
      // ----------------------------------- //

      // ------- SCAN BUNCH CROSSINGS ------- //
      int snapshot_6bx;
      int start_val = 3530; //0;   // near your orbit region of interest
      int end_val = 3563;  // up to orbit rollover
      int testval = 3532; 

      for (int snapshot_val = start_val; snapshot_val <= end_val;
           snapshot_val += 6) {
        // int snapshot_val = testval;
        parameters["ALIGNER"]["GLOBAL_ORBSYN_CNT_SNAPSHOT"] = snapshot_val;
        auto econ_word_align_currentvals = econ.applyParameters(parameters);



        auto tmp_load_val =
          econ.dumpParameter("ALIGNER", "GLOBAL_ORBSYN_CNT_SNAPSHOT");
        std::cout << "temp snapshot val = " << tmp_load_val << ", 0x"
                  << std::hex << tmp_load_val << std::dec << std::endl;
                  

        // FAST CONTROL - LINK_RESET
        tgt->fc().orbit_count_reset();
        // usleep(100);
        tgt->fc().standard_setup();
        tgt->fc().linkreset_rocs();

        // print out snapshot
        for (int channel : list_channels) {
          std::string var_name_snapshot1 =
              std::to_string(channel) + "_SNAPSHOT_0";
          std::string var_name_snapshot2 =
              std::to_string(channel) + "_SNAPSHOT_1";
          std::string var_name_snapshot3 =
              std::to_string(channel) + "_SNAPSHOT_2";
          auto ch_snapshot_1 =
              econ.dumpParameter("CHALIGNER", var_name_snapshot1);
          auto ch_snapshot_2 =
              econ.dumpParameter("CHALIGNER", var_name_snapshot2);
          auto ch_snapshot_3 =
              econ.dumpParameter("CHALIGNER", var_name_snapshot3);
        
          std::ostringstream hexstring;
          hexstring << std::hex << std::setfill('0') << std::setw(16)
                    << ch_snapshot_1 << std::setw(16) << ch_snapshot_2
                    << std::setw(16) << ch_snapshot_3;
          std::string snapshot_hex = hexstring.str();

          if (snapshot_hex.find("95555555") != std::string::npos) {
            std::cout << "Header match near BX " << snapshot_val << " (channel "
                      << channel << ") "
                      << "snapshot_hex: 0x" << snapshot_hex << std::endl;
            snapshot_6bx = snapshot_val;
          }
        }
      }
      // -------------- END BX SCAN ------------ //

      // ------- READING ECON REGISTERS ------- //
      // READ SNAPSHOT
      for (int channel : list_channels) {
        std::string var_name_align =
            std::to_string(channel) + "_PER_CH_ALIGN_EN";
        std::string var_name_pm = std::to_string(channel) + "_PATTERN_MATCH";
        std::string var_name_snap_dv = std::to_string(channel) + "_SNAPSHOT_DV";
        std::string var_name_snapshot1 =
            std::to_string(channel) + "_SNAPSHOT_0";
        std::string var_name_snapshot2 =
            std::to_string(channel) + "_SNAPSHOT_1";
        std::string var_name_snapshot3 =
            std::to_string(channel) + "_SNAPSHOT_2";
        std::string var_name_select = std::to_string(channel) + "_SELECT";

        auto ch_snap = econ.dumpParameter("CHALIGNER", var_name_align);
        auto ch_pm = econ.dumpParameter("CHALIGNER", var_name_pm);
        auto ch_snap_dv = econ.dumpParameter("CHALIGNER", var_name_snap_dv);
        auto ch_select = econ.dumpParameter("CHALIGNER", var_name_select);

        auto ch_snapshot_1 =
            econ.dumpParameter("CHALIGNER", var_name_snapshot1);
        auto ch_snapshot_2 =
            econ.dumpParameter("CHALIGNER", var_name_snapshot2);
        auto ch_snapshot_3 =
            econ.dumpParameter("CHALIGNER", var_name_snapshot3);
        // concatentate full 192 bit snapshot:
        std::string str_channel_snapshot = std::to_string(ch_snapshot_1) +
                                           std::to_string(ch_snapshot_2) +
                                           std::to_string(ch_snapshot_3);

        std::cout << "channel_snapshot1 " << channel << " = " << ch_snapshot_1
                  << ", 0x" << std::hex << ch_snapshot_1 << std::dec
                  << std::endl;
        std::cout << "channel_snapshot2 " << channel << " = " << ch_snapshot_2
                  << ", 0x" << std::hex << ch_snapshot_2 << std::dec
                  << std::endl;
        std::cout << "channel_snapshot3 " << channel << " = " << ch_snapshot_3
                  << ", 0x" << std::hex << ch_snapshot_3 << std::dec
                  << std::endl;
        std::cout << "channel_snapshot_full_hexstring " << channel << " = 0x"
                  << std::hex << std::setfill('0') << std::setw(16)
                  << ch_snapshot_1 << std::setw(16) << ch_snapshot_2
                  << std::setw(16) << ch_snapshot_3 << std::dec
                  << std::setfill(' ') << std::endl;
        std::cout << "channel_snap " << channel << " = " << ch_snap << ", 0x"
                  << std::hex << ch_snap << std::dec << std::endl;
        std::cout << "chAligner pattern_match = " << ch_pm << ", 0x" << std::hex
                  << ch_pm << std::dec << std::endl;
        std::cout << "chAligner snapshot_dv = " << ch_snap_dv << ", 0x"
                  << std::hex << ch_snap_dv << std::dec << std::endl;
        std::cout << "chAligner select " << channel << " = " << ch_select
                  << ", 0x" << std::hex << ch_select << std::dec
                  << "(0xa0 = failed alignment)" << std::endl;
      }

      // Custom BX value
      int bx_new = 3000;
      int bx_addr = 3;
      int bx_mask = 0xfff000;
      tgt->fc().bx_custom(bx_addr, bx_mask, bx_new);

    } else {
      std::cout << "PUSM_STATE / runbit does not equal 8. Not running phase "
                   "aligment task."
                << std::endl;
    }
    // ------------------------------------------- //

  }  // -------- END WORD ALIGNMENT SCOPE ------- //
}  // End
