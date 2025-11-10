#include "align_phase_word.h"

#include "pflib/DecodeAndWrite.h"
#include "pflib/utility/json.h"
#include <boost/multiprecision/cpp_int.hpp>
#include "pflib/ROC.h"

ENABLE_LOGGING();

bool debug_checks = false;

// ROC Idle Frame
constexpr uint32_t ROC_IDLE_FRAME = 0x5555555;

// ECON PUSM State READY
constexpr int ECON_EXPECTED_PUSM_STATE = 8;

// temporary tool to print hex
template <typename T>
std::string to_hex(T v) {
  std::ostringstream oss;
  oss << "0x" << std::hex << std::uppercase << v << std::dec;
  return oss.str();
}

uint32_t build_channel_mask(std::vector<int>& channels) {
  /*
    Bit wise OR comparsion between e.g. 6 and 7,
    and shifting the '1' bit in the lowest sig bit,
    with << operator by the amount of the channel #.
  */
  uint32_t mask = 0;
  for (int ch : channels) 
    mask |= (1 << ch);
  return mask;
}

void print_roc_status(pflib::ROC& roc) {
  auto top_params = roc.getParameters("TOP");
  auto RunL = top_params.find("RUNL")->second;
  auto RunR = top_params.find("RUNR")->second;
  std::cout << "RunL = " << RunL << std::endl;
  std::cout << "RunR = " << RunR << std::endl;

  for (int half = 0; half < 2; ++half) {
    auto params = roc.getParameters("DIGITALHALF_" + std::to_string(half));

    auto idle    = params.at("IDLEFRAME");
    auto bx      = params.at("BX_OFFSET");
    auto bxtrig  = params.at("BX_TRIGGER");

    std::cout << "Idle_"      << half << " = " << idle   << ", " << to_hex(idle)   << '\n'
              << "bxoffset_"  << half << " = " << bx     << ", " << to_hex(bx)     << '\n'
              << "bxtrigger_" << half << " = " << bxtrig << ", " << to_hex(bxtrig) << '\n';
  }
}

void align_phase_word(Target* tgt) {
  auto roc = tgt->hcal().roc(pftool::state.iroc);
  auto econ = tgt->hcal().econ(pftool::state.iecon);

  // TODO: get channels from user
  std::vector<int> channels = {6, 7};
  uint32_t binary_channels = build_channel_mask(channels);
  std::cout << "Channels to be configured: ";
  for (int ch : channels) std::cout << ch << " ";
  std::cout << std::endl;
  std::cout << "Decimal value of channels: " << binary_channels << std::endl;

  // Check PUSM state
  auto pusm_state = econ.dumpParameter("CLOCKSANDRESETS", "GLOBAL_PUSM_STATE");
  std::cout << "PUSM_STATE = " << pusm_state << ", " << to_hex(pusm_state) << std::endl;

  if (pusm_state != ECON_EXPECTED_PUSM_STATE) {
    std::cout << "PUSM_STATE / runbit does not equal " << ECON_EXPECTED_PUSM_STATE
	      << ". Not running alignment task." << std::endl;
    return;
  }

  // Set IDLEs in ROC with enough bit transitions
  auto roc_setup_builder = roc.testParameters()
    .add("DIGITALHALF_0", "IDLEFRAME", ROC_IDLE_FRAME)
    .add("DIGITALHALF_1", "IDLEFRAME", ROC_IDLE_FRAME);
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
      auto val = econ.dumpParameter("CHEPRXGRP", name);
      std::cout << "channel_locked " << ch << " = " << val << ", " << to_hex(val)
                  << std::endl;
    }
  }
  // ------ END PHASE ALIGNMENT ------ //

  // ------------ WORD ALIGNMENT ----------- //
  {
    // print ROC status
    print_roc_status(roc);

    // ---- SETTING ECON REGISTERS ---- //
    std::map<std::string, std::map<std::string, uint64_t>> parameters = {};
    // BX value econ resets to when it receives BCR (linkreset)
    // Overall phase marker between ROC and ECON
    parameters["ALIGNER"]["GLOBAL_ORBSYN_CNT_LOAD_VAL"] = 3514; // 0xdba
    parameters["ALIGNER"]["GLOBAL_MATCH_MASK_VAL"] = 0;
    parameters["ALIGNER"]["GLOBAL_I2C_SNAPSHOT_EN"] = 0;
    parameters["ALIGNER"]["GLOBAL_SNAPSHOT_ARM"] = 0;
    parameters["ALIGNER"]["GLOBAL_SNAPSHOT_EN"] = 1;
    parameters["ALIGNER"]["GLOBAL_ORBSYN_CNT_MAX_VAL"] = 3563;  // 0xdeb
    
    // Channel settings
    for (int channel : channels) {
      std::string var_name_align =
	std::to_string(channel) + "_PER_CH_ALIGN_EN";
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
      auto val = econ.dumpParameter("CHEPRXGRP", name);
      std::cout << "channel_locked " << ch << " = " << val << ", " << to_hex(val)
                  << std::endl;
    }

    auto global_match_pattern_val =
      econ.dumpParameter("ALIGNER", "GLOBAL_MATCH_PATTERN_VAL");
    auto cnt_load_val =
      econ.dumpParameter("ALIGNER", "GLOBAL_ORBSYN_CNT_LOAD_VAL");
    
    std::cout << "GLOBAL_MATCH_PATTERN_VAL test: "
	      << global_match_pattern_val << ", 0x" << std::hex
	      << global_match_pattern_val << std::dec << std::endl;    
    std::cout << "Orbsyn_cnt_load_val = " << cnt_load_val << ", 0x"
	      << std::hex << cnt_load_val << std::dec << std::endl;

    // FAST CONTROL - ENABLE THE BCR (ORBIT SYNC)
    tgt->fc().standard_setup();

    // TO DO
    // // Read BX value of link reset rocd
    // tgt->fc().bx_custom(3, 0xfff000, 3000);    
    
    // ------- Scan when the ECON takes snapshot -----
    int start_val = 3531; // near your orbit region of interest
    int end_val = 3540;  // up to orbit rollover
    int testval = 3532; 

    
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
        econ.dumpParameter("ALIGNER", "GLOBAL_ORBSYN_CNT_SNAPSHOT");
      if(debug_checks){
        std::cout << "Current snapshot BX = " << tmp_load_val << ", 0x"
        << std::hex << tmp_load_val << std::dec << std::endl;
      }
      
      // FAST CONTROL - LINK_RESET
      tgt->fc().linkreset_rocs();

      for (int channel : channels) {
        // print out snapshot
        std::string var_name_pm = std::to_string(channel) + "_PATTERN_MATCH";
        auto ch_pm = econ.dumpParameter("CHALIGNER", var_name_pm);

        // TODO read only in debug
        //std::string var_name_snap_dv = std::to_string(channel) + "_SNAPSHOT_DV";
        //auto ch_snap_dv = econ.dumpParameter("CHALIGNER", var_name_snap_dv);
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
            << std::setw(16) << ch_snapshot_3 << std::dec
            << std::setfill(' ');
        std::string snapshot_hex = hexstring.str();

        // Combine 3 × 64-bit words into one 192-bit integer
        boost::multiprecision::uint256_t snapshot = (boost::multiprecision::uint256_t(ch_snapshot_3) << 128) |
                            (boost::multiprecision::uint256_t(ch_snapshot_2) << 64) |
                            boost::multiprecision::uint256_t(ch_snapshot_1);

        // 192-bit >> 1 shift
        uint64_t w0_shifted = (ch_snapshot_1 >> 1);
        uint64_t w1_shifted = (ch_snapshot_2 >> 1) | ((ch_snapshot_1 & 1ULL) << 63);
        uint64_t w2_shifted = (ch_snapshot_3 >> 1) | ((ch_snapshot_2 & 1ULL) << 63);
        
        std::ostringstream hexstring_sh;
        hexstring_sh << std::hex << std::setfill('0')
              << std::setw(16) << w2_shifted
              << std::setw(16) << w1_shifted
              << std::setw(16) << w0_shifted;
        std::string snapshot_hex_shifted = hexstring_sh.str();
        
        //if (snapshot_hex.find("955") != std::string::npos) {
        if(ch_pm == 1) {
          std::cout << "Header match" << std::endl << " (channel "
              << channel << ") " << std::endl
              << "snapshot_hex_shifted: 0x" << snapshot_hex_shifted << std::endl;

                std::cout << "snapshot_hex: 0x" << snapshot_hex << std::endl;

          std::cout << " pattern_match = " << ch_pm << ", 0x" << std::hex
              << ch_pm << std::dec << std::endl;

          std::string var_name_select = std::to_string(channel) + "_SELECT";
          auto ch_select = econ.dumpParameter("CHALIGNER", var_name_select);
          std::cout << " select " << channel << " = " << ch_select
              << ", 0x" << std::hex << ch_select << std::dec
              << std::endl;
            
          std::cout << "256bit snap: " << snapshot << std::endl;
          boost::multiprecision::uint256_t shifted = (snapshot >> (ch_select - 32)) & 0xffffffffffffffffULL;
          std::cout << "Shifted and masked: " << shifted << std::endl;

          header_found = true;
          break; // out of channel loop
        }
        else if (debug_checks)
        {
          std::cout << " (Channel "
              << channel << ") " << std::endl
              << "snapshot_hex_shifted: 0x" << snapshot_hex_shifted << std::endl;
          
          std::cout << "snapshot_hex: 0x" << std::hex << std::setfill('0') << std::setw(16)
              << ch_snapshot_1 << std::setw(16) << ch_snapshot_2
              << std::setw(16) << ch_snapshot_3 << std::dec
              << std::setfill(' ') << std::endl;
        }
        
      } // end loop over snapshots for single channel
      if (header_found) break;  // out of loop over snapshots
    }
    // -------------- END SNAPSHOT BX SCAN ------------ //
        
  }  // -------- END WORD ALIGNMENT ------- //
  
}  // End
