#include "align_phase_word.h"

#include <boost/multiprecision/cpp_int.hpp>

#include "pflib/ROC.h"
#include "pflib/packing/Hex.h"

ENABLE_LOGGING();

// ROC Idle Frame
constexpr uint32_t ROC_WORD_IDLE_FRAME = 0x12c5c57;

/**
 * The ECON-ROC alignment pattern uses the new-orbit nibble 0x9
 * prefixing the 28-bit idle word that the ROC is sending
 */
constexpr uint64_t ECON_ROC_ALIGN_PATTERN = (
    (0x9ull << 28) | ROC_WORD_IDLE_FRAME
);

// ECON PUSM State READY
constexpr int ECON_EXPECTED_PUSM_STATE = 8;

static const int ALIGNER_BASE = 0x0380;

// 12 bits, default 0x2
static const int ALIGNER_ORBSYN_CNT_SNAPSHOT = ALIGNER_BASE + 0x16;

static const int CHALIGNER_RW[12] = {0x000, 0x040, 0x080, 0x0c0, 0x100, 0x140,
                                     0x180, 0x1c0, 0x200, 0x240, 0x280, 0x2c0};

static const int CHALIGNER_RO[12] = {0x014, 0x054, 0x094, 0x0d4, 0x114, 0x154,
                                     0x194, 0x1d4, 0x214, 0x254, 0x294, 0x2d4};

// 1b, flag if pattern found
static const int CHALIGNER_PATTERN_MATCH = 0x0;

// snapshot location, 192b
static const int CHALIGNER_SNAPSHOT = 0x2;

using pflib::packing::hex;

/**
 * emit specific parameters of the roc relevant for alignment
 *
 * keep this operator overload in this file since other areas
 * may want different stuff printed
 */
class ROCAlignParams {
  pflib::ROC& roc_;
 public:
  ROCAlignParams(pflib::ROC& roc) : roc_{roc} {}
  friend inline std::ostream& operator<<(std::ostream& o, const ROCAlignParams& self) {
    auto top_params = self.roc_.getParameters("TOP");
    auto RunL = top_params.find("RUNL")->second;
    auto RunR = top_params.find("RUNR")->second;
    o << "{ RunL = " << RunL << ", RunR = " << RunR;
    for (int half = 0; half < 2; ++half) {
      auto params = self.roc_.getParameters("DIGITALHALF_" + std::to_string(half));
  
      auto idle = params.at("IDLEFRAME");
      auto bx = params.at("BX_OFFSET");
      auto bxtrig = params.at("BX_TRIGGER");
  
      o << ", idle_" << half << " = 0x" << std::hex << idle << std::dec
        << ", bx_offset_" << half << " = " << bx << ", bx_trigger_"
        << half << " = " << bxtrig;
    }
    o << " }";
    return o;
  }
};

/**
 * emit a vector as a space-separate list
 */
class SpaceSeparated {
  const std::vector<int>& v_;
 public:
  SpaceSeparated(const std::vector<int>& v) : v_{v} {}
  friend inline std::ostream& operator<<(std::ostream& o, const SpaceSeparated& self) {
    o << "[";
    for (const auto& v: self.v_) {
      o << " " << v;
    }
    o << " ]";
    return o;
  }
};

/**
 * go through and check if the input channels are locked
 *
 * get the selected phase while there
 */
std::pair<bool, std::map<int, int>> check_channel_phase_lock(
    pflib::ECON& econ, std::vector<int> channels) {
  // Check channel locks
  static const int CHEPRXGRP_RO[12] = {0x341, 0x345, 0x349, 0x34d,
                                       0x351, 0x355, 0x359, 0x35d,
                                       0x361, 0x365, 0x369, 0x36d};
  bool all_locked = true;
  std::map<int, int> phases;
  for (int ch : channels) {
    auto status = econ.getValues(CHEPRXGRP_RO[ch], 2);
    bool locked = (((status[0] >> 7) & 0b1) == 1);
    bool dll_locked = ((status[1] & 0b1) == 1);
    int dll_state = (status[0] & 0b11);
    int phase = ((status[0] >> 2) & 0xf);
    phases[ch] = phase;
    all_locked = (all_locked and locked);
    pflib_log(debug) << "channel " << ch << std::boolalpha << " locked = " << locked
                     << " dll_locked = " << dll_locked << " dll_state = " << dll_state
                     << " phase = " << phase;
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

  /**
   * manual says we should go into fixed phase mode (track_mode = 0)
   * and code the phase into the configuration so that it is
   * written down somewhere
   */
  if (all_locked) {
    pflib_log(info) << "all channels found phase lock, fixing phase into ECON configuration";
    // go into fixed phase mode to avoid the phase shifting
    parameters.clear();
    parameters["EPRXGRPTOP"]["GLOBAL_TRACK_MODE"] = 0;
    for (auto [ch, phase] : phases) {
      parameters["CHEPRXGRP"]
                [std::to_string(ch) + "_PHASE_SELECT_CHANNELINPUT"] = phase;
    }
    econ.applyParameters(parameters);
  } else {
    pflib_log(warn) << "Not all channels are locked so not fixing the phase "
                       "(leaving in track_mode = 1)";
  }
}

void align_word(Target* tgt, pflib::ECON& econ, std::vector<int> channels,
                bool on_zcu) {
  // print ROC status
  pflib_log(debug) << "ROC_WORD_IDLE_FRAME: 0x" << std::hex << ROC_WORD_IDLE_FRAME << std::dec;
  pflib_log(debug) << "ECON_ROC_ALIGN_PATTERN: 0x" << std::hex << ECON_ROC_ALIGN_PATTERN << std::dec;

  // ---- SETTING ECON REGISTERS ---- //
  std::map<std::string, std::map<std::string, uint64_t>> parameters = {};
  // BX value econ resets to when it receives BCR (linkreset)
  // Overall phase marker between ROC and ECON
  parameters["ALIGNER"]["GLOBAL_I2C_SNAPSHOT_EN"] = 0;
  parameters["ALIGNER"]["GLOBAL_SNAPSHOT_ARM"] = 0;
  parameters["ALIGNER"]["GLOBAL_SNAPSHOT_EN"] = 1;
  parameters["ALIGNER"]["GLOBAL_ORBSYN_CNT_LOAD_VAL"] = 1;
  if (on_zcu) {
    parameters["ALIGNER"]["GLOBAL_ORBSYN_CNT_MAX_VAL"] = 3563;  // 0xdeb
  } else {
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

  // configure the ECON to look for the correct alignment pattern
  econ.setValue(ALIGNER_BASE + 0x1, ECON_ROC_ALIGN_PATTERN, 8);

  // FAST CONTROL - ENABLE THE BCR (ORBIT SYNC)
  tgt->fc().standard_setup();

  // TODO: Read BX value of link reset rocd

  // ------- Scan when the ECON takes snapshot -----
  int start_val{1}, end_val{51};
  /*
  if (on_zcu) {
    start_val = 1;  // 3490;  // near your orbit region of interest
    end_val = 51;   // 3540;    // up to orbit rollover
  } else {
    start_val = 64 * 40 - 60;  // near your orbit region of interest
    end_val = 64 * 40 - 1;     // up to orbit rollover
  }
  */

  bool aligned{false};
  for (int snapshot_val = start_val; snapshot_val <= end_val;
       snapshot_val++) {

    econ.setValue(ALIGNER_ORBSYN_CNT_SNAPSHOT, snapshot_val, 2);

    // FAST CONTROL - LINK_RESET
    tgt->fc().linkreset_rocs();

    pflib_log(debug) << "checking snapshot on bx " << snapshot_val;

    bool should_continue = false;
    for (int channel : channels) {
      // print out snapshot
      auto bit_flags = econ.getValues(CHALIGNER_RO[channel], 1);
      auto ch_pm = (bit_flags[0] & 0b1);

      auto select = econ.getValues(CHALIGNER_RO[channel] + 0x1, 1)[0];

      boost::multiprecision::uint256_t snapshot{0};
      // have to read in 8byte chunks
      for (int i_snp{0}; i_snp < 3; i_snp++) {
        auto word = econ.getValues(
            CHALIGNER_RO[channel] + CHALIGNER_SNAPSHOT + 8 * i_snp, 8);
        for (int i_byte{0}; i_byte < 8; i_byte++) {
          snapshot |= (boost::multiprecision::uint256_t(word[i_byte])
                       << (8 * (8 * i_snp + i_byte)));
        }
      }

      if (should_continue or (ch_pm != 1)) {
        // this channel has not found the header so we should continue searching
        should_continue = true;
      }

      pflib_log(debug) << "channel " << channel << " match: " << std::boolalpha
                       << (ch_pm == 1) << " : 0x" << std::hex << snapshot << std::dec;
      if (ch_pm == 1) {
        auto shifted((snapshot >> (select - 32)) & 0xffffffffull);
        pflib_log(debug) << " select = " << int(select) << " shifted: 0x" << std::hex
                         << shifted << std::dec << " (match: " << std::boolalpha
                         << (shifted == ECON_ROC_ALIGN_PATTERN) << ")";
      }
    }  // loop over channels
    if (not should_continue) {
      aligned = true;
      pflib_log(info) << "all channels found pattern match with snapshot on bx "
                      << snapshot_val;
      auto [all_locked, phases] = check_channel_phase_lock(econ, channels);
      pflib_log(info) << "all channels have phase lock (still): "
                      << std::boolalpha << all_locked;
      break;
    }
  }
  // -------------- END SNAPSHOT BX SCAN ------------ //
  if (not aligned) {
    pflib_log(warn) << "failed to find header pattern in any BX checked";
  }
}

void align_phase_word(Target* tgt) {
  bool on_zcu =
      (pftool::state.readout_config() == pftool::State::CFG_HCALFMC) ||
      (pftool::state.readout_config() == pftool::State::CFG_HCALOPTO_ZCU) ||
      (pftool::state.readout_config() == pftool::State::CFG_ECALOPTO_ZCU);

  int iecon = 0;
  // pftool::readline_int("Which ECON to manage: ", pftool::state.iecon);

  auto& econ = tgt->econ(iecon);
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
   *
   * If targets evolve to multiple ECONs that each have different sets
   * of ROCs, this code will need to change to select only the ROCs
   * corresponding to the ECON that is being configured.
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

  pflib_log(info) << "Channels to be configured: " << SpaceSeparated(channels);

  // Check PUSM state
  auto pusm_state = econ.readParameter("CLOCKSANDRESETS", "GLOBAL_PUSM_STATE");
  pflib_log(debug) << "econ PUSM STATE = " << pusm_state;

  if (pusm_state != ECON_EXPECTED_PUSM_STATE) {
    pflib_log(warn) << "ECON PUSM STATE does not equal the expected value " << ECON_EXPECTED_PUSM_STATE
                    << " implying that more configuration/setup needs to happen before alignment";
    pflib_log(error) << "NOT running ECON-ROC alignment"; 
    return;
  }

  // Set IDLEs in ROC with enough bit transitions
  // phase alignment wants the idle to be all A
  std::map<std::string, std::map<std::string, uint64_t>> fancy_roc_idles;
  fancy_roc_idles["DIGITALHALF_0"]["IDLEFRAME"] = 0xaaaaaaa;
  fancy_roc_idles["DIGITALHALF_1"]["IDLEFRAME"] = 0xaaaaaaa;
  std::map<int, std::map<int, std::map<int, uint8_t>>> resets;
  for (int i_roc : roc_ids) {
    auto& roc{tgt->roc(i_roc)};
    resets[i_roc] = roc.applyParameters(fancy_roc_idles);
    pflib_log(debug) << "roc " << i_roc << " " << ROCAlignParams(roc);
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
    pflib_log(debug) << "roc " << i_roc << " " << ROCAlignParams(roc);
  }

  // ----- WORD ALIGNMENT ----- //
  align_word(tgt, econ, channels, on_zcu);

  for (int i_roc : roc_ids) {
    tgt->roc(i_roc).setRegisters(resets[i_roc]);
  }
}
