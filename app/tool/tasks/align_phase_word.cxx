#include "align_phase_word.h"

#include <boost/multiprecision/cpp_int.hpp>

#include "../econ_snapshot.h"
#include "pflib/ROC.h"
#include "pflib/packing/Hex.h"

ENABLE_LOGGING();

bool debug_checks = false;

// ROC Idle Frame
constexpr uint32_t ROC_IDLE_FRAME = 0x5555555;

// ECON PUSM State READY
constexpr int ECON_EXPECTED_PUSM_STATE = 8;

using pflib::packing::hex;

void reset_stream() { std::cout << std::dec << std::setfill(' '); }

void print_roc_status(pflib::ROC& roc) {
  auto top_params = roc.getParameters("TOP");
  auto RunL = top_params.find("RUNL")->second;
  auto RunR = top_params.find("RUNR")->second;
  std::cout << "RunL = " << RunL << std::endl;
  std::cout << "RunR = " << RunR << std::endl;

  for (int half = 0; half < 2; ++half) {
    auto params = roc.getParameters("DIGITALHALF_" + std::to_string(half));

    auto idle = params.at("IDLEFRAME");
    auto bx = params.at("BX_OFFSET");
    auto bxtrig = params.at("BX_TRIGGER");

    std::cout << "Idle_" << half << " = " << idle << ", " << hex(idle) << '\n'
              << "bxoffset_" << half << " = " << bx << ", " << hex(bx) << '\n'
              << "bxtrigger_" << half << " = " << bxtrig << ", " << hex(bxtrig)
              << '\n';
    reset_stream();
  }
}

void align_phase_word(Target* tgt) {
  bool on_zcu =
      (pftool::state.readout_config() == pftool::State::CFG_HCALFMC) ||
      (pftool::state.readout_config() == pftool::State::CFG_HCALOPTO_ZCU) ||
      (pftool::state.readout_config() == pftool::State::CFG_ECALOPTO_ZCU);

  debug_checks = pftool::readline_bool("Enable debug checks?", true);

  int iroc = pftool::readline_int("Which ROC to manage: ", pftool::state.iroc);
  int iecon =
      pftool::readline_int("Which ECON to manage: ", pftool::state.iecon);

  auto roc = tgt->roc(iroc);
  auto econ = tgt->econ(iecon);

  // // Get channels from user
  // std::string ch_str = pftool::readline(
  //     "Enter channel(s) to check (comma-separated), default is Ch0, Ch1. Upon succesful match, all channels will be checked for alignment at that "
  //     "BX, for the appropriate ROC: ",
  //     "0,1,2,3,4,5,6,7,8,9,10,11");

  // GET Channels dynamically from ROC to ECON object channel mapping
  auto& mapping = tgt->get_channel_mapping();
  
  //TESTING CODE
  std::cout << "Mapping test, roc 0: " << mapping[0].first << ", " << mapping[0].second << std::endl;
  std::cout << "Mapping test, roc #iroc: " << mapping[iroc].first << ", " << mapping[iroc].second << std::endl;

  // DEPRECATED manual input code.
  // std::vector<int> channels_input;
  // std::vector<int> all_channels = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
  // std::stringstream ss(ch_str);
  // std::string item;
  // while (std::getline(ss, item, ',')) {
  //   try {
  //     channels_input.push_back(std::stoi(item));
  //   } catch (...) {
  //     std::cerr << "Invalid channel entry: " << item << std::endl;
  //   }
  // }

  // Dynamic channels. Only 2 per link.
  std::vector<int> channels;
  channels.push_back(mapping[iroc].first);
  channels.push_back(mapping[iroc].second);

  uint32_t binary_channels = build_channel_mask(channels);
  std::cout << "Channels to be configured: ";
  for (int ch : channels) std::cout << ch << " ";
  std::cout << std::endl;

  // Check PUSM state
  auto pusm_state = econ.readParameter("CLOCKSANDRESETS", "GLOBAL_PUSM_STATE");
  if (debug_checks) {
    std::cout << "Decimal value of channels: " << binary_channels << std::endl;
    std::cout << "PUSM_STATE = " << pusm_state << ", " << hex(pusm_state)
              << std::endl;
    reset_stream();
  }

  if (pusm_state != ECON_EXPECTED_PUSM_STATE) {
    std::cout << "PUSM_STATE / runbit does not equal "
              << ECON_EXPECTED_PUSM_STATE << ". Not running alignment task."
              << std::endl;
    return;
  }

  // Set IDLEs in ROC with enough bit transitions

  auto roc_setup_builder =
      roc.testParameters()
          .add("DIGITALHALF_0", "IDLEFRAME", ROC_IDLE_FRAME)
          .add("DIGITALHALF_1", "IDLEFRAME", ROC_IDLE_FRAME)
          .add("DIGITALHALF_0", "BX_OFFSET", 1)
          .add("DIGITALHALF_1", "BX_OFFSET", 1);
  if (on_zcu) {
    roc_setup_builder.add("DIGITALHALF_0", "BX_TRIGGER", 3543)
        .add("DIGITALHALF_1", "BX_TRIGGER", 3543);
  } else {
    roc_setup_builder.add("DIGITALHALF_0", "BX_TRIGGER", 64 * 40 - 20)
        .add("DIGITALHALF_1", "BX_TRIGGER", 64 * 40 - 20);
  }

  auto roc_test_params = roc_setup_builder.apply();

  // ----- PHASE ALIGNMENT ----- //
  {
    print_roc_status(roc);

    // Set ECON registers
    std::map<std::string, std::map<std::string, uint64_t>> parameters = {};
    // TrackMode 1: ECON will automatically find phase
    parameters["EPRXGRPTOP"]["GLOBAL_TRACK_MODE"] = 1;
    // Enable the ERX and set channel to train
    for (int ch : channels) {
      parameters["ERX"][std::to_string(ch) + "_ENABLE"] = 1;
      parameters["CHEPRXGRP"][std::to_string(ch) + "_TRAIN_CHANNEL"] = 1;
    }
    econ.applyParameters(parameters);

    // Toggle Phase Training off
    parameters.clear();
    for (int ch : channels) {
      parameters["CHEPRXGRP"][std::to_string(ch) + "_TRAIN_CHANNEL"] = 0;
    }
    econ.applyParameters(parameters);

    // Check channel locks
    for (int ch : channels) {
      std::string name = std::to_string(ch) + "_CHANNEL_LOCKED";
      auto val = econ.readParameter("CHEPRXGRP", name);
      std::cout << "Channel_locked " << ch << " = " << val << ", " << hex(val)
                << std::endl;
      reset_stream();
    }
  }
  // ------ END PHASE ALIGNMENT ------ //

  // ------------ WORD ALIGNMENT ----------- //
  {
    // print ROC status
    if (debug_checks) {
      print_roc_status(roc);
    }

    // ---- SETTING ECON REGISTERS ---- //
    std::map<std::string, std::map<std::string, uint64_t>> parameters = {};
    // BX value econ resets to when it receives BCR (linkreset)
    // Overall phase marker between ROC and ECON
    parameters["ALIGNER"]["GLOBAL_MATCH_MASK_VAL"] = 0;
    parameters["ALIGNER"]["GLOBAL_I2C_SNAPSHOT_EN"] = 0;
    parameters["ALIGNER"]["GLOBAL_SNAPSHOT_ARM"] = 0;
    parameters["ALIGNER"]["GLOBAL_SNAPSHOT_EN"] = 1;
    if (on_zcu) {
      parameters["ALIGNER"]["GLOBAL_ORBSYN_CNT_LOAD_VAL"] = 3514;  // 0xdba
      parameters["ALIGNER"]["GLOBAL_ORBSYN_CNT_MAX_VAL"] = 3563;   // 0xdeb
    } else {
      parameters["ALIGNER"]["GLOBAL_ORBSYN_CNT_LOAD_VAL"] = 40 * 64 - 39;
      parameters["ALIGNER"]["GLOBAL_ORBSYN_CNT_MAX_VAL"] = 40 * 64 - 1;
    }

    // Channel settings
    for (int channel : channels) {
      std::string var_name_align = std::to_string(channel) + "_PER_CH_ALIGN_EN";
      std::string var_name_erx = std::to_string(channel) + "_ENABLE";

      parameters["CHALIGNER"][var_name_align] = 1;
      parameters["ERX"][var_name_erx] = 1;
    }
    auto econ_word_align_currentvals_check = econ.applyParameters(parameters);

    // Set GLOBAL_MATCH_PATTERN_VAL
    econ.setValue(0x0381, 0x95555555A5555555, 8);

    // Verify that channels are still locked
    for (int ch : channels) {
      std::string name = std::to_string(ch) + "_CHANNEL_LOCKED";
      auto val = econ.readParameter("CHEPRXGRP", name);
      if (debug_checks) {
        std::cout << "channel_locked " << ch << " = " << val << ", " << hex(val)
                  << std::endl;
        reset_stream();
      }
    }

    auto global_match_pattern_val =
        econ.readParameter("ALIGNER", "GLOBAL_MATCH_PATTERN_VAL");
    auto cnt_load_val =
        econ.readParameter("ALIGNER", "GLOBAL_ORBSYN_CNT_LOAD_VAL");

    if (debug_checks) {
      std::cout << "GLOBAL_MATCH_PATTERN_VAL test: " << global_match_pattern_val
                << ", 0x" << std::hex << global_match_pattern_val << std::dec
                << std::endl;
      std::cout << "Orbsyn_cnt_load_val = " << cnt_load_val << ", 0x"
                << std::hex << cnt_load_val << std::dec << std::endl;
    }

    // FAST CONTROL - ENABLE THE BCR (ORBIT SYNC)
    tgt->fc().standard_setup();

    // TO DO
    // // Read BX value of link reset rocd
    // tgt->fc().bx_custom(3, 0xfff000, 3000);

    // ------- Scan when the ECON takes snapshot -----
    int start_val, end_val, testval, snapshot_match;
    if (on_zcu) {
      start_val = 3490;  // 3531;  // near your orbit region of interest
      end_val = 3540;    // up to orbit rollover
      testval = 3532;
    } else {
      start_val = 64 * 40 - 60;  // near your orbit region of interest
      end_val = 64 * 40 - 1;     // up to orbit rollover
      testval = start_val + 1;
    }

    std::cout << "Iterating over snapshots to find SPECIAL HEADER: "
              << std::endl;
    bool header_found = false;
    for (int snapshot_val = start_val; snapshot_val <= end_val;
         snapshot_val += 1) {
      std::cout << " --------------------------------------------------- "
                << std::endl;

      // int snapshot_val = testval;
      parameters.clear();
      parameters["ALIGNER"]["GLOBAL_ORBSYN_CNT_SNAPSHOT"] = snapshot_val;
      auto econ_word_align_currentvals = econ.applyParameters(parameters);

      auto tmp_load_val =
          econ.readParameter("ALIGNER", "GLOBAL_ORBSYN_CNT_SNAPSHOT");
      if (debug_checks) {
        std::cout << "Current snapshot BX = " << tmp_load_val << ", 0x"
                  << std::hex << tmp_load_val << std::dec << std::endl;
        std::cout << "Looking for special header pattern: "
                  << global_match_pattern_val << ", 0x" << std::hex
                  << global_match_pattern_val << std::dec << std::endl;
      }

      // FAST CONTROL - LINK_RESET
      tgt->fc().linkreset_rocs();

      for (int channel : channels) {
        // print out snapshot
        std::string var_name_pm = std::to_string(channel) + "_PATTERN_MATCH";
        auto ch_pm = econ.readParameter("CHALIGNER", var_name_pm);

        std::string var_name_snapshot1 =
            std::to_string(channel) + "_SNAPSHOT_0";
        std::string var_name_snapshot2 =
            std::to_string(channel) + "_SNAPSHOT_1";
        std::string var_name_snapshot3 =
            std::to_string(channel) + "_SNAPSHOT_2";
        auto ch_snapshot_1 =
            econ.readParameter("CHALIGNER", var_name_snapshot1);
        auto ch_snapshot_2 =
            econ.readParameter("CHALIGNER", var_name_snapshot2);
        auto ch_snapshot_3 =
            econ.readParameter("CHALIGNER", var_name_snapshot3);

        // Combine 3 × 64-bit words into one 192-bit integer
        boost::multiprecision::uint256_t snapshot =
            (boost::multiprecision::uint256_t(ch_snapshot_3) << 128) |
            (boost::multiprecision::uint256_t(ch_snapshot_2) << 64) |
            boost::multiprecision::uint256_t(ch_snapshot_1);

        // 192-bit >> 1 shift
        uint64_t w0_shifted = (ch_snapshot_1 >> 1);
        uint64_t w1_shifted =
            (ch_snapshot_2 >> 1) | ((ch_snapshot_1 & 1ULL) << 63);
        uint64_t w2_shifted =
            (ch_snapshot_3 >> 1) | ((ch_snapshot_2 & 1ULL) << 63);

        // shift by 1
        boost::multiprecision::uint256_t shifted1 = (snapshot >> 1);

        if (ch_pm == 1) {
          std::cout << "Header match in Snapshot: " << snapshot_val << std::endl
                    << "(channel " << channel << "): " << std::endl
                    << "snapshot_hex shifted >> 1bit: 0x" << std::hex
                    << std::uppercase << shifted1 << std::dec << std::endl;

          std::cout << "snapshot_hex: 0x" << std::hex << std::uppercase
                    << snapshot << std::dec << std::endl;

          std::cout << " pattern_match = " << ch_pm << ", 0x" << std::hex
                    << ch_pm << std::dec << std::endl;

          std::string var_name_select = std::to_string(channel) + "_SELECT";
          auto ch_select = econ.readParameter("CHALIGNER", var_name_select);
          std::cout << " select " << channel << " = " << ch_select << ", 0x"
                    << std::hex << ch_select << std::dec << std::endl;

          // shift and mask the snapshot to confirm special header
          boost::multiprecision::uint256_t shifted =
              (snapshot >> (ch_select - 32)) & 0xffffffffffffffffULL;
          std::cout << "Shifted and masked: 0x" << std::hex << std::uppercase
                    << shifted << std::dec << std::endl;

          header_found = true;
          std::cout << " --------------------------------------------------- "
                    << std::endl;
          std::cout << "Successful header match in Snapshot: " << snapshot_val
                    << std::endl;
          snapshot_match = snapshot_val;
          break;  // out of channel loop.
        } else if (debug_checks) {
          std::cout << " (Channel " << channel << ") " << std::endl
                    << "snapshot_hex_shifted: 0x" << std::hex << std::uppercase
                    << shifted1 << std::dec << std::endl;
          std::cout << "snapshot_hex: 0x" << std::hex << std::uppercase
                    << snapshot << std::dec << std::endl;
        } else if (!debug_checks) {
          std::cout << "No header pattern match found in Snapshot:  "
                    << snapshot_val << ", Channel: " << channel << std::endl;
        } else {
          std::cout << "No header pattern match found in Snapshot:  "
                    << snapshot_val << std::endl;
          break;  // out of channel loop
        }

      }  // end loop over snapshots for single channel
      if (header_found) break;  // out of loop over snapshots
    }
    // -------------- END SNAPSHOT BX SCAN ------------ //
    if (!header_found) {
      std::cout << "------------------------------------------" << std::endl
                << "Failure to match header pattern in ANY Snapshot."
                << std::endl;
    } else {
      // Header successfully found at snapshot_val. Check pattern match for all
      // eRx
      for (int channel : all_channels) {
        // print out pattern match for all channels
        std::string var_name_pm = std::to_string(channel) + "_PATTERN_MATCH";
        auto ch_pm = econ.readParameter("CHALIGNER", var_name_pm);
        std::cout << "------------------------------------------" << std::endl
                  << "Channel " << channel << " pattern match: " << ch_pm
                  << std::endl;
      }
    }

  }  // -------- END WORD ALIGNMENT ------- //
  // ensure 0 remaining 0's filling cout
  std::cout << std::dec << std::setfill(' ');

}  // End
