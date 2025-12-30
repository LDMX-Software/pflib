#include "econ_snapshot.h"

#include <boost/multiprecision/cpp_int.hpp>

ENABLE_LOGGING();

uint32_t build_channel_mask(std::vector<int>& channels) {
  /*
    Bit wise OR comparsion between e.g. 6 and 7,
    and shifting the '1' bit in the lowest sig bit,
    with << operator by the amount of the channel #.
  */
  uint32_t mask = 0;
  for (int ch : channels) mask |= (1 << ch);
  return mask;
}

void econ_snapshot(Target* tgt, pflib::ECON& econ, std::vector<int>& channels) {
  std::cout << "Channels to be configured: ";
  for (int ch : channels) std::cout << ch << " ";
  std::cout << std::endl;

  // ---- TO SET ECON REGISTERS ---- //
  std::map<std::string, std::map<std::string, uint64_t>> parameters = {};

  // FAST CONTROL - ENABLE THE BCR (ORBIT SYNC)
  tgt->fc().standard_setup();

  // ---- SETTING ECON REGISTERS ---- //
  parameters["ALIGNER"]["GLOBAL_I2C_SNAPSHOT_EN"] = 0;
  parameters["ALIGNER"]["GLOBAL_SNAPSHOT_ARM"] = 0;
  parameters["ALIGNER"]["GLOBAL_SNAPSHOT_EN"] = 1;
  auto econ_word_align_currentvals = econ.applyParameters(parameters);

  // FAST CONTROL - LINK_RESET
  tgt->fc().linkreset_rocs();

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

    std::cout << "Snapshot Printout: "
              << " (channel " << channel << ") " << std::endl
              << "snapshot_hex_shifted: 0x" << std::hex << std::uppercase
              << shifted1 << std::dec << std::endl;

    std::cout << "snapshot_hex: 0x" << std::hex << std::uppercase << snapshot
              << std::dec << std::endl;
  }

  // ensure 0 remaining 0's filling cout
  std::cout << std::dec << std::setfill(' ');

}  // End
