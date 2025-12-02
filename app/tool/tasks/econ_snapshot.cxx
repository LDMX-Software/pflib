#include "econ_snapshot.h"

#include <boost/multiprecision/cpp_int.hpp>

#include "pflib/ROC.h"
#include "pflib/packing/Hex.h"

ENABLE_LOGGING();

using pflib::packing::hex;

void econ_snapshot(Target* tgt) {
  int iecon =
      pftool::readline_int("Which ECON to manage: ", pftool::state.iecon);

  auto econ = tgt->econ(iecon);

  std::string ch_str = pftool::readline(
      "Enter channels (comma-separated), default is all channels: ",
      "0,1,2,3,4,5,6,7");

  std::vector<int> channels;
  std::stringstream ss(ch_str);
  std::string item;

  while (std::getline(ss, item, ',')) {
    try {
      channels.push_back(std::stoi(item));
    } catch (...) {
      std::cerr << "Invalid channel entry: " << item << std::endl;
    }
  }

  uint32_t binary_channels =
      build_channel_mask(channels);  // defined in align_phase_word.cxx
  std::cout << "Channels to be configured: ";
  for (int ch : channels) std::cout << ch << " ";
  std::cout << std::endl;

  // ---- TO SET ECON REGISTERS ---- //
  std::map<std::string, std::map<std::string, uint64_t>> parameters = {};

  // ---- SETTING ECON REGISTERS ---- //
  parameters["ALIGNER"]["GLOBAL_I2C_SNAPSHOT_EN"] = 0;
  parameters["ALIGNER"]["GLOBAL_SNAPSHOT_ARM"] = 0;
  parameters["ALIGNER"]["GLOBAL_SNAPSHOT_EN"] = 1;

  // FAST CONTROL - ENABLE THE BCR (ORBIT SYNC)
  tgt->fc().standard_setup();

  // ------- Scan when the ECON takes snapshot -----
  int snapshot_val = 3531;  // near your orbit region of interest
  int end_val = 3534;

  // for (int snapshot_val = start_val; snapshot_val <= end_val; snapshot_val +=
  // 1) { set Bunch Crossing (BX)
  parameters.clear();
  parameters["ALIGNER"]["GLOBAL_ORBSYN_CNT_SNAPSHOT"] = snapshot_val;
  auto econ_word_align_currentvals = econ.applyParameters(parameters);

  // FAST CONTROL - LINK_RESET
  tgt->fc().linkreset_rocs();

  std::cout << "Outputting snapshot at BX " << snapshot_val << std::endl;
  for (int channel : channels) {
    // print out snapshot

    std::string var_name_snapshot1 = std::to_string(channel) + "_SNAPSHOT_0";
    std::string var_name_snapshot2 = std::to_string(channel) + "_SNAPSHOT_1";
    std::string var_name_snapshot3 = std::to_string(channel) + "_SNAPSHOT_2";
    auto ch_snapshot_1 = econ.readParameter("CHALIGNER", var_name_snapshot1);
    auto ch_snapshot_2 = econ.readParameter("CHALIGNER", var_name_snapshot2);
    auto ch_snapshot_3 = econ.readParameter("CHALIGNER", var_name_snapshot3);

    // Combine 3 × 64-bit words into one 192-bit integer
    boost::multiprecision::uint256_t snapshot =
        (boost::multiprecision::uint256_t(ch_snapshot_3) << 128) |
        (boost::multiprecision::uint256_t(ch_snapshot_2) << 64) |
        boost::multiprecision::uint256_t(ch_snapshot_1);

    // 192-bit >> 1 shift
    uint64_t w0_shifted = (ch_snapshot_1 >> 1);
    uint64_t w1_shifted = (ch_snapshot_2 >> 1) | ((ch_snapshot_1 & 1ULL) << 63);
    uint64_t w2_shifted = (ch_snapshot_3 >> 1) | ((ch_snapshot_2 & 1ULL) << 63);

    // shift by 1
    boost::multiprecision::uint256_t shifted1 = (snapshot >> 1);

    std::cout << "Snapshot Printout: " << snapshot_val << std::endl
              << " (channel " << channel << ") " << std::endl
              << "snapshot_hex_shifted: 0x" << std::hex << std::uppercase
              << shifted1 << std::dec << std::endl;

    std::cout << "snapshot_hex: 0x" << std::hex << std::uppercase << snapshot
              << std::dec << std::endl;
  }  // end loop over snapshots for single channel
  // }

  // ensure 0 remaining 0's filling cout
  std::cout << std::dec << std::setfill(' ');

}  // End
