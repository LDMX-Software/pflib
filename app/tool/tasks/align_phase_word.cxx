#include "align_phase_word.h"

#include <boost/multiprecision/cpp_int.hpp>

#include "../econ_snapshot.h"
#include "pflib/ROC.h"
#include "pflib/packing/Hex.h"

ENABLE_LOGGING();

bool debug_checks = false;

// ROC Idle Frame
constexpr uint32_t ROC_WORD_IDLE_FRAME = 0x12c5c57;

constexpr uint64_t ECON_ROC_ALIGN_PATTERN = (
/* old, 64-bit match pattern
 * auto-aligner only looks at top 32-bits so its not working
 */
  (0x9ull << 60) | (static_cast<uint64_t>(ROC_WORD_IDLE_FRAME) << 32) |
  (0xaull << 28) | ROC_WORD_IDLE_FRAME
  // 32-bit pattern including ROC_IDLE and new-orbit header nibble 0xa
  //(0xaull << 28) | ROC_WORD_IDLE_FRAME
);

// ECON PUSM State READY
constexpr int ECON_EXPECTED_PUSM_STATE = 8;

static const int ALIGNER_BASE = 0x0380;

// 12 bits, default 0x2
static const int ALIGNER_ORBSYN_CNT_SNAPSHOT = ALIGNER_BASE + 0x16;

static const int CHALIGNER_RW[12] = {
  0x000,
  0x040,
  0x080,
  0x0c0,
  0x100,
  0x140,
  0x180,
  0x1c0,
  0x200,
  0x240,
  0x280,
  0x2c0
};

static const int CHALIGNER_RO[12] = {
  0x014,
  0x054,
  0x094,
  0x0d4,
  0x114,
  0x154,
  0x194,
  0x1d4,
  0x214,
  0x254,
  0x294,
  0x2d4
};

// 1b, flag if pattern found
static const int CHALIGNER_PATTERN_MATCH = 0x0;

// snapshot location, 192b
static const int CHALIGNER_SNAPSHOT = 0x2;

using pflib::packing::hex;

void print_roc_status(pflib::ROC& roc) {
  auto top_params = roc.getParameters("TOP");
  auto RunL = top_params.find("RUNL")->second;
  auto RunR = top_params.find("RUNR")->second;
  std::cout << "{ RunL = " << RunL << ", RunR = " << RunR;
  for (int half = 0; half < 2; ++half) {
    auto params = roc.getParameters("DIGITALHALF_" + std::to_string(half));

    auto idle = params.at("IDLEFRAME");
    auto bx = params.at("BX_OFFSET");
    auto bxtrig = params.at("BX_TRIGGER");

    std::cout << ", idle_" << half << " = 0x" << std::hex << idle << std::dec
              << ", bx_offset_" << half << " = " << bx
              << ", bx_trigger_" << half << " = " << bxtrig;
  }
  std::cout << " }\n";
};

/**
 * go through and check if the input channels are locked
 *
 * get the selected phase while there
 */
std::pair<bool, std::map<int,int>>
check_channel_phase_lock(pflib::ECON& econ, std::vector<int> channels) {
  // Check channel locks
  static const int CHEPRXGRP_RO[12] = {
    0x341, 0x345, 0x349, 0x34d, 0x351, 0x355, 0x359,
    0x35d, 0x361, 0x365, 0x369, 0x36d
  };
  bool all_locked = true;
  std::map<int,int> phases;
  for (int ch : channels) {
    auto status = econ.getValues(CHEPRXGRP_RO[ch], 2);
    bool locked = (((status[0] >> 7) & 0b1) == 1);
    bool dll_locked = ((status[1] & 0b1) == 1);
    int dll_state = (status[0] & 0b11);
    int phase = ((status[0] >> 2) & 0xf);
    phases[ch] = phase;
    all_locked = (all_locked and locked);
    std::cout << "channel " << ch << std::boolalpha
              << " locked = " << locked
              << " dll_locked = " << dll_locked
              << " dll_state = " << dll_state
              << " phase = " << phase
              << std::endl;
  }
  return std::make_pair(all_locked, phases);
}

void align_phase(Target* tgt, pflib::ECON& econ, std::vector<int> channels) {
  /**
   * phase alignment between ECON and HGCROC's 1.28GHz data channels
   */
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

  // Toggle Phase Training
  // this 1->0 transition is what triggers the phase training request
  parameters.clear();
  for (int ch : channels) {
    parameters["CHEPRXGRP"][std::to_string(ch) + "_TRAIN_CHANNEL"] = 0;
  }
  econ.applyParameters(parameters);

  auto [all_locked, phases] = check_channel_phase_lock(econ, channels);
  std::cout << "all locked: " << all_locked << std::endl;

  /**
   * manual says we should go into fixed phase mode (track_mode = 0)
   * and code the phase into the configuration so that it is
   * written down somewhere
   */
  if (all_locked) {
    // go into fixed phase mode to avoid the phase shifting
    parameters.clear();
    parameters["EPRXGRPTOP"]["GLOBAL_TRACK_MODE"] = 0;
    for (auto [ch, phase]: phases) {
      parameters["CHEPRXGRP"][std::to_string(ch)+"_PHASE_SELECT_CHANNELINPUT"] = phase;
    }
    econ.applyParameters(parameters);
  } else {
    pflib_log(warn) << "Not all channels are locked so not fixing the phase (leaving in track_mode = 1)";
  }
}

void align_word(Target* tgt, pflib::ECON& econ,
                std::vector<int> channels, bool on_zcu) {
  // print ROC status
  if (debug_checks) {
    std::cout << "ROC_WORD_IDLE_FRAME: 0x" << std::hex << ROC_WORD_IDLE_FRAME << std::dec << std::endl;
    std::cout << "ECON_ROC_ALIGN_PATTERN: 0x" << std::hex << ECON_ROC_ALIGN_PATTERN << std::dec << std::endl;
  }

  // ---- SETTING ECON REGISTERS ---- //
  std::map<std::string, std::map<std::string, uint64_t>> parameters = {};
  // BX value econ resets to when it receives BCR (linkreset)
  // Overall phase marker between ROC and ECON
  parameters["ALIGNER"]["GLOBAL_I2C_SNAPSHOT_EN"] = 0;
  parameters["ALIGNER"]["GLOBAL_SNAPSHOT_ARM"] = 0;
  parameters["ALIGNER"]["GLOBAL_SNAPSHOT_EN"] = 1;
  if (on_zcu) {
    parameters["ALIGNER"]["GLOBAL_ORBSYN_CNT_LOAD_VAL"] = 1;  // 0xdba
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
  //econ.setValue(ALIGNER_BASE+0x1, ECON_ROC_ALIGN_PATTERN, 8);
  // don't mask out any of the pattern
  // not sure if this means the match_mask should be all ones:
  //econ.setValue(ALIGNER_BASE+0x9, 0xffffffffffffffffull, 8);
  // or all zeros:
  //econ.setValue(ALIGNER_BASE+0x9, 0x0ull, 8);
  // or the default:
  econ.setValue(ALIGNER_BASE+0x9, 0xffffffff00000000ull, 8);
  // another option is not to mess with the match_mask at all
  // and just have the lower 32-bits be the correct pattern
  econ.setValue(ALIGNER_BASE+0x1, ECON_ROC_ALIGN_PATTERN >> 32, 8);

  if (debug_checks) {
    auto global_match_pattern_val =
        econ.readParameter("ALIGNER", "GLOBAL_MATCH_PATTERN_VAL");
    auto cnt_load_val =
        econ.readParameter("ALIGNER", "GLOBAL_ORBSYN_CNT_LOAD_VAL");
    std::cout << "GLOBAL_MATCH_PATTERN_VAL test: " << global_match_pattern_val
              << ", 0x" << std::hex << global_match_pattern_val << std::dec
              << std::endl;
    std::cout << "Orbsyn_cnt_load_val = " << cnt_load_val << ", 0x" << std::hex
              << cnt_load_val << std::dec << std::endl;
  }

  // FAST CONTROL - ENABLE THE BCR (ORBIT SYNC)
  tgt->fc().standard_setup();

  // TODO: Read BX value of link reset rocd

  // ------- Scan when the ECON takes snapshot -----
  int start_val, end_val;
  if (on_zcu) {
    start_val = 1; //3490;  // near your orbit region of interest
    end_val = 51; //3540;    // up to orbit rollover
  } else {
    start_val = 64 * 40 - 60;  // near your orbit region of interest
    end_val = 64 * 40 - 1;     // up to orbit rollover
  }

  std::cout << "Iterating over snapshots to find SPECIAL HEADER: " << std::endl;
  bool aligned{false};
  for (int snapshot_val = start_val; snapshot_val <= end_val;
       snapshot_val += 1) {
    std::cout << " --------------------------------------------------- "
              << std::endl;

    /*
    parameters.clear();
    parameters["ALIGNER"]["GLOBAL_ORBSYN_CNT_SNAPSHOT"] = snapshot_val;
    auto econ_word_align_currentvals = econ.applyParameters(parameters);
    */
    econ.setValue(ALIGNER_ORBSYN_CNT_SNAPSHOT, snapshot_val, 2);

    if (debug_checks) {
      auto tmp_load_val =
          econ.readParameter("ALIGNER", "GLOBAL_ORBSYN_CNT_SNAPSHOT");
      std::cout << "Current snapshot BX = " << tmp_load_val << ", 0x"
                << std::hex << tmp_load_val << std::dec << std::endl;
    }

    // FAST CONTROL - LINK_RESET
    tgt->fc().linkreset_rocs();

    std::cout << "Checking Snapshot: " << snapshot_val << std::endl;

    bool should_continue = false;
    for (int channel : channels) {
      // print out snapshot
      auto bit_flags = econ.getValues(CHALIGNER_RO[channel], 1);
      auto ch_pm = (bit_flags[0] & 0b1);

      auto select = econ.getValues(CHALIGNER_RO[channel]+0x1, 1)[0];

      boost::multiprecision::uint256_t snapshot{0};
      // have to read in 8byte chunks
      for (int i_snp{0}; i_snp < 3; i_snp++) {
        auto word = econ.getValues(CHALIGNER_RO[channel]+CHALIGNER_SNAPSHOT+8*i_snp, 8);
        for (int i_byte{0}; i_byte < 8; i_byte++) {
          snapshot |= (boost::multiprecision::uint256_t(word[i_byte]) << (8*(8*i_snp + i_byte)));
        }
      }

      if (should_continue or (ch_pm != 1)) {
        // this channel has not found the header so we should continue searching
        should_continue = true;
      }

      if (true) { //debug_checks) {
        std::cout << "channel " << channel << " match: " << std::boolalpha << (ch_pm == 1)
                  << " : 0x" << std::hex << snapshot << std::dec << std::endl;
        if (ch_pm == 1) {
          auto shifted((snapshot >> (select - 32)) & 0xffffffffull);
          std::cout << " select = " << int(select)
                    << " shifted: 0x" << std::hex << shifted << std::dec
                    << " (match: " << std::boolalpha
                    << (shifted == (ECON_ROC_ALIGN_PATTERN >> 32)) << ")"
                    << std::endl;
        }
      }
    }  // loop over channels
    if (not should_continue) {
      aligned = true;
      std::cout << "all channels found pattern match on snapshot " << snapshot_val << std::endl;
      auto [all_locked, phases] = check_channel_phase_lock(econ, channels);
      std::cout << "all locked: " << all_locked << std::endl;
      break;
    }
  }
  // -------------- END SNAPSHOT BX SCAN ------------ //
  if (not aligned) {
    std::cout << "------------------------------------------" << std::endl
              << "Failure to match header pattern in ANY Snapshot."
              << std::endl;
  } 
}

void align_phase_word(Target* tgt) {
  bool on_zcu =
      (pftool::state.readout_config() == pftool::State::CFG_HCALFMC) ||
      (pftool::state.readout_config() == pftool::State::CFG_HCALOPTO_ZCU) ||
      (pftool::state.readout_config() == pftool::State::CFG_ECALOPTO_ZCU);

  debug_checks = false; //pftool::readline_bool("Enable debug checks?", true);

  int iecon = 0;
      //pftool::readline_int("Which ECON to manage: ", pftool::state.iecon);

  auto econ = tgt->econ(iecon);
  int edgesel = 0;
  int invertfcmd = 0;
  if (pftool::state.readout_config() == pftool::State::CFG_HCALOPTO_ZCU ||
      pftool::state.readout_config() == pftool::State::CFG_HCALOPTO_BW) {
    invertfcmd = 1;
  }
  // Ensure ECON is in Run mode
  econ.setRunMode(true, edgesel, invertfcmd);

  /**
   * Right now, we are assuming that all of the ROCs the target handles
   * are connected to the same ECON (DAQ or TRG) as selected by the user
   * earlier.
   */
  auto roc_ids = tgt->roc_ids();
  // Get channels dynamically from ROC to eRx object channel mapping
  auto& mapping = tgt->getRocErxMapping();
  // Dynamic channels. 2 eRx per ROC
  std::vector<int> channels;
  for (int i_roc : roc_ids) {
    channels.push_back(mapping[i_roc].first);
    channels.push_back(mapping[i_roc].second);
  }

  std::cout << "Channels to be configured: ";
  for (int ch : channels) std::cout << ch << " ";
  std::cout << std::endl;

  // Check PUSM state
  auto pusm_state = econ.readParameter("CLOCKSANDRESETS", "GLOBAL_PUSM_STATE");
  if (debug_checks) {
    std::cout << "PUSM_STATE = " << pusm_state << ", "
              << std::hex << pusm_state << std::dec << std::endl;
  }

  if (pusm_state != ECON_EXPECTED_PUSM_STATE) {
    std::cout << "PUSM_STATE / runbit does not equal "
              << ECON_EXPECTED_PUSM_STATE << ". Not running alignment task."
              << std::endl;
    return;
  }

  // Set IDLEs in ROC with enough bit transitions
  // phase alignment wants the idle to be all A
  int roc_bx_trigger = on_zcu ? 3543 : (64*40 - 20);
  std::map<std::string, std::map<std::string, uint64_t>> fancy_roc_idles;
  fancy_roc_idles["DIGITALHALF_0"]["IDLEFRAME"] = 0xaaaaaaa;
  fancy_roc_idles["DIGITALHALF_1"]["IDLEFRAME"] = 0xaaaaaaa;
  fancy_roc_idles["DIGITALHALF_0"]["BX_OFFSET"] = 1;
  fancy_roc_idles["DIGITALHALF_1"]["BX_OFFSET"] = 1;
  //fancy_roc_idles["DIGITALHALF_0"]["BX_TRIGGER"] = roc_bx_trigger;
  //fancy_roc_idles["DIGITALHALF_1"]["BX_TRIGGER"] = roc_bx_trigger;
  std::map<int, std::map<int, std::map<int, uint8_t>>> resets;
  for (int i_roc : roc_ids) {
    auto& roc{tgt->roc(i_roc)};
    resets[i_roc] = roc.applyParameters(fancy_roc_idles);
    std::cout << "roc " << i_roc << " ";
    print_roc_status(roc);
  }

  // ----- PHASE ALIGNMENT ----- //
  align_phase(tgt, econ, channels);

  // do something funkier for word alignment
  // to ensure that word alignment is functioning properly
  fancy_roc_idles.clear();
  fancy_roc_idles["DIGITALHALF_0"]["IDLEFRAME"] = ROC_WORD_IDLE_FRAME;
  fancy_roc_idles["DIGITALHALF_1"]["IDLEFRAME"] = ROC_WORD_IDLE_FRAME;
  for (int i_roc : roc_ids) {
    auto& roc{tgt->roc(i_roc)};
    // do NOT store the resets from here, use resets from first
    // apply call to revert back to prior state
    roc.applyParameters(fancy_roc_idles);
    std::cout << "roc " << i_roc << " ";
    print_roc_status(roc);
  }

  // ----- WORD ALIGNMENT ----- //
  align_word(tgt, econ, channels, on_zcu);

  for (int i_roc : roc_ids) {
    tgt->roc(i_roc).setRegisters(resets[i_roc]);
  }
}
